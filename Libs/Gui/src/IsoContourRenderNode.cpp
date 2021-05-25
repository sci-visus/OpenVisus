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

#include <Visus/IsoContourRenderNode.h>

namespace Visus {


  /////////////////////////////////////////////////////////////////////////////////////////////////////////
class IsoContourShaderConfig
{
public:

  bool clippingbox_enabled = false;
  int second_field_nchannels=0;

  //constructor
  IsoContourShaderConfig() {
  }

  //valid
  bool valid() const {
    return true;
  }

  //operator<
  bool operator<(const IsoContourShaderConfig& other) const {
    return this->key() < other.key();
  }

private:

  //key
  std::tuple<bool, int> key() const {
    return std::make_tuple(clippingbox_enabled, second_field_nchannels);
  }

};


/////////////////////////////////////////////////////////////////////////////////////////////////////////
class IsoContourShader : public GLShader
{
public:

  VISUS_NON_COPYABLE_CLASS(IsoContourShader)

  typedef IsoContourShaderConfig Config;

  static std::map<Config, IsoContourShader*> shaders;

  Config config;

  GLSampler u_field;
  GLSampler u_second_field;
  GLSampler u_palette;

  //constructor
  IsoContourShader(const Config& config_) :
    GLShader(":/IsoContourShader.glsl"),
    config(config_)
  {
    addDefine("CLIPPINGBOX_ENABLED", cstring(config.clippingbox_enabled ? 1 : 0));
    addDefine("SECOND_FIELD_NCHANNELS", cstring(config.second_field_nchannels));

    u_field = addSampler("u_field");
    u_second_field = addSampler("u_second_field");
    u_palette = addSampler("u_palette");
  }

  //destructor
  virtual ~IsoContourShader()
  {}

  //getSingleton
  static IsoContourShader* getSingleton(const Config& config)
  {
    auto it = shaders.find(config);
    if (it != shaders.end()) return it->second;
    auto ret = new IsoContourShader(config);
    shaders[config] = ret;
    return ret;
  }


};

std::map<IsoContourShader::Config, IsoContourShader*> IsoContourShader::shaders;

void IsoContourRenderNode::allocShaders()
{
}

void IsoContourRenderNode::releaseShaders()
{
  for (auto it : IsoContourShader::shaders)
    delete it.second;
  IsoContourShader::shaders.clear();
}

///////////////////////////////////////////////////////////////////////////
IsoContourRenderNode::IsoContourRenderNode() 
{
  addInputPort("mesh");
  addInputPort("palette"); //if provided, can color the vertices by (say) height
}

///////////////////////////////////////////////////////////////////////////
IsoContourRenderNode::~IsoContourRenderNode() {
}

///////////////////////////////////////////////////////////////////////////
void IsoContourRenderNode::execute(Archive& ar)
{
  if (ar.name == "SetMaterial")
  {
    GLMaterial value;
    value.read(*ar.getFirstChild());
    setMaterial(value);
    return;
  }

  return Node::execute(ar);
}

///////////////////////////////////////////////////////////////////////////
void IsoContourRenderNode::setMaterial(GLMaterial new_value) {
  setEncodedProperty("SetMaterial", this->material, new_value);
}


///////////////////////////////////////////////////////////////////////////
void IsoContourRenderNode::setMesh(SharedPtr<IsoContour> value) {

  VisusReleaseAssert(VisusHasMessageLock());
  this->mesh = value; //not part of the model
}


///////////////////////////////////////////////////////////////////////////
void IsoContourRenderNode::setPalette(SharedPtr<Palette> value) {
  if (value) value->texture.reset(); //force regeneration
  this->palette = value;//not part of the model
}

///////////////////////////////////////////////////////////////////////////
bool IsoContourRenderNode::processInput()
{
  auto palette = readValue<Palette>("palette");
  auto mesh      = readValue<IsoContour>("mesh");
  setPalette(palette);
  setMesh(mesh);
  return mesh ? true : false;
}

/////////////////////////////////////////////////////////////
void IsoContourRenderNode::glRender(GLCanvas& gl)
{
  if (!mesh)
    return;

  //gpu normals (first component for computing normals, second component for applying palette)
  //I can calculate the normals on GPU, eventually if the incoming 
  //field has 2 components I can use the second component to shop on top of the surface

  //NOT: isocontour mesh vertices are in data.dims space
  auto data = mesh->field;
  auto T = Position::computeTransformation(data.bounds, data.dims);

  //main field (for gpu normal computation)
  {
    mesh->field.texture = mesh->field.texture? mesh->field.texture : GLTexture::createFromArray(mesh->field);
    if (!mesh->field.texture)
      return;

    //if you want to debug
#if 0
    {
      static int I = 0;
      auto slice = Array::createView(mesh->field, mesh->field.dims[0], mesh->field.dims[1], mesh->field.dtype);
      ArrayUtils::saveImageUINT8(concatenate(this->name, ".", I++, ".png"), slice);
    }
#endif

  }

  //upload second field
  if (mesh->second_field.valid())
  {
    mesh->second_field.texture = mesh->second_field.texture? mesh->second_field.texture : GLTexture::createFromArray(mesh->second_field);
    if (!mesh->second_field.texture)
      return;

    palette->texture = palette->texture? palette->texture : GLTexture::createFromArray(palette->toArray());
    if (!palette->texture)
      return;
  }

  //how to clip bounds
  if (data.clipping.valid())
    gl.pushClippingBox(data.clipping);

  gl.pushModelview();
  gl.multModelview(T);
  Point3d pos,dir,vup;
  gl.getModelview().getLookAt(pos, dir,vup);

  IsoContourShader::Config config;
  config.clippingbox_enabled = gl.hasClippingBox();
  config.second_field_nchannels = mesh->second_field.dtype.ncomponents();

  auto shader=IsoContourShader::getSingleton(config);
  gl.setShader(shader);
  gl.setUniformMaterial(*shader, config.second_field_nchannels? GLMaterial() : this->material);
  gl.setUniformLight(*shader,Point4d(pos,1.0));

  gl.setTexture(shader->u_field, std::dynamic_pointer_cast<GLTexture>(mesh->field.texture));

  if (mesh->second_field.valid())
  {
    gl.setTextureInSlot(1, shader->u_second_field, std::dynamic_pointer_cast<GLTexture>(mesh->second_field.texture));
    gl.setTextureInSlot(2, shader->u_palette, std::dynamic_pointer_cast<GLTexture>(palette->texture));
  }

  gl.glRenderMesh(*mesh);
  gl.popModelview();

  if (data.clipping.valid())
    gl.popClippingBox();
}

/////////////////////////////////////////////////////////////
void IsoContourRenderNode::write(Archive& ar) const
{
  Node::write(ar);
  ar.writeObject("material", material);
  //NOTE: the palette is a runtime value, and is not part of the model
}

/////////////////////////////////////////////////////////////
void IsoContourRenderNode::read(Archive& ar)
{
  Node::read(ar);
  ar.readObject("material", material);
  //NOTE: the palette is a runtime value, and is not part of the model
}

/////////////////////////////////////////////////////////
class IsoContourRenderNodeView :
  public QFrame,
  public View<IsoContourRenderNode>
{
public:

  //constructor
  IsoContourRenderNodeView(IsoContourRenderNode* model) {
    bindModel(model);
  }

  //destructor
  virtual ~IsoContourRenderNodeView() {
    bindModel(nullptr);
  }

  //bindModel
  virtual void bindModel(IsoContourRenderNode* model) override
  {
    if (this->model)
    {
      QUtils::clearQWidget(this);
    }

    View<ModelClass>::bindModel(model);

    if (this->model)
    {
      auto layout = new QVBoxLayout();
      layout->addWidget(GuiFactory::CreateGLMaterialView(model->getMaterial(), [model](GLMaterial value) {model->setMaterial(value); }));
      setLayout(layout);
    }
  }

};

void IsoContourRenderNode::createEditor()
{
  auto win = new IsoContourRenderNodeView(this);
  win->show();
}


} //namespace Visus
