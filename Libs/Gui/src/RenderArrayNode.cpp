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
#include <Visus/GLPhongShader.h>

#if VISUS_OSPRAY
#if WIN32
#pragma warning(disable:4005)
#endif
#include <ospray/ospray.h>
#include <ospray/ospray_cpp.h>
using namespace ospray;
using namespace ospcommon;
#endif

namespace Visus {

/////////////////////////////////////////////////////////////////////////////
class RenderArrayNode::Pimpl
{
public:

  //destructor
  virtual ~Pimpl() {}

  //setData
  virtual void setData(Array data, SharedPtr<Palette> palette) = 0;

  //glRender
  virtual void glRender(GLCanvas& gl) = 0;

};


/////////////////////////////////////////////////////////////////////////////
class OpenGLRenderArrayNode : public RenderArrayNode::Pimpl
{
public:

  //___________________________________________________________________________
  class Config
  {
  public:

    int  texture_dim = 0;       //2 or 3
    int  texture_nchannels = 0; //(1) luminance (2) luminance+alpha (3) rgb (4) rgba
    bool clippingbox_enabled = false;
    bool palette_enabled = false;
    bool lighting_enabled = false;
    bool discard_if_zero_alpha = false;

    //constructor
    Config() {}

    //valid
    bool valid() const {
      return (texture_dim == 2 || texture_dim == 3) && (texture_nchannels >= 1 && texture_nchannels <= 4);
    }

    //operator<
    bool operator<(const Config& other) const {
      return this->key() < other.key();
    }

  private:

    //key
    std::tuple<int, int, bool, bool, bool, bool> key() const {
      VisusAssert(valid());
      return std::make_tuple(texture_dim, texture_nchannels, clippingbox_enabled, palette_enabled, lighting_enabled, discard_if_zero_alpha);
    }

  };

  //___________________________________________________________________________
  class MyShader : public GLShader
  {
  public:

    VISUS_NON_COPYABLE_CLASS(MyShader)

    Config config;

    GLSampler u_sampler;
    GLSampler u_palette_sampler;
    GLUniform u_opacity;

    //constructor
    MyShader(const Config& config_) :
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
    virtual ~MyShader() {
    }

    //shaders
    static std::map<Config, SharedPtr<MyShader> >& shaders() {
      static std::map<Config, SharedPtr<MyShader> > ret;
      return ret;
    }

    //getSingleton
    static MyShader* getSingleton(const Config& config) {
      auto it = shaders().find(config);
      if (it != shaders().end()) return it->second.get();
      auto ret = std::make_shared<MyShader>(config);
      shaders()[config] = ret;
      return ret.get();
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

  };

  RenderArrayNode* owner;
  Array data;
  SharedPtr<GLTexture> data_texture;
  SharedPtr<GLTexture> palette_texture;

  //constructor
  OpenGLRenderArrayNode(RenderArrayNode* owner_) : owner(owner_) {
  }

  //destructor
  virtual ~OpenGLRenderArrayNode() {
  }

  //setData
  virtual void setData(Array data, SharedPtr<Palette> palette) override
  {
    VisusAssert(VisusHasMessageLock());

    Time t1 = Time::now();

    if (!data || !data.dims.innerProduct() || !data.dtype.valid())
    {
      this->data = Array();
      this->data_texture.reset();
      this->palette_texture.reset();
      return;
    }

    bool got_new_data = (data.heap != this->data.heap);
    this->data = data;

    //automatically convert to RGB
    int ncomponents = this->data.dtype.ncomponents();
    if (ncomponents >= 5)
      this->data = ArrayUtils::withNumberOfComponents(this->data, 3);

    //create texture
    if (!this->data_texture || got_new_data)
      this->data_texture = std::make_shared<GLTexture>(this->data);

    //overrule filter
    this->data_texture->minfilter = owner->minifyFilter();
    this->data_texture->magfilter = owner->magnifyFilter();

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
      "dims", data.dims, "dtype", data.dtype,
      "scheduling texture upload", data_texture->dims, data_texture->dtype, StringUtils::getStringFromByteSize(data_texture->dtype.getByteSize(this->data_texture->dims)),
      "got_new_data", got_new_data,
      "msec", t1.elapsedMsec());
  }

  //glRender
  virtual void glRender(GLCanvas& gl) override 
  {
    if (!data_texture)
      return;

    if (!data_texture->uploaded())
    {
      if (!data_texture->textureId(gl))
      {
        PrintInfo("Failed to upload the texture");
        return;
      }

      PrintInfo("Uploaded texture", data_texture->dims, data_texture->dtype, StringUtils::getStringFromByteSize(data_texture->dtype.getByteSize(this->data_texture->dims)));
    }

    //need to upload a new palette?
    bool b3D = data.dims.getPointDim() > 2 &&
      data.dims[0] > 1 &&
      data.dims[1] > 1 &&
      data.dims[2] > 1;

    Config config;
    config.texture_dim = b3D ? 3 : 2;
    config.texture_nchannels = data.dtype.ncomponents();
    config.palette_enabled = palette_texture ? true : false;
    config.lighting_enabled = owner->lightingEnabled();
    config.clippingbox_enabled = gl.hasClippingBox() || (data.clipping.valid() || owner->useViewDirection());
    config.discard_if_zero_alpha = true;

    MyShader* shader = MyShader::getSingleton(config);
    gl.setShader(shader);

    if (shader->config.lighting_enabled)
    {
      gl.setUniformMaterial(*shader, owner->getLightingMaterial());

      Point3d pos, center, vup;
      gl.getModelview().getLookAt(pos, center, vup, 1.0);
      gl.setUniformLight(*shader, Point4d(pos, 1.0));
    }

    if (data.clipping.valid())
      gl.pushClippingBox(data.clipping);

    //in 3d I render the box (0,0,0)x(1,1,1)
    //in 2d I render the box (0,0)  x(1,1)
    gl.pushModelview();
    {
      auto box = data.bounds.getBoxNd();

      gl.multModelview(data.bounds.getTransformation());
      gl.multModelview(Matrix::translate(box.p1));
      gl.multModelview(Matrix::nonZeroScale(box.size()));
      if (shader->config.texture_dim == 2)
      {
        if (!box.size()[0])
          gl.multModelview(Matrix(Point3d(0, 1, 0), Point3d(0, 0, 1), Point3d(1, 0, 0), Point3d(0, 0, 0)));

        else if (!box.size()[1])
          gl.multModelview(Matrix(Point3d(1, 0, 0), Point3d(0, 0, 1), Point3d(0, 1, 0), Point3d(0, 0, 0)));
      }
    }

    gl.pushBlend(true);
    gl.pushDepthMask(shader->config.texture_dim == 3 ? false : true);

    //note: the order seeems to be important, first bind GL_TEXTURE1 then GL_TEXTURE0
    if (shader->config.palette_enabled && palette_texture)
      shader->setPaletteTexture(gl, palette_texture);

    shader->setTexture(gl, data_texture);

    if (b3D)
    {
      double opacity = owner->opacity;

      int nslices = owner->maxNumSlices();
      if (nslices <= 0)
      {
        double factor = owner->useViewDirection() ? 2.5 : 1; //show a little more if use_view_direction!
        nslices = (int)(*data.dims.max_element() * factor);
      }

      //see https://github.com/sci-visus/visus/issues/99
      const int max_nslices = 128;
      if (owner->bFastRendering && nslices > max_nslices) {
        opacity *= nslices / (double)max_nslices;
        nslices = max_nslices;
      }

      shader->setOpacity(gl, opacity);

      if (owner->useViewDirection())
      {
        if (!data.clipping.valid())
          gl.pushClippingBox(BoxNd(Point3d(0, 0, 0), Point3d(1, 1, 1)));

        gl.glRenderMesh(GLMesh::ViewDependentUnitVolume(gl.getFrustum(), nslices));

        if (!data.clipping.valid())
          gl.popClippingBox();
      }
      else
        gl.glRenderMesh(GLMesh::AxisAlignedUnitVolume(gl.getFrustum(), nslices));
    }
    else
    {
      shader->setOpacity(gl, owner->opacity);
      gl.glRenderMesh(GLMesh::Quad(Point2d(0, 0), Point2d(1, 1),/*bNormal*/shader->config.lighting_enabled,/*bTexCoord*/true));
    }

    gl.popDepthMask();
    gl.popBlend();
    gl.popModelview();

    if (data.clipping.valid())
      gl.popClippingBox();
  }
};


/////////////////////////////////////////////////////////////////////////////////////
#if VISUS_OSPRAY
class OSPRayRenderArrayNode : public RenderArrayNode::Pimpl{
public:

  RenderArrayNode* owner;

  // Note: The C++ wrappers automatically manage life time tracking and reference
  // counting for the OSPRay objects, and OSPRay internally tracks references to
  // parameters
  ospray::cpp::Instance instance;
  ospray::cpp::World world;
  ospray::cpp::Camera camera;
  ospray::cpp::Renderer renderer;
  ospray::cpp::FrameBuffer framebuffer;

  Point4d prevEyePos = Point4d(0.f, 0.f, 0.f, 0.f);
  Point4d prevEyeDir = Point4d(0.f, 0.f, 0.f, 0.f);
  Point4d prevUpDir = Point4d(0.f, 0.f, 0.f, 0.f);
  bool sceneChanged = true;
  bool volumeValid = false;

  float varianceThreshold = 15.f;

  std::array<int, 2> imgDims = { -1,-1 };

  //constructor
  OSPRayRenderArrayNode(RenderArrayNode* owner_) : owner(owner_)
  {
    world = cpp::World();
    camera = cpp::Camera("perspective");
    // TODO: Need a way to tell visus to keep calling render even if the camera isn't moving
    // so we can do progressive refinement
    if (VisusModule::getModuleConfig()->readString("Configuration/VisusViewer/DefaultRenderNode/ospray_renderer") == "pathtracer") {
      renderer = cpp::Renderer("pathtracer");
    }
    else {
      renderer = cpp::Renderer("scivis");
    }

    // create and setup an ambient light
    cpp::Light ambient_light("ambient");
    ambient_light.setParam("intensity", 0.25f);
    ambient_light.commit();

    cpp::Light directional_light("distant");
    directional_light.setParam("direction", math::vec3f(0.5f, 1.f, 0.25f));
    directional_light.setParam("intensity", 10.f);
    directional_light.commit();
    std::vector<cpp::Light> lights = { ambient_light, directional_light };
    world.setParam("light", cpp::Data(lights));

    // This is the Colors::DarkBlue color but pre-transformed srgb -> linear
    // so that when it's converted to srgb for display it will match the
    // regular render node (which doesn't seem to do srgb?)
    const math::vec3f bgColor(0.021219f, 0.0423114f, 0.093059f);
    renderer.setParam("backgroundColor", bgColor);
    renderer.setParam("varianceThreshold", varianceThreshold);
    renderer.setParam("maxPathLength", int(8));
    renderer.setParam("volumeSamplingRate", 0.25f);
    renderer.commit();
  }

  //destructor
  virtual ~OSPRayRenderArrayNode() {
  }


  //setData
  virtual void setData(Array data, SharedPtr<Palette> palette) override
  {
    if (!data || data.getPointDim() != 3)
    {
      this->volumeValid = false;
      return;
    }
   
    // Read transfer function data from the palette and pass to OSPRay,
    // assuming an RGBA palette
    if (palette->functions.size() != 4)
      PrintInfo("WARNING: OSPRay palettes must be RGBA!");

    sceneChanged = true;

    std::vector<cpp::Instance> instances;

    for (int ch=0; ch<data.dtype.ncomponents(); ch++){
      
      // should == data if ncomponent==1 and ch==0
      Array channel = data.getComponent(ch);
      
      std::cout <<"channel whd: "
		<<channel.getWidth() <<" "
		<<channel.getHeight() <<" "
		<<channel.getDepth()<<" "
		<<channel.dtype.toString() <<"\n";
      
      const size_t npaletteSamples = 128;
      std::vector<math::vec3f> tfnColors(npaletteSamples, math::vec3f(0.f));
      std::vector<float> tfnOpacities(npaletteSamples, 0.f);

      for (size_t i = 0; i < npaletteSamples; ++i)
	{
	  const float x = static_cast<float>(i) / npaletteSamples;

	  // Assumes functions = {R, G, B, A}
	  // assigin one color to each channel
	  //for (size_t j = 0; j < 3; ++j)
	  tfnColors[i][ch] = palette->functions[ch]->getValue(x);

	  tfnOpacities[i] = palette->functions[3]->getValue(x);
	}

      cpp::TransferFunction transferFcn("piecewiseLinear");
      transferFcn.setParam("color", cpp::Data(tfnColors));
      transferFcn.setParam("opacity", cpp::Data(tfnOpacities));
      const Range range = palette->computeRange(channel, 0);
      transferFcn.setParam("valueRange", math::vec2f(range.from, range.to));
      transferFcn.commit();

      const math::vec3ul volumeDims(channel.getWidth(), channel.getHeight(), channel.getDepth());
      
      // OSPRay shares the data pointer with us, does not copy internally
      const OSPDataType ospDType = DTypeToOSPDtype(channel.dtype);
      cpp::Data volumeData;

     
      
      if (ospDType == OSP_UCHAR) {
	volumeData = cpp::Data(volumeDims, reinterpret_cast<uint8_t*>(channel.c_ptr()), true);
      }
      else if (ospDType == OSP_USHORT) {
	volumeData = cpp::Data(volumeDims, reinterpret_cast<uint16_t*>(channel.c_ptr()), true);
	//std::vector<uint16_t> arr(channel.getWidth()*channel.getHeight()*channel.getDepth(),0);
	//volumeData = cpp::Data(volumeDims, arr.data(), true);
	std::cout <<"dtype ushort\n";
      }
      else if (ospDType == OSP_FLOAT) {
	volumeData = cpp::Data(volumeDims, reinterpret_cast<float*>(channel.c_ptr()), true);
      }
      else if (ospDType == OSP_DOUBLE) {
	volumeData = cpp::Data(volumeDims, reinterpret_cast<double*>(channel.c_ptr()), true);
      }
      else {
	PrintInfo("OSPRay only supports scalar voxel types");
	volumeValid = false;
	return;
      }
      volumeValid = true;

      cpp::Volume volume = cpp::Volume("structuredRegular");
      volume.setParam("dimensions", volumeDims);
      volume.setParam("data", volumeData);
      volume.setParam("voxelType", int(ospDType));

      auto grid = channel.bounds.toAxisAlignedBox();
      grid.setPointDim(3);

      // Scale the smaller volumes we get while loading progressively to fill the true bounds
      // of the full dataset
      const math::vec3f gridSpacing(
				    (grid.p2[0] - grid.p1[0]) / channel.getWidth(),
				    (grid.p2[1] - grid.p1[1]) / channel.getHeight(),
				    (grid.p2[2] - grid.p1[2]) / channel.getDepth());
      volume.setParam("gridSpacing", gridSpacing);
      volume.commit();

      // TODO setup group/instance/world
      cpp::VolumetricModel volumeModel(volume);
      volumeModel.setParam("transferFunction", transferFcn);
      volumeModel.commit();

      cpp::Group group;
      group.setParam("volume", cpp::Data(volumeModel));
      group.commit();

      cpp::Instance instance(group);
      instance.commit();

      instances.push_back(instance);

    }
    
    world.setParam("instance", cpp::Data(instances));
    // TODO some lights?
    world.commit();

    if (data.clipping.valid())
      PrintInfo("CLIPPING TODO");
  }
  
  //glRender
  virtual void glRender(GLCanvas& gl) override
  {
    if (!volumeValid) {
      PrintInfo("Skipping rendering unsupported volume");
      return;
    }

    // Extract camera parameters from model view matrix
    // TODO track camera position to see if it changed and reset accum only if that changed
    const auto invCamera = gl.getModelview().invert();
    const auto eyePos = invCamera * Point4d(0.f, 0.f, 0.f, 1.f);
    const auto eyeDir = invCamera * Point4d(0.f, 0.f, -1.f, 0.f);
    const auto upDir = invCamera * Point4d(0.f, 1.f, 0.f, 0.f);

    if (eyePos != prevEyePos || eyeDir != prevEyeDir || upDir != prevUpDir) {
      camera.setParam("position", math::vec3f(eyePos.x, eyePos.y, eyePos.z));
      camera.setParam("direction", math::vec3f(eyeDir.x, eyeDir.y, eyeDir.z));
      camera.setParam("up", math::vec3f(upDir.x, upDir.y, upDir.z));
      camera.commit();
      sceneChanged = true;
    }
    prevEyePos = eyePos;
    prevEyeDir = eyeDir;
    prevUpDir = upDir;

    // Get window dimensions for framebuffer
    const auto viewport = gl.getViewport();

    if (viewport.width != imgDims[0] || viewport.height != imgDims[1])
    {
      // On windows it seems like the ref-counting doesn't quite work and we leak?
      ospRelease(framebuffer.handle());
      sceneChanged = true;

      imgDims[0] = viewport.width;
      imgDims[1] = viewport.height;

      camera.setParam("aspect", imgDims[0] / static_cast<float>(imgDims[1]));
      camera.commit();

      framebuffer = cpp::FrameBuffer(math::vec2i(imgDims[0], imgDims[1]), OSP_FB_SRGBA,
        OSP_FB_COLOR | OSP_FB_ACCUM | OSP_FB_VARIANCE);
    }

    if (sceneChanged) {
      sceneChanged = false;
      framebuffer.clear();
    }

    framebuffer.renderFrame(renderer, camera, world);

    uint32_t* fb = (uint32_t*)framebuffer.map(OSP_FB_COLOR);
    // Blit the rendered framebuffer from OSPRay
    {
      auto fbArray = Array(imgDims[0], imgDims[1], DTypes::UINT8_RGBA, HeapMemory::createUnmanaged(fb, imgDims[0] * imgDims[1] * 4));
      auto fbTexture = SharedPtr<GLTexture>(new GLTexture(fbArray));
      fbTexture->minfilter = GL_NEAREST;
      fbTexture->magfilter = GL_NEAREST;

      GLPhongShader* shader = GLPhongShader::getSingleton(GLPhongShader::Config().withTextureEnabled(true));
      gl.setShader(shader);
      shader->setUniformColor(gl, Colors::White);
      shader->setTexture(gl, fbTexture);

      // Render directly to normalized device coordinates and overwrite everything
      // with the rendered result from OSPRay
      gl.pushModelview();
      gl.pushProjection();

      gl.setModelview(Matrix::identity(4));
      gl.setProjection(Matrix::identity(4));

      gl.glRenderMesh(GLMesh::Quad(
        Point3d(-1, -1, 0.5), 
        Point3d(+1, -1, 0.5),
        Point3d(+1, +1, 0.5),
        Point3d(-1, +1, 0.5), false, true));

      gl.popProjection();
      gl.popModelview();
    }
    framebuffer.unmap(fb);

    if (ospGetVariance(framebuffer.handle()) > varianceThreshold) {
      gl.postRedisplay();
    }
  }

  //DTypeToOSPDtype
  static OSPDataType DTypeToOSPDtype(const DType& dtype) {

    if (dtype == DTypes::INT8) return OSP_CHAR;

    if (dtype == DTypes::UINT8) return OSP_UCHAR;
    if (dtype == DTypes::UINT8_GA) return OSP_VEC2UC;
    if (dtype == DTypes::UINT8_RGB) return OSP_VEC3UC;
    if (dtype == DTypes::UINT8_RGBA) return OSP_VEC4UC;

    if (dtype == DTypes::UINT16) return OSP_USHORT;
    if (dtype == DTypes::INT16) return OSP_SHORT;

    if (dtype == DTypes::INT32) return OSP_INT;
    if (dtype == DTypes::UINT32) return OSP_UINT;

    if (dtype == DTypes::INT64) return OSP_LONG;
    if (dtype == DTypes::UINT64) return OSP_ULONG;

    if (dtype == DTypes::FLOAT32) return OSP_FLOAT;
    if (dtype == DTypes::FLOAT32_GA) return OSP_VEC2F;
    if (dtype == DTypes::FLOAT32_RGB) return OSP_VEC3F;
    if (dtype == DTypes::FLOAT32_RGBA) return OSP_VEC4F;

    if (dtype == DTypes::FLOAT64) return OSP_DOUBLE;

    return OSP_UNKNOWN;
  }

  //OspDTypeStr
  static String OspDTypeStr(const OSPDataType t)
  {
    switch (t)
    {
    case OSP_UCHAR:  return "uchar";
    case OSP_SHORT:  return "short";
    case OSP_USHORT: return "ushort";
    case OSP_FLOAT: return  "float";
    case OSP_DOUBLE: return "double";
    default: break;
    }
    ThrowException("Unsupported data type for OSPVolume");
    return nullptr;
  }

};


#endif // #if VISUS_OSPRAY

/////////////////////////////////////////////////////////////////////////////////////////////////////////
RenderArrayNode::RenderArrayNode()
{
  addInputPort("array");
  addInputPort("palette");

  this->render_type = "OpenGL";
  this->pimpl = new OpenGLRenderArrayNode(this);

  lighting_material.front.ambient=Colors::Black;
  lighting_material.front.diffuse=Colors::White;
  lighting_material.front.specular=Colors::White;
  lighting_material.front.shininess=100;

  lighting_material.back.ambient=Colors::Black;
  lighting_material.back.diffuse=Colors::White;
  lighting_material.back.specular=Colors::White;
  lighting_material.back.shininess=100;

  // XUAN: force ospray for now!!!!!!!!!!!!!!!!
  setRenderType("OSPRay");
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
RenderArrayNode::~RenderArrayNode()
{
  if (pimpl) {
    delete pimpl;
    pimpl = nullptr;
  }
}

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

  if (ar.name == "SetRenderType") {
    String value;
    ar.read("value", value);
    setRenderType(value);
    return;
  }

  return Node::execute(ar);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
void RenderArrayNode::setData(Array data,SharedPtr<Palette> palette)
{
  VisusAssert(VisusHasMessageLock());

  //invalid data?
  if (!data || !data.dims.innerProduct() || !data.dtype.valid())
  {
    data = Array();
    palette.reset();
  }

  this->data    = data;
  this->palette = palette;
  pimpl->setData(data, palette);
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
    setData(Array(), SharedPtr<Palette>());
    return false;
  }

  //so far I can apply the transfer function on the GPU only if the data is atomic
  //TODO: i can support even 3 and 4 component arrays

  bool bPaletteEnabled = paletteEnabled() || (palette && data->dtype.ncomponents() == 1);
  //if (!bPaletteEnabled)
  //  palette.reset();

  this->return_receipt = return_receipt; //wait until the

  setData(*data, palette);

  return true;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////
void RenderArrayNode::setRenderType(String value)
{
  if (value.empty())
    value = "OpenGL";

#if !VISUS_OSPRAY
  value = "OpenGL";
#endif

  //useless call
  if (value == render_type)
    return;

  //deallocate old pimpl
  if (pimpl) {
    delete pimpl;
    pimpl = nullptr;
  }

#if VISUS_OSPRAY
  if (value == "OSPRay")
    pimpl = new OSPRayRenderArrayNode(this);
#endif

  if (!pimpl)
  {
    value = "OpenGL";
    this->pimpl = new OpenGLRenderArrayNode(this);
  }
  
  //set new pimpl
  pimpl->setData(this->data, this->palette);
  setProperty("SetRenderType", this->render_type, value);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
void RenderArrayNode::glRender(GLCanvas& gl) 
{
  auto return_receipt = this->return_receipt;
  this->return_receipt.reset();

  if (!data)
    return;

  pimpl->glRender(gl);
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
  ar.write("render_type", render_type);
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

  //need to create the pimpl
  {
    String value;
    ar.read("render_type", value);
    setRenderType(value);
  }
}

void RenderArrayNode::allocShaders()
{
  //nothing to do
}

void RenderArrayNode::releaseShaders()
{
  OpenGLRenderArrayNode::MyShader::shaders().clear();
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

#if VISUS_OSPRAY
      std::vector<String> options = { "OpenGL" , "OSPRay" };
#else
      std::vector<String> options = { "OpenGL" };
#endif

      layout->addRow("Render dtype",GuiFactory::CreateComboBox(model->getRenderType().c_str(), options, [model](String s) { model->setRenderType(s); }));

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

