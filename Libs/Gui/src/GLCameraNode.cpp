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

#include <Visus/GLCameraNode.h>
#include <Visus/GLLookAtCamera.h>
#include <Visus/GLOrthoCamera.h>

namespace Visus {

//////////////////////////////////////////////////
GLCameraNode::GLCameraNode(SharedPtr<GLCamera> glcamera)
{
  setGLCamera(glcamera);
}

//////////////////////////////////////////////////
GLCameraNode::~GLCameraNode()
{
  setGLCamera(nullptr);
}

//////////////////////////////////////////////////
void GLCameraNode::execute(Archive& ar)
{
  if (GetPassThroughAction("glcamera", ar))
  {
    getGLCamera()->execute(ar);
    return;
  }

  return Node::execute(ar);
}
//////////////////////////////////////////////////
void GLCameraNode::setGLCamera(SharedPtr<GLCamera> value) 
{
  if (this->glcamera)
  {
    this->glcamera->begin_update.disconnect(glcamera_begin_update_slot);
    this->glcamera->end_update.disconnect(glcamera_end_update_slot);
  }

  this->glcamera=value;

  if (this->glcamera) 
  {
    this->glcamera->begin_update.connect(glcamera_begin_update_slot=[this](){
      beginTransaction();
    });

    this->glcamera->end_update.connect(glcamera_end_update_slot =[this](){
      addUpdate(
        CreatePassThroughAction("glcamera", this->glcamera->lastRedo()),
        CreatePassThroughAction("glcamera", this->glcamera->lastUndo()));
      endTransaction();
    });
  }
}

//////////////////////////////////////////////////
void GLCameraNode::write(Archive& ar) const
{
  Node::write(ar);

  if (glcamera)
  {
    StringTree sub(glcamera->getTypeName());
    glcamera->write(sub);
    ar.addChild(sub);
  }

  //bDebugFrustum
}

//////////////////////////////////////////////////
void GLCameraNode::read(Archive& ar)
{
  Node::read(ar);

  if (ar.getNumberOfChilds())
  {
    auto sub = *ar.getFirstChild();
    this->glcamera = GLCamera::decode(sub);
  }
  else
  {
    this->glcamera.reset();
  }
}
  
} //namespace Visus









