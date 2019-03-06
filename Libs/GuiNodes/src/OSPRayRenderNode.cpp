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
#include <Visus/VisusConfig.h>
#include <Visus/GLPhongShader.h>

namespace Visus {

/////////////////////////////////////////////////////////////////////////////////////////////////////////
OSPRayRenderNode::OSPRayRenderNode(String name) : Node(name)
{
  addInputPort("data");
  addInputPort("palette");
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
OSPRayRenderNode::~OSPRayRenderNode()
{}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
bool OSPRayRenderNode::processInput()
{
  Time t1 = Time::now();

  //I want to sign the input return receipt only after the rendering
  auto return_receipt = ReturnReceipt::createPassThroughtReceipt(this);
  auto palette = readInput<Palette>("palette");
  auto data = readInput<Array>("data");

  //so far I can apply the transfer function on the GPU only if the data is atomic
  bool bPaletteEnabled = (palette && data && data->dtype.ncomponents() == 1);
  if (!bPaletteEnabled)
    palette.reset();

  //request to flush all
  if (!data || !data->dims.innerProduct() || !data->dtype.valid())
  {
    this->return_receipt.reset();
    this->data = Array();
    this->palette.reset();
    return false;
  }

  this->return_receipt = return_receipt;
  this->data = *data;
  this->palette = palette;

  VisusInfo() << "got array"  << " data(" << this->data.dims.toString() << ")";
  return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
void OSPRayRenderNode::glRender(GLCanvas& gl)
{
  if (!data)
    return;

  SharedPtr<ReturnReceipt> return_receipt = this->return_receipt;
  this->return_receipt.reset();

  if (data.clipping.valid())
    gl.pushClippingBox(data.clipping);

  //here your draw code, the example here just shows a bounding box
  {
    GLPhongShader* shader = GLPhongShader::getSingleton(GLPhongShader::Config().withTextureEnabled(false));
    gl.setShader(shader);
    shader->setUniformColor(gl, Colors::Yellow);

    auto T = data.bounds.getTransformation();
    auto box = data.bounds.getBox();

    gl.pushModelview();
    gl.multModelview(T);
    gl.setLineWidth(8);
    gl.glRenderMesh(GLMesh::WireBox(box));
    gl.popModelview();
  }

  if (data.clipping.valid())
    gl.popClippingBox();
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////
void OSPRayRenderNode::writeToObjectStream(ObjectStream& ostream)
{
  Node::writeToObjectStream(ostream);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
void OSPRayRenderNode::readFromObjectStream(ObjectStream& istream)
{
  Node::readFromObjectStream(istream);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
void OSPRayRenderNode::writeToSceneObjectStream(ObjectStream& ostream)
{
  Node::writeToSceneObjectStream(ostream);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
void OSPRayRenderNode::readFromSceneObjectStream(ObjectStream& istream)
{
  Node::readFromSceneObjectStream(istream);
}

} //namespace Visus

