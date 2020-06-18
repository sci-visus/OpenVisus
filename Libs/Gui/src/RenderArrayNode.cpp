/*-----------------------------------------------------------------------------
Copyright(c) 2010 - 2018 ViSUS L.L.C.,
Scientific Computing and Imaging Institute of the University of Utah

ViSUS L.L.C., 50 W.Broadway, Ste. 300, 84101 - 2044 Salt Lake City, UT
University of Utah, 72 S Central Campus Dr, Room 3750, 84112 Salt Lake City, UT

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met :

* Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

For additional information about this project contact : pascucci@acm.org
For support : support@visus.net
-----------------------------------------------------------------------------*/

#include <Visus/RenderArrayNode.h>
#include <Visus/GLCanvas.h>
#include <Visus/StringTree.h>

namespace Visus {

  /////////////////////////////////////////////////////////////////////////////
class RenderArrayNodeShaderConfig
{
public:

  int  texture_dim = 0;       //2 or 3
  int  texture_nchannels = 0; //(1) luminance (2) luminance+alpha (3) rgb (4) rgba
  bool clippingbox_enabled = false;
  bool palette_enabled = false;
  bool lighting_enabled = false;
  bool discard_if_zero_alpha = false;

  //constructor
  RenderArrayNodeShaderConfig() {}

  //valid
  bool valid() const {
    return (texture_dim == 2 || texture_dim == 3) && (texture_nchannels >= 1 && texture_nchannels <= 4);
  }

  //operator<
  bool operator<(const RenderArrayNodeShaderConfig& other) const {
    return this->key() < other.key();
  }

private:

  //key
  std::tuple<int, int, bool, bool, bool, bool> key() const {
    VisusAssert(valid());
    return std::make_tuple(texture_dim, texture_nchannels, clippingbox_enabled, palette_enabled, lighting_enabled, discard_if_zero_alpha);
  }

};

/////////////////////////////////////////////////////////////////////////////
class RenderArrayNodeShader : public GLShader
{
public:

  VISUS_NON_COPYABLE_CLASS(RenderArrayNodeShader)

  typedef RenderArrayNodeShaderConfig Config;

  static std::map<Config, RenderArrayNodeShader*> shaders;

  Config config;

  //constructor
  RenderArrayNodeShader(const Config& config_) :
    GLShader(":/RenderArrayShader.glsl"),
    config(config_)
  {
    addDefine("CLIPPINGBOX_ENABLED", cstring(config.clippingbox_enabled ? 1 : 0));
    addDefine("TEXTURE_DIM", cstring(config.texture_dim));
    addDefine("TEXTURE_NCHANNELS", cstring(config.texture_nchannels));
    addDefine("LIGHTING_ENABLED", cstring(config.lighting_enabled ? 1 : 0));
    addDefine("PALETTE_ENABLED", cstring(config.palette_enabled ? 1 : 0));
    addDefine("DISCARD_IF_ZERO_ALPHA", cstring(config.discard_if_zero_alpha ? 1 : 0));

    u_sampler = addSampler("u_sampler");
    u_palette_sampler = addSampler("u_palette_sampler");
    u_opacity = addUniform("u_opacity");
  }

  //destructor
  virtual ~RenderArrayNodeShader() {
  }

  //getSingleton
  static RenderArrayNodeShader* getSingleton(const Config& config) {
    auto it = shaders.find(config);
    if (it != shaders.end()) return it->second;
    auto ret = new RenderArrayNodeShader(config);
    shaders[config] = ret;
    return ret;
  }

  //setTexture
  void setTexture(GLCanvas& gl, SharedPtr<GLTexture> value) {
    gl.setTexture(u_sampler, value);
  }

  //setPaletteTexture 
  void setPaletteTexture(GLCanvas& gl, SharedPtr<GLTexture> value)
  {
    VisusAssert(config.palette_enabled);
    gl.setTextureInSlot(1, u_palette_sampler, value);
  }

  //setOpacity
  void setOpacity(GLCanvas& gl, double value) {
    gl.setUniform(u_opacity, (float)value);
  }

private:

  GLSampler u_sampler;
  GLSampler u_palette_sampler;
  GLUniform u_opacity;

};

std::map<RenderArrayNodeShader::Config, RenderArrayNodeShader*> RenderArrayNodeShader::shaders;


void RenderArrayNode::allocShaders()
{
  //nothing to do
}

void RenderArrayNode::releaseShaders()
{
  for (auto it : RenderArrayNodeShader::shaders)
    delete it.second;
  RenderArrayNodeShader::shaders.clear();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
RenderArrayNode::RenderArrayNode()
{
  addInputPort("array");
  addInputPort("palette");

  lighting_material.front.ambient=Colors::Black;
  lighting_material.front.diffuse=Colors::White;
  lighting_material.front.specular=Colors::White;
  lighting_material.front.shininess=100;

  lighting_material.back.ambient=Colors::Black;
  lighting_material.back.diffuse=Colors::White;
  lighting_material.back.specular=Colors::White;
  lighting_material.back.shininess=100;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
RenderArrayNode::~RenderArrayNode()
{}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
void RenderArrayNode::execute(Archive& ar)
{
  if (ar.name == "SetLightingMaterial") {
    GLMaterial value;
    value.read(*ar.getFirstChild());
    setLightingMaterial(value);
    return;
  }

  if (ar.name == "SetLightingEnabled") {
    bool value;
    ar.read("value", value);
    setLightingEnabled(value);
    return;
  }

  if (ar.name == "SetPaletteEnabled") {
    bool value;
    ar.read("value", value);
    setPaletteEnabled(value);
    return;
  }

  if (ar.name == "SetUseViewDirection") {
    bool value;
    ar.read("value", value);
    setUseViewDirection(value);
    return;
  }

  if (ar.name == "SetMaxNumSlices") {
    int value;
    ar.read("value", value);
    setMaxNumSlices(value);
    return;
  }

  if (ar.name == "SetMinifyFilter") {
    int value;
    ar.read("value", value);
    setMinifyFilter(value);
    return;
  }

  if (ar.name == "SetMagnifyFilter") {
    int value;
    ar.read("value", value);
    setMagnifyFilter(value);
    return;
  }

  return Node::execute(ar);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
void RenderArrayNode::setData(Array value,SharedPtr<Palette> palette)
{
  VisusAssert(VisusHasMessageLock());

  Time t1 = Time::now();

  if (!value || !value.dims.innerProduct() || !value.dtype.valid())
  {
    this->data = Array();
    this->data_texture.reset();
    this->palette.reset();
    this->palette_texture.reset();
    return;
  }

  bool got_new_data = (value.heap != this->data.heap);

  this->data = value;

  //automatically convert to RGB
  int ncomponents = this->data.dtype.ncomponents();
  if (ncomponents >= 5)
    this->data = ArrayUtils::withNumberOfComponents(this->data, 3);

  //create texture
  if (!this->data_texture || got_new_data)
    this->data_texture = std::make_shared<GLTexture>(this->data);

  //overrule filter
  this->data_texture->minfilter = this->minifyFilter();
  this->data_texture->magfilter = this->magnifyFilter();

  //compute translate scale for texture values (to go in the color range 0,1)
  //TODO: this range stuff can be slow and blocking...
  auto& vs = this->data_texture->vs; vs = Point4d(1, 1, 1, 1);
  auto& vt = this->data_texture->vt; vt = Point4d(0, 0, 0, 0);

  if (!this->data.dtype.isVectorOf(DTypes::UINT8))
  {
    int ncomponents = this->data.dtype.ncomponents();

    for (int C = 0; C < std::min(4, ncomponents); C++)
    {
      Range range;
      if (palette) range = palette->computeRange(this->data, C);
      if (range.delta() <= 0) range = this->data.dtype.getDTypeRange(C);
      if (range.delta() <= 0) range = ArrayUtils::computeRange(this->data, C);
      auto p = range.getScaleTranslate();
      vs[C] = p.first;
      vt[C] = p.second;
    }

    //1 component will end up in texture RGB, I want all channels to be the same (as it was gray)
    if (ncomponents == 1)
    {
      vs = Point4d(vs[0], vs[0], vs[0], 1.0);
      vt = Point4d(vt[0], vt[0], vt[0], 0.0);
    }

  }

  //palette
  this->palette = palette;
  if (palette)
    this->palette_texture = std::make_shared<GLTexture>(palette->toArray());

  //if you want to see what's going on...
#if 0
  {
    static int cont = 0;
    ArrayUtils::saveImageUINT8(concatenate("tmp/debug_render_array_node/", cont++, ".png"), *data);
  }
#endif

  //note I'm not sure if the texture can be generated... 
  PrintInfo("got array",
    "dims",value.dims, "dtype",value.dtype,
    "scheduling texture upload" ,data_texture->dims, data_texture->dtype, StringUtils::getStringFromByteSize(data_texture->dtype.getByteSize(this->data_texture->dims)),
    "got_new_data", got_new_data,
    "msec", t1.elapsedMsec());
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////
bool RenderArrayNode::processInput()
{
  //I want to sign the input return receipt only after the rendering
  auto return_receipt = createPassThroughtReceipt();
  auto palette        = readValue<Palette>("palette");
  auto data           = readValue<Array>("array");

  this->return_receipt.reset();

  if (!data || !data->dims.innerProduct() || !data->dtype.valid())
  {
    setData(Array());
    return false;
  }

  //so far I can apply the transfer function on the GPU only if the data is atomic
  //TODO: i can support even 3 and 4 component arrays
  bool bPaletteEnabled = paletteEnabled() || (palette && data->dtype.ncomponents() == 1);
  if (!bPaletteEnabled)
    palette.reset();

  this->return_receipt = return_receipt;
  setData(*data, palette);

  return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
void RenderArrayNode::glRender(GLCanvas& gl) 
{
  if (!data_texture) 
    return;

  auto return_receipt=this->return_receipt;
  this->return_receipt.reset();

  if (!data_texture->uploaded())
  {
    if (!data_texture->textureId(gl))
    {
      PrintInfo("Failed to upload the texture");
      return;
    }

    PrintInfo("Uploaded texture",data_texture->dims, data_texture->dtype, StringUtils::getStringFromByteSize(data_texture->dtype.getByteSize(this->data_texture->dims)));
  }

  //need to upload a new palette?
  bool b3D = data.dims.getPointDim()>2 && 
    data.dims[0] > 1 && 
    data.dims[1] > 1 && 
    data.dims[2] > 1;

  RenderArrayNodeShader::Config config;
  config.texture_dim                    = b3D?3:2;
  config.texture_nchannels              = data.dtype.ncomponents();
  config.palette_enabled                = palette_texture?true:false;
  config.lighting_enabled               = lightingEnabled();
  config.clippingbox_enabled            = gl.hasClippingBox() || (data.clipping.valid() || useViewDirection());
  config.discard_if_zero_alpha          = true;

  RenderArrayNodeShader* shader=RenderArrayNodeShader::getSingleton(config);
  gl.setShader(shader);

  if (shader->config.lighting_enabled)
  {
    gl.setUniformMaterial(*shader,this->lighting_material);

    Point3d pos,center,vup;
    gl.getModelview().getLookAt(pos, center,vup, 1.0);
    gl.setUniformLight(*shader,Point4d(pos,1.0));
  }

  if (data.clipping.valid())
    gl.pushClippingBox(data.clipping);

  //in 3d I render the box (0,0,0)x(1,1,1)
  //in 2d I render the box (0,0)  x(1,1)
  gl.pushModelview();
  {
    auto box=data.bounds.getBoxNd();

    gl.multModelview(data.bounds.getTransformation());
    gl.multModelview(Matrix::translate(box.p1));
    gl.multModelview(Matrix::nonZeroScale(box.size()));
    if (shader->config.texture_dim==2)
    {
      if (!box.size()[0]) 
        gl.multModelview(Matrix(Point3d(0,1,0),Point3d(0,0,1),Point3d(1,0,0),Point3d(0,0,0)));

      else if (!box.size()[1]) 
        gl.multModelview(Matrix(Point3d(1,0,0),Point3d(0,0,1),Point3d(0,1,0),Point3d(0,0,0)));
    }
  }

  gl.pushBlend(true);
  gl.pushDepthMask(shader->config.texture_dim==3?false:true);

  //note: the order seeems to be important, first bind GL_TEXTURE1 then GL_TEXTURE0
  if (shader->config.palette_enabled && palette_texture)
    shader->setPaletteTexture(gl, palette_texture);

  shader->setTexture(gl, data_texture);

  if (b3D)
  {
    double opacity = this->opacity;
    
    int nslices = this->maxNumSlices();
    if (nslices <= 0)
    {
      double factor = useViewDirection() ? 2.5 : 1; //show a little more if use_view_direction!
      nslices = (int)(*data.dims.max_element() * factor);
    }

    //see https://github.com/sci-visus/visus/issues/99
    const int max_nslices = 128;
    if (bFastRendering && nslices > max_nslices) {
      opacity *= nslices / (double)max_nslices;
      nslices = max_nslices;
    }

    shader->setOpacity(gl, opacity);

    if (useViewDirection())
    {
      if (!data.clipping.valid())
        gl.pushClippingBox(BoxNd(Point3d(0,0,0),Point3d(1,1,1)));

      gl.glRenderMesh(GLMesh::ViewDependentUnitVolume(gl.getFrustum(), nslices));

      if (!data.clipping.valid())
        gl.popClippingBox();
    }
    else
      gl.glRenderMesh(GLMesh::AxisAlignedUnitVolume(gl.getFrustum(),nslices));
  }
  else
  {
    shader->setOpacity(gl, opacity);
    gl.glRenderMesh(GLMesh::Quad(Point2d(0,0),Point2d(1,1),/*bNormal*/shader->config.lighting_enabled,/*bTexCoord*/true));
  }

  gl.popDepthMask();
  gl.popBlend();
  gl.popModelview();

  if (data.clipping.valid())
    gl.popClippingBox();
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////
void RenderArrayNode::write(Archive& ar) const
{
  Node::write(ar);

  ar.write("lighting_enabled", lighting_enabled);
  ar.write("palette_enabled", palette_enabled);
  ar.write("use_view_direction", use_view_direction);
  ar.write("max_num_slices", max_num_slices);
  ar.write("magnify_filter", magnify_filter);
  ar.write("minify_filter", minify_filter);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
void RenderArrayNode::read(Archive& ar)
{
  Node::read(ar);

  ar.read("lighting_enabled", lighting_enabled);
  ar.read("palette_enabled", palette_enabled);
  ar.read("use_view_direction", use_view_direction);
  ar.read("max_num_slices", max_num_slices);
  ar.read("magnify_filter", magnify_filter);
  ar.read("minify_filter", minify_filter);
}
 
/////////////////////////////////////////////////////////
class RenderArrayNodeView :
  public QFrame,
  public View<RenderArrayNode>
{
public:

  //constructor
  RenderArrayNodeView(RenderArrayNode* model = nullptr) {
    if (model)
      bindModel(model);
  }

  //destructor
  virtual ~RenderArrayNodeView() {
    bindModel(nullptr);
  }

  //bindModel
  virtual void bindModel(RenderArrayNode* model) override
  {
    if (this->model)
    {
      QUtils::clearQWidget(this);
    }

    View<ModelClass>::bindModel(model);

    if (this->model)
    {
      std::map< int, String> filter_options = {
        {GL_LINEAR,"linear"},
        {GL_NEAREST,"nearest"}
      };

      QFormLayout* layout = new QFormLayout();
      layout->addRow("Enable lighting", GuiFactory::CreateCheckBox(model->lightingEnabled(), "", [model](int value) {model->setLightingEnabled(value); }));
      layout->addRow("Minify filter", GuiFactory::CreateIntegerComboBoxWidget(model->minifyFilter(), filter_options, [model](int value) {model->setMinifyFilter(value); }));
      layout->addRow("Magnify filter", GuiFactory::CreateIntegerComboBoxWidget(model->magnifyFilter(), filter_options, [model](int value) {model->setMagnifyFilter(value); }));
      layout->addRow("Enable Palette", GuiFactory::CreateCheckBox(model->paletteEnabled(), "", [model](int value) {model->setPaletteEnabled(value); }));
      layout->addRow("Use view direction", GuiFactory::CreateCheckBox(model->useViewDirection(), "", [model](int value) {model->setUseViewDirection(value); }));
      layout->addRow("Max slices", GuiFactory::CreateIntegerTextBoxWidget(model->maxNumSlices(), [model](int value) {model->setMaxNumSlices(value); }));
      setLayout(layout);
    }
  }

};


void RenderArrayNode::createEditor()
{
  auto win = new RenderArrayNodeView(this);
  win->show();
}

} //namespace Visus

