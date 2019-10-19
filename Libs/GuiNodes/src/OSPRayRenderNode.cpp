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
#endif

namespace Visus {

/////////////////////////////////////////////////////////////////////////////////////
#if VISUS_OSPRAY
class OSPRayRenderNode::Pimpl {
public:

  //constructor
  Pimpl() 
  {
    this->world = ospNewModel();
    this->camera = ospNewCamera("perspective");
    this->renderer = ospNewRenderer("scivis");

    // TODO: Set it to whatever visus viewer uses for clear color
    ospSet3f(this->renderer, "bgColor", 0.05f, 0.05f, 0.05f);

    ospSetObject(this->renderer, "model", this->world);
    ospSetObject(this->renderer, "camera", this->camera);

    this->transferFcn = ospNewTransferFunction("piecewise_linear");
  }

  //destructor
  ~Pimpl() {
    ospRelease(volume);
    ospRelease(volumeData);
    ospRelease(transferFcn);
    ospRelease(world);
    ospRelease(camera);
    ospRelease(renderer);
    ospRelease(framebuffer);
  }

  //setData
  void setData(Array data,SharedPtr<Palette> palette)
  {
    if (!data || data.getPointDim() != 3) 
      ThrowException("OSPRay Volume must be 3D");

    // Read transfer function data from the palette and pass to OSPRay,
    // assuming an RGBA palette
    if (palette->functions.size() != 4) 
      PrintInfo("WARNING: OSPRay palettes must be RGBA!");

    const size_t npaletteSamples = 256;
    std::vector<float> tfnColors(3 * npaletteSamples, 0.f);
    std::vector<float> tfnOpacities(npaletteSamples, 0.f);

    for (size_t i = 0; i < npaletteSamples; ++i) 
    {
      const float x = static_cast<float>(i) / npaletteSamples;

      // Assumes functions = {R, G, B, A}
      for (size_t j = 0; j < 3; ++j) 
        tfnColors[i * 3 + j] = palette->functions[j]->getValue(x);

      tfnOpacities[i] = palette->functions[3]->getValue(x);
    }

    OSPData colorsData = ospNewData(tfnColors.size() / 3, OSP_FLOAT3, tfnColors.data());
    ospCommit(colorsData);

    OSPData opacityData = ospNewData(tfnOpacities.size(), OSP_FLOAT, tfnOpacities.data());
    ospCommit(opacityData);

    ospSetData(transferFcn, "colors", colorsData);
    ospSetData(transferFcn, "opacities", opacityData);

    // TODO: Somehow get the value range of the array
    ospSet2f(transferFcn, "valueRange", 0.f, 255.f);
    ospCommit(transferFcn);

    if (volume) 
    {
      ospRemoveVolume(world, volume);
      ospRelease(volume);
      // Not actually releasing the Array, just OSPRay's Data struct
      ospRelease(volumeData);
    }

    volume = ospNewVolume("shared_structured_volume");
    const OSPDataType ospDType = dtypeToOSPDtype(data.dtype);

    // The OSP_DATA_SHARED_BUFFER flag tells OSPRay to not copy the data , but to just share the pointer with us.
    volumeData = ospNewData(data.getTotalNumberOfSamples(), ospDType,data.c_ptr(), OSP_DATA_SHARED_BUFFER);

    // TODO: How to get the value range of the array?
    ospSet2f(volume, "voxelRange", 0.f, 255.f);
    ospSetString(volume, "voxelType", ospDTypeStr(ospDType).c_str()); //scrgiorgio: seems ospray does not support multi-channel VR
    ospSet3i(volume, "dimensions", data.getWidth(), data.getHeight(), data.getDepth());
    ospSetData(volume, "voxelData", volumeData);
    ospSetObject(volume, "transferFunction", transferFcn);

    PrintInfo(data.bounds);

    auto grid = data.bounds.toAxisAlignedBox();
    grid.setPointDim(3);

    // Scale the smaller volumes we get while loading progressively to fill the true bounds
    // of the full dataset
    ospSet3f(volume, "gridSpacing",
      (grid.p2[0] - grid.p1[0]) / data.getWidth(),
      (grid.p2[1] - grid.p1[1]) / data.getHeight(),
      (grid.p2[2] - grid.p1[2]) / data.getDepth());

    // TODO: This parameter should be exposed in the UI
    // Sampling rate will adjust the quality and cost of rendering,
    // lower: faster, poorer quality, higher: slower, better quality
    ospSet1f(volume, "samplingRate", 0.125f);

    ospCommit(volume);

    ospAddVolume(world, volume);
    ospCommit(world);

    this->data    = data;
    this->palette = palette;
  }

  //glRender
  void glRender(GLCanvas& gl)
  {
    Time startRender = Time::now();

    // TODO: This should be done by setting the volume clip box instead
    if (data.clipping.valid()) {
      PrintInfo("CLIPPING TODO");
      // This should set the volume parameters: volumeClippingBoxLower
      // and volumeClippingBoxUpper
    }

    // Extract camera parameters from model view matrix
    const auto invCamera = gl.getModelview().invert();
    const auto eyePos = invCamera * Point4d(0.f, 0.f, 0.f, 1.f);
    const auto eyeDir = invCamera * Point4d(0.f, 0.f, -1.f, 0.f);
    const auto upDir  = invCamera * Point4d(0.f, 1.f, 0.f, 0.f);

    ospSet3f(camera, "pos", eyePos.x, eyePos.y, eyePos.z);
    ospSet3f(camera, "dir", eyeDir.x, eyeDir.y, eyeDir.z);
    ospSet3f(camera, "up", upDir.x, upDir.y, upDir.z);

    // Get window dimensions for framebuffer
    const auto viewport = gl.getViewport();

    if (viewport.width != imgDims[0] || viewport.height != imgDims[1]) 
    {
      imgDims[0] = viewport.width;
      imgDims[1] = viewport.height;
      ospSetf(camera, "aspect", imgDims[0] / static_cast<float>(imgDims[1]));

      if (framebuffer) {
        ospRelease(framebuffer);
      }
      osp::vec2i dims{ imgDims[0], imgDims[1] };
      framebuffer = ospNewFrameBuffer(dims, OSP_FB_SRGBA, OSP_FB_COLOR | OSP_FB_ACCUM);
    }

    ospCommit(camera);
    ospCommit(renderer);

    // TODO: We can use progressive accumulation if we know the camera didn't move
    // and the scene hasn't changed. But it looks like this render function is only
    // called if the data, camera, etc. has changed.
    ospFrameBufferClear(framebuffer, OSP_FB_COLOR | OSP_FB_ACCUM);

    ospRenderFrame(framebuffer, renderer, OSP_FB_COLOR | OSP_FB_ACCUM);

    uint32_t *fb = (uint32_t*)ospMapFrameBuffer(framebuffer, OSP_FB_COLOR);
    PrintInfo("OSPRay rendering total took:",startRender.elapsedMsec(),"ms");

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

    ospUnmapFrameBuffer(fb, framebuffer);

    PrintInfo("OSPRayNode total took:",startRender.elapsedMsec(),"ms");
  }

private:

  Array data;
  SharedPtr<Palette> palette;

  OSPVolume volume = nullptr;
  OSPData volumeData = nullptr;
  OSPTransferFunction transferFcn = nullptr;
  OSPModel world = nullptr;
  OSPCamera camera = nullptr;
  OSPRenderer renderer = nullptr;
  OSPFrameBuffer framebuffer = nullptr;

  std::array<int, 2>  imgDims = { -1,-1 };

  //dtypeToOSPDtype
  static OSPDataType dtypeToOSPDtype(const DType& dtype) {

    if (dtype == DTypes::INT8) return OSP_CHAR;

    if (dtype == DTypes::UINT8     ) return OSP_UCHAR;
    if (dtype == DTypes::UINT8_GA  ) return OSP_UCHAR2;
    if (dtype == DTypes::UINT8_RGB ) return OSP_UCHAR3;
    if (dtype == DTypes::UINT8_RGBA) return OSP_UCHAR4;

    if (dtype == DTypes::UINT16) return OSP_SHORT;
    if (dtype == DTypes::INT16 ) return OSP_USHORT;

    if (dtype == DTypes::INT32 ) return OSP_INT;
    if (dtype == DTypes::UINT32) return OSP_UINT;

    if (dtype == DTypes::INT64  ) return OSP_LONG;
    if (dtype == DTypes::UINT64 ) return OSP_ULONG;

    if (dtype == DTypes::FLOAT32     ) return OSP_FLOAT;
    if (dtype == DTypes::FLOAT32_GA  ) return OSP_FLOAT2;
    if (dtype == DTypes::FLOAT32_RGB ) return OSP_FLOAT3;
    if (dtype == DTypes::FLOAT32_RGBA) return OSP_FLOAT4;

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
OSPRayRenderNode::OSPRayRenderNode(String name) : Node(name)
{
  pimpl = new Pimpl();

  addInputPort("array");
  addInputPort("palette");
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

  PrintInfo("got array","data",this->data.dims);
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

