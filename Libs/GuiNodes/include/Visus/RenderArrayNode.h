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

#include <Visus/GuiNodes.h>
#include <Visus/DataflowNode.h>
#include <Visus/TransferFunction.h>

#include <Visus/GLCanvas.h>

namespace Visus {


/////////////////////////////////////////////////////////////////////////////
class VISUS_GUI_NODES_API RenderArrayNodeShader : public GLShader
{
public:

  VISUS_NON_COPYABLE_CLASS(RenderArrayNodeShader)

  VISUS_DECLARE_SHADER_CLASS(VISUS_GUI_NODES_API, RenderArrayNodeShader)

  //___________________________________________________________
  class Config
  {
  public:

    int  texture_dim=0;       //2 or 3
    int  texture_nchannels=0; //(1) luminance (2) luminance+alpha (3) rgb (4) rgba
    bool clippingbox_enabled=false;
    bool palette_enabled = false;
    bool lighting_enabled = false;
    bool discard_if_zero_alpha = false;

    //constructor
    Config() {}

    //valid
    bool valid() const {
      return (texture_dim == 2 || texture_dim == 3) && (texture_nchannels >= 1 && texture_nchannels <= 4);
    }

    //getId
    int getId() const
    {
      VisusAssert(valid());
      int ret = 0, shift = 0;
      ret |= (texture_dim) << shift; shift += 2;
      ret |= (texture_nchannels) << shift; shift += 3;
      ret |= (clippingbox_enabled ? 1 : 0) << shift++;
      ret |= (palette_enabled ? 1 : 0) << shift++;
      ret |= (lighting_enabled ? 1 : 0) << shift++;
      ret |= (discard_if_zero_alpha ? 1 : 0) << shift++;
      return ret;
    }

  };

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
    return Shaders::getSingleton()->get(config.getId(), config);
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

  ////////////////////////////////////////////////////////////////////
class VISUS_GUI_NODES_API RenderArrayNode : 
  public Node,
  public GLObject 
{
public:

  VISUS_NON_COPYABLE_CLASS(RenderArrayNode)

  //(run time) fast rendering
  bool bFastRendering=false;

  //(run time) opacity
  double opacity = 1.0;

  //constructor
  RenderArrayNode(String name="");

  //destructor
  virtual ~RenderArrayNode();

  //getData
  Array getData() const {
    VisusAssert(VisusHasMessageLock()); return data;
  }

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

  //getNodeBounds 
  virtual Position getNodeBounds() override {
    return getDataBounds();
  }

  //getLightingMaterial
  GLMaterial& getLightingMaterial() {
    return lighting_material;
  }

  //lightingEnabled
  bool lightingEnabled() const {
    return lighting_enabled;
  }

  //setLightingEnabled
  void setLightingEnabled(bool value) {
    if (lighting_enabled == value) return;
    beginUpdate();
    this->lighting_enabled = value;
    endUpdate();
  }

  //paletteEnabled
  bool paletteEnabled() const {
    return palette_enabled;
  }

  //setPaletteEnabled
  void setPaletteEnabled(bool value) {
    if (palette_enabled == value) return;
    beginUpdate();
    this->palette_enabled = value;
    endUpdate();
  }

  //useViewDirection
  bool useViewDirection() const {
    return use_view_direction;
  }

  //setUseViewDirection
  void setUseViewDirection(bool value) {
    if (use_view_direction == value) return;
    beginUpdate();
    this->use_view_direction = value;
    endUpdate();
  }

  //maxNumSlices
  int maxNumSlices() const {
    return max_num_slices;
  }

  //setMaxNumSlices
  void setMaxNumSlices(int value) {
    if (max_num_slices == value) return;
    beginUpdate();
    this->max_num_slices = value;
    endUpdate();
  }

  //minifyFilter
  int minifyFilter() const {
    return minify_filter;
  }

  //setMinifyFilter
  void setMinifyFilter(int value) {
    if (minify_filter == value) return;
    beginUpdate();
    this->minify_filter = value;
    endUpdate();
  }

  //magnifyFilter
  int magnifyFilter() const {
    return magnify_filter;
  }

  //setMagnifyFilter
  void setMagnifyFilter(int value) {
    if (magnify_filter == value) return;
    beginUpdate();
    this->magnify_filter = value;
    endUpdate();
  }

  //glRender
  virtual void glRender(GLCanvas& gl) override;

  //processInput
  virtual bool processInput() override;

public:

  //writeToObjectStream
  virtual void writeToObjectStream(ObjectStream& ostream) override;

  //readFromObjectStream
  virtual void readFromObjectStream(ObjectStream& istream) override;

private:

  class MyGLObject; friend class MyGLObject;

  SharedPtr<ReturnReceipt>    return_receipt;

  Array                       data;
  SharedPtr<GLTexture>        data_texture;

  SharedPtr<Palette>          palette;
  SharedPtr<GLTexture>        palette_texture;

  GLMaterial lighting_material;

  bool   lighting_enabled=false;
  bool   palette_enabled=false;
  bool   use_view_direction=false;

  int    max_num_slices=0;
  int    minify_filter=GL_LINEAR;
  int    magnify_filter=GL_LINEAR;


}; //end class


} //namespace Visus

#endif //VISUS_RENDER_ARRAYNODE_H

