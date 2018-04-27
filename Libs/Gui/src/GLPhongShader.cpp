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

#include <Visus/GLPhongShader.h>
#include <Visus/GLCanvas.h>

namespace Visus {

///////////////////////////////////////////////
int GLPhongShader::Config::getId() const
{
  VisusAssert(valid());
  int ret=0,shift=0;
  ret|=(lighting_enabled            ?1:0)<<shift++;  
  ret|=(color_attribute_enabled     ?1:0)<<shift++; 
  ret|=(clippingbox_enabled         ?1:0)<<shift++; 
  ret|=(texture_enabled             ?1:0)<<shift++;
  return ret;
}

///////////////////////////////////////////////
GLPhongShader::GLPhongShader(const Config& config_) : 
  GLShader(":/GLPhongShader.glsl"),
  config(config_)
{
  addDefine("CLIPPINGBOX_ENABLED"     ,cstring(config.clippingbox_enabled       ?1:0));
  addDefine("LIGHTING_ENABLED"        ,cstring(config.lighting_enabled          ?1:0));
  addDefine("COLOR_ATTRIBUTE_ENABLED" ,cstring(config.color_attribute_enabled   ?1:0));
  addDefine("TEXTURE_ENABLED"         ,cstring(config.texture_enabled           ?1:0));  

  u_color=addUniform("u_color"   );
  u_sampler=addSampler("u_sampler");
}

///////////////////////////////////////////////
GLPhongShader::~GLPhongShader(){
}


///////////////////////////////////////////////
void GLPhongShader::setUniformColor(GLCanvas& gl,const Color& color) {
  gl.setUniformColor(u_color,color);
}

void GLPhongShader::setTexture(GLCanvas& gl,SharedPtr<GLTexture> texture) {
  if (!texture) return;
  VisusAssert(config.texture_enabled);
  gl.setTexture(0,u_sampler,texture);
}

} //namespace

