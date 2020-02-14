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

#include <Visus/OSPRayRenderNode.h>
#include <Visus/GLCanvas.h>
#include <Visus/StringTree.h>
#include <Visus/GLPhongShader.h>
#include <Visus/DType.h>
#include <Visus/Kernel.h>

#if VISUS_OSPRAY
#include <ospray/ospray.h>
#include <ospray/ospray_cpp.h>

#endif

namespace Visus {

/////////////////////////////////////////////////////////////////////////////////////
#if VISUS_OSPRAY
class OSPRayRenderNode::Pimpl {
public:

  //constructor
  Pimpl() 
  {
    using namespace ospray;
    using namespace ospcommon;

    world = cpp::World();
    camera = cpp::Camera("perspective");
    // TODO: Need a way to tell visus to keep calling render even if the camera isn't moving
    // so we can do progressive refinement
    if (VisusModule::getModuleConfig()->readString("Configuration/VisusViewer/DefaultRenderNode/ospray_renderer") == "pathtracer") {
      renderer = cpp::Renderer("pathtracer");
    } else {
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
    std::vector<cpp::Light> lights = {ambient_light, directional_light};
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
  ~Pimpl() = default;

  //setData
  void setData(Array data,SharedPtr<Palette> palette)
  {
    using namespace ospcommon;
    using namespace ospray;

    if (!data || data.getPointDim() != 3) 
      ThrowException("OSPRay Volume must be 3D");

    // Read transfer function data from the palette and pass to OSPRay,
    // assuming an RGBA palette
    if (palette->functions.size() != 4) 
      PrintInfo("WARNING: OSPRay palettes must be RGBA!");

    sceneChanged = true;

    const size_t npaletteSamples = 128;
    std::vector<math::vec3f> tfnColors(npaletteSamples, math::vec3f(0.f));
    std::vector<float> tfnOpacities(npaletteSamples, 0.f);

    for (size_t i = 0; i < npaletteSamples; ++i) 
    {
      const float x = static_cast<float>(i) / npaletteSamples;

      // Assumes functions = {R, G, B, A}
      for (size_t j = 0; j < 3; ++j)
      {
        tfnColors[i][j] = palette->functions[j]->getValue(x);
      }

      tfnOpacities[i] = palette->functions[3]->getValue(x);
    }

    cpp::TransferFunction transferFcn("piecewiseLinear");
    transferFcn.setParam("color", cpp::Data(tfnColors));
    transferFcn.setParam("opacity", cpp::Data(tfnOpacities));
    const Range range = palette->computeRange(data, 0);
    transferFcn.setParam("valueRange", math::vec2f(range.from, range.to));
    transferFcn.commit();

    const OSPDataType ospDType = dtypeToOSPDtype(data.dtype);
    const math::vec3ul volumeDims(data.getWidth(), data.getHeight(), data.getDepth());

    cpp::Volume volume = cpp::Volume("structuredRegular");
    volume.setParam("dimensions", volumeDims);

    // OSPRay shares the data pointer with us, does not copy internally
    cpp::Data volumeData;
    if (ospDType == OSP_UCHAR) {
        volumeData = cpp::Data(volumeDims, reinterpret_cast<uint8_t*>(data.c_ptr()), true);
    } else if (ospDType == OSP_USHORT) {
        volumeData = cpp::Data(volumeDims, reinterpret_cast<uint16_t*>(data.c_ptr()), true);
    } else if (ospDType == OSP_FLOAT) {
        volumeData = cpp::Data(volumeDims, reinterpret_cast<float*>(data.c_ptr()), true);
    } else if (ospDType == OSP_DOUBLE) {
        volumeData = cpp::Data(volumeDims, reinterpret_cast<double*>(data.c_ptr()), true);
    } else {
        throw std::runtime_error("Unsupported voxel type for OSPRay volume rendr node");
    }

    volume.setParam("data", volumeData);
    volume.setParam("voxelType", int(ospDType));

    auto grid = data.bounds.toAxisAlignedBox();
    grid.setPointDim(3);

    // Scale the smaller volumes we get while loading progressively to fill the true bounds
    // of the full dataset
    const math::vec3f gridSpacing(
      (grid.p2[0] - grid.p1[0]) / data.getWidth(),
      (grid.p2[1] - grid.p1[1]) / data.getHeight(),
      (grid.p2[2] - grid.p1[2]) / data.getDepth());
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

    world.setParam("instance", cpp::Data(instance));
    // TODO some lights?
    world.commit();

    this->palette = palette;
  }

  //glRender
  void glRender(GLCanvas& gl)
  {
    using namespace ospray;
    using namespace ospcommon;

    if (data.clipping.valid()) {
      PrintInfo("CLIPPING TODO");
    }

    // Extract camera parameters from model view matrix
    // TODO track camera position to see if it changed and reset accum only if that changed
    const auto invCamera = gl.getModelview().invert();
    const auto eyePos = invCamera * Point4d(0.f, 0.f, 0.f, 1.f);
    const auto eyeDir = invCamera * Point4d(0.f, 0.f, -1.f, 0.f);
    const auto upDir  = invCamera * Point4d(0.f, 1.f, 0.f, 0.f);

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

    uint32_t *fb = (uint32_t*)framebuffer.map(OSP_FB_COLOR);
    // Blit the rendered framebuffer from OSPRay
    {
      auto fbArray = Array(imgDims[0], imgDims[1], DTypes::UINT8_RGBA,HeapMemory::createUnmanaged(fb, imgDims[0] * imgDims[1] * 4));
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

      gl.glRenderMesh(GLMesh::Quad(Point3d(-1, -1, 0.5), Point3d(1, -1, 0.5),
        Point3d(1, 1, 0.5), Point3d(-1, 1, 0.5), false, true));

      gl.popProjection();
      gl.popModelview();
    }
    framebuffer.unmap(fb);

    if (ospGetVariance(framebuffer.handle()) > varianceThreshold) {
        gl.postRedisplay();
    }
  }

private:

  Array data;
  SharedPtr<Palette> palette;

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

  float varianceThreshold = 15.f;

  std::array<int, 2> imgDims = { -1,-1 };

  //dtypeToOSPDtype
  static OSPDataType dtypeToOSPDtype(const DType& dtype) {

    if (dtype == DTypes::INT8) return OSP_CHAR;

    if (dtype == DTypes::UINT8     ) return OSP_UCHAR;
    if (dtype == DTypes::UINT8_GA  ) return OSP_VEC2UC;
    if (dtype == DTypes::UINT8_RGB ) return OSP_VEC3UC;
    if (dtype == DTypes::UINT8_RGBA) return OSP_VEC4UC;

    if (dtype == DTypes::UINT16) return OSP_SHORT;
    if (dtype == DTypes::INT16 ) return OSP_USHORT;

    if (dtype == DTypes::INT32 ) return OSP_INT;
    if (dtype == DTypes::UINT32) return OSP_UINT;

    if (dtype == DTypes::INT64  ) return OSP_LONG;
    if (dtype == DTypes::UINT64 ) return OSP_ULONG;

    if (dtype == DTypes::FLOAT32     ) return OSP_FLOAT;
    if (dtype == DTypes::FLOAT32_GA  ) return OSP_VEC2F;
    if (dtype == DTypes::FLOAT32_RGB ) return OSP_VEC3F;
    if (dtype == DTypes::FLOAT32_RGBA) return OSP_VEC4F;

    if (dtype == DTypes::FLOAT64) return OSP_DOUBLE;

    ThrowException("Unsupported Visus Datatype");
    return (OSPDataType)0;
  }

  //ospDTypeStr
  static String ospDTypeStr(const OSPDataType t) 
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
#else

/////////////////////////////////////////////////////////////////////////////////////////////////////////
class OSPRayRenderNode::Pimpl
{
public:

  //constructor
  Pimpl(){
  }

  //destructor
  ~Pimpl() {
  }

  //setData
  void setData(Array data, SharedPtr<Palette> palette) {
  }

  //glRender
  void glRender(GLCanvas& gl) {
  }
};


#endif



/////////////////////////////////////////////////////////////////////////////////////////////////////////
OSPRayRenderNode::OSPRayRenderNode()
{
  pimpl = new Pimpl();

  addInputPort("array");
  addInputPort("palette");
#ifndef VISUS_OSPRAY
  ThrowException("Requested to use OSPRayRenderNode, but OSPRay support was not compiled in!");
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
OSPRayRenderNode::~OSPRayRenderNode()
{
  delete pimpl;
}

void OSPRayRenderNode::initEngine()
{
#if VISUS_OSPRAY
  if (ospInit(&CommandLine::argn, CommandLine::argv) != OSP_NO_ERROR)
    ThrowException("Failed to initialize OSPRay");
#endif
}

void OSPRayRenderNode::shutdownEngine()
{
#if VISUS_OSPRAY
  ospShutdown();
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
bool OSPRayRenderNode::processInput()
{
  //I want to sign the input return receipt only after the rendering
  auto return_receipt = createPassThroughtReceipt();
  auto palette = readValue<Palette>("palette");
  auto data    = readValue<Array>("array");

  //request to flush all
  if (!data || !data->dims.innerProduct() || !data->dtype.valid())
  {
    this->return_receipt.reset();
    this->data = Array();
    this->palette.reset();
    return false;
  }

  pimpl->setData(*data, palette);

  //so far I can apply the transfer function on the GPU only if the data is atomic
  bool bPaletteEnabled = (palette && data && data->dtype.ncomponents() == 1);
  if (!bPaletteEnabled)
    palette.reset();

  this->return_receipt = return_receipt;
  this->data   = *data;
  this->palette = palette;

  return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
void OSPRayRenderNode::glRender(GLCanvas& gl)
{
  if (!data)
    return;

  auto return_receipt = this->return_receipt;
  this->return_receipt.reset();
  pimpl->glRender(gl);
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////
void OSPRayRenderNode::write(Archive& ar) const
{
  Node::write(ar);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
void OSPRayRenderNode::read(Archive& ar)
{
  Node::read(ar);
}

} //namespace Visus

