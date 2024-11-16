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

#ifndef VISUS_RENDER_ISOCONTOUR_NODE_H
#define VISUS_RENDER_ISOCONTOUR_NODE_H

#include <Visus/Gui.h>
#include <Visus/IsoContourNode.h>
#include <Visus/TransferFunction.h>

#include <Visus/GLCanvas.h>
#include <Visus/Model.h>
#include <Visus/GuiFactory.h>

namespace Visus {

//////////////////////////////////////////////////////////////
class VISUS_GUI_API IsoContourRenderNode : 
  public Node, 
  public GLObject 
{
public:

  VISUS_NON_COPYABLE_CLASS(IsoContourRenderNode)

  //constructor
  IsoContourRenderNode();

  //destructor
  virtual ~IsoContourRenderNode();

  //getTypeName
  virtual String getTypeName() const override {
    return "IsoContourRenderNode";
  }

  //glRender
  virtual void glRender(GLCanvas& gl) override;
   
  //getBounds
  virtual Position getBounds() override {
    return mesh? mesh->field.bounds : Position::invalid();
  }

  //getMaterial
  GLMaterial getMaterial() const {
    return material;
  }

  //setMaterial
  void setMaterial(GLMaterial new_value);

  //setBackColor
  void setBackColor(Color diffuse);

  //setFrontColor
  void setFrontColor(Color diffuse);

  //getPalette
  SharedPtr<Palette> getPalette() const {
    return palette;
  }

  //getPalette
  void setPalette(SharedPtr<Palette> value);

  //getMesh
  SharedPtr<IsoContour> getMesh() const {
    return mesh;
  }

  //setMesh
  void setMesh(SharedPtr<IsoContour> value);

  //dataflow
  virtual bool processInput() override;

  //createEditor
  virtual void createEditor();

public:

  //shaders
  static void allocShaders();
  static void releaseShaders();

public:

  static IsoContourRenderNode* castFrom(Node* obj) {
    return dynamic_cast<IsoContourRenderNode*>(obj);
  }

public:

  //execute
  virtual void execute(Archive& ar) override;

  //write
  virtual void write(Archive& ar) const override;

  //read
  virtual void read(Archive& ar) override;

private:

  SharedPtr<IsoContour> mesh;
  GLMaterial            material = GLMaterial::createRandom();
  SharedPtr<Palette>    palette=Palette::getDefault("grayopaque");

};



} //namespace Visus


#endif //VISUS_RENDER_ISOCONTOUR_NODE_H




