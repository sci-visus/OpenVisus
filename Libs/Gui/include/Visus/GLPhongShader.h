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

#ifndef __VISUS_GL_PHONG_SHADER_H
#define __VISUS_GL_PHONG_SHADER_H

#include <Visus/Gui.h>
#include <Visus/GLShader.h>
#include <Visus/GLTexture.h>

namespace Visus {

class GLCanvas;


////////////////////////////////////////////////////////
class VISUS_GUI_API GLPhongShaderConfig
{
public:

  bool lighting_enabled;
  bool color_attribute_enabled;
  bool clippingbox_enabled;
  bool texture_enabled;

  //constructor
  GLPhongShaderConfig(bool lighting_enable_ = false, bool color_attribute_enabled = false, bool clippingbox_enabled = false, bool texture_enabled = false) :
    lighting_enabled(lighting_enable_), color_attribute_enabled(color_attribute_enabled), clippingbox_enabled(clippingbox_enabled), texture_enabled(texture_enabled){
  }

  //valid
  bool valid() const {
    return !(lighting_enabled && color_attribute_enabled);
  }

  //withXXXXEnabled
  GLPhongShaderConfig& withLightingEnabled(bool value = true) { lighting_enabled = value; return *this; }
  GLPhongShaderConfig& withColorAttributeEnabled(bool value = true) { color_attribute_enabled = value; return *this; }
  GLPhongShaderConfig& withClippingBoxEnabled(bool value = true) { clippingbox_enabled = value; return *this; }
  GLPhongShaderConfig& withTextureEnabled(bool value = true) { texture_enabled = value; return *this; }

  //operator< (for map)
  bool operator<(const GLPhongShaderConfig& other) const {
    return this->key() < other.key();
  }

private:

  //key
  std::tuple<bool, bool, bool, bool> key() const {
    return std::make_tuple(lighting_enabled, color_attribute_enabled, clippingbox_enabled, texture_enabled);
  }


};

////////////////////////////////////////////////////////
class VISUS_GUI_API GLPhongShader: public GLShader
{
public:

  VISUS_NON_COPYABLE_CLASS(GLPhongShader)

  typedef GLPhongShaderConfig Config;

  Config config;

  //destructor
  virtual ~GLPhongShader();

  //constructor
  static void allocShaders();

  //releaseShaders
  static void releaseShaders();

  //getSingleton
  static GLPhongShader* getSingleton(const Config& config = Config());

  //setUniformColor
  void setUniformColor(GLCanvas& gl,const Color& color);

  //setTexture
  void setTexture(GLCanvas& gl,SharedPtr<GLTexture> texture);

private:

  GLUniform   u_color;

  //config.texture_enabled
  GLSampler   u_sampler;

  static std::map< Config, GLPhongShader* > shaders;

  //constructor
  GLPhongShader(const Config& config);
  
};


} //namespace

#endif //__VISUS_GL_PHONG_SHADER_H