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


#ifndef VISUS_RENDER_ARRAYNODE_H
#define VISUS_RENDER_ARRAYNODE_H

#include <Visus/Gui.h>
#include <Visus/DataflowNode.h>
#include <Visus/TransferFunction.h>
#include <Visus/Model.h>
#include <Visus/GuiFactory.h>
#include <Visus/GLCanvas.h>

namespace Visus {

  ////////////////////////////////////////////////////////////////////
class VISUS_GUI_API RenderArrayNode : 
  public Node,
  public GLObject 
{
public:

  VISUS_PIMPL_CLASS(RenderArrayNode)

  //(run time) fast rendering
  bool bFastRendering=false;

  //(run time) opacity
  double opacity = 1.0;

  //constructor
  RenderArrayNode();

  //destructor
  virtual ~RenderArrayNode();

  //getTypeName
  virtual String getTypeName() const override {
    return "RenderArrayNode";
  }

  //getData
  Array getData() const {
    VisusAssert(VisusHasMessageLock()); return data;
  }

  //setData
  void setData(Array value,SharedPtr<Palette> palette= SharedPtr<Palette>());

  //getDataDimension
  int getDataDimension() const {
    VisusAssert(VisusHasMessageLock()); 
    return (data.getWidth() > 1 ? 1 : 0) + (data.getHeight() > 1 ? 1 : 0) + (data.getDepth() > 1 ? 1 : 0);
  }

  //getDataBounds 
  Position getDataBounds() {
    VisusAssert(VisusHasMessageLock()); 
    return (data.clipping.valid() ? data.clipping : data.bounds);
  }

  //getBounds 
  virtual Position getBounds() override {
    return getDataBounds();
  }

  //getLightingMaterial
  GLMaterial getLightingMaterial() {
    return lighting_material;
  }

  //setLightingMaterial
  void setLightingMaterial(GLMaterial value) {
    setEncodedProperty("SetLightingMaterial", this->lighting_material, value);
  }

  //lightingEnabled
  bool lightingEnabled() const {
    return lighting_enabled;
  }

  //setLightingEnabled
  void setLightingEnabled(bool value) {
    setProperty("SetLightingEnabled", this->lighting_enabled, value);
  }

  //getPalette
  SharedPtr<Palette> getPalette() const {
    VisusAssert(VisusHasMessageLock());
    return palette;
  }

  //paletteEnabled
  bool paletteEnabled() const {
    return palette_enabled;
  }

  //setPaletteEnabled
  void setPaletteEnabled(bool value) {
    setProperty("SetPaletteEnabled", this->palette_enabled, value);
  }

  //useViewDirection
  bool useViewDirection() const {
    return use_view_direction;
  }

  //setUseViewDirection
  void setUseViewDirection(bool value) {
    setProperty("SetUseViewDirection", this->use_view_direction, value);
  }

  //maxNumSlices
  int maxNumSlices() const {
    return max_num_slices;
  }

  //setMaxNumSlices
  void setMaxNumSlices(int value) {
    setProperty("SetMaxNumSlices", this->max_num_slices, value);
  }

  //minifyFilter
  int minifyFilter() const {
    return minify_filter;
  }

  //setMinifyFilter
  void setMinifyFilter(int value) {
    setProperty("SetMinifyFilter", this->minify_filter, value);
  }

  //magnifyFilter
  int magnifyFilter() const {
    return magnify_filter;
  }

  //setMagnifyFilter
  void setMagnifyFilter(int value) {
    setProperty("SetMagnifyFilter", this->magnify_filter, value);
  }

  //getRenderType
  String getRenderType() const {
    return render_type;
  }

  //setRenderType
  void setRenderType(String value);

  //glRender
  virtual void glRender(GLCanvas& gl) override;

  //processInput
  virtual bool processInput() override;

  //createEditor
  virtual void createEditor();

public:

  //shaders
  static void allocShaders();
  static void releaseShaders();

public:

  //execute
  virtual void execute(Archive& ar) override;

  //write
  virtual void write(Archive& ar) const override;

  //read
  virtual void read(Archive& ar) override;

private:

  SharedPtr<ReturnReceipt>    return_receipt;

  Array                       data;
  SharedPtr<Palette>          palette;

  GLMaterial                  lighting_material;
  bool                        lighting_enabled=false;
  bool                        palette_enabled=false;
  bool                        use_view_direction=false;
  int                         max_num_slices=0;
  int                         minify_filter=GL_LINEAR;
  int                         magnify_filter=GL_LINEAR;
  String                      render_type; // '' | 'gl' | 'ospray'

}; //end class




} //namespace Visus

#endif //VISUS_RENDER_ARRAYNODE_H

