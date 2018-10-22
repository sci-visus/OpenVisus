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


#ifndef VISUS_RENDER_KD_ARRAY_NODE_H
#define VISUS_RENDER_KD_ARRAY_NODE_H

#include <Visus/GuiNodes.h>
#include <Visus/DataflowNode.h>
#include <Visus/KdArray.h>
#include <Visus/TransferFunction.h>

#include <Visus/GLCanvas.h>

namespace Visus {


/////////////////////////////////////////////////////////////////////////////
class VISUS_GUI_NODES_API KdRenderArrayNodeShader : public GLShader
{
public:

  VISUS_NON_COPYABLE_CLASS(KdRenderArrayNodeShader)

  VISUS_DECLARE_SHADER_CLASS(VISUS_GUI_NODES_API, KdRenderArrayNodeShader)

  //_____________________________________________________________
  class Config
  {
  public:
    int  texture_dim=0;
    int  texture_nchannels=0; //(1) luminance (2) luminance+alpha (3) rgb (4) rgba
    bool clippingbox_enabled = false;
    bool palette_enabled = false;
    bool discard_if_zero_alpha = false;

    //constructor
    Config() {
    }

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
      ret |= (discard_if_zero_alpha ? 1 : 0) << shift++;
      return ret;
    }

  };

  Config config;

  //constructor
  KdRenderArrayNodeShader(const Config& config_) : GLShader(":/KdRenderArrayShader.glsl"), config(config_)
  {
    addDefine("CLIPPINGBOX_ENABLED", cstring(config.clippingbox_enabled ? 1 : 0));
    addDefine("TEXTURE_DIM", cstring(config.texture_dim));
    addDefine("TEXTURE_NCHANNELS", cstring(config.texture_nchannels));
    addDefine("PALETTE_ENABLED", cstring(config.palette_enabled ? 1 : 0));
    addDefine("DISCARD_IF_ZERO_ALPHA", cstring(config.discard_if_zero_alpha ? 1 : 0));

    u_sampler = addSampler("u_sampler");
    u_palette_sampler = addSampler("u_palette_sampler");
  }

  //destructor
  virtual ~KdRenderArrayNodeShader() {
  }

  //getSingleton
  static KdRenderArrayNodeShader* getSingleton(Config config) {
    return Shaders::getSingleton()->get(config.getId(), config);
  }

  //setTexture
  void setTexture(GLCanvas& gl, SharedPtr<GLTexture> value) {
    gl.setTexture(u_sampler, value);
  }

  //setPaletteTexture 
  void setPaletteTexture(GLCanvas& gl, SharedPtr<GLTexture> value) {
    VisusAssert(config.palette_enabled);
    gl.setTextureInSlot(1, u_palette_sampler, value);
  }

private:

  GLSampler u_sampler;
  GLSampler u_palette_sampler;
};


//////////////////////////////////////////////////////////////////////////////
//Receive a KdArray data and display it as it is
class VISUS_GUI_NODES_API KdRenderArrayNode : 
  public Node, 
  public GLObject 
{
public:

  VISUS_NON_COPYABLE_CLASS(KdRenderArrayNode)

  //constructor
  KdRenderArrayNode(String name="") : Node(name)
  {
    addInputPort("data");
    addInputPort("palette"); 
  }

  //destructor
  virtual ~KdRenderArrayNode() {
  }

  //getKdArray
  SharedPtr<KdArray> getKdArray() const
  {return kdarray;}

  //getNodeBounds
  virtual Position getNodeBounds() override
  {
    if (!kdarray) return Position::invalid();
    return (kdarray->clipping.valid())? kdarray->clipping : Position(kdarray->root->box).getBox();
  }

  //from Node
  virtual bool processInput() override;

  //glRender
  virtual void glRender(GLCanvas& gl) override;

private:

  SharedPtr<KdArray>    kdarray;

  SharedPtr<Palette>    palette;
  SharedPtr<GLTexture>  palette_texture;

}; //end class

} //namespace Visus

#endif //VISUS_RENDER_KD_ARRAY_NODE_H

