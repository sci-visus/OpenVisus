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

#include <Visus/GLMaterial.h>

namespace Visus {

///////////////////////////////////////////////
GLMaterial GLMaterial::createRandom() 
{
  GLMaterial ret;

  ret.front.ambient=Color(0.1f, 0.1f, 0.1f, 1.0f);
  ret.front.diffuse=Color::random();
  ret.front.specular=Color(0.6f, 0.6f, 0.6f, 1.0f);
  ret.front.emission=Colors::Black;
  ret.front.shininess=10;

  ret.back.ambient=Color(0.1f, 0.1f, 0.1f, 1.0f);
  ret.back.diffuse=(ret.front.diffuse*0.5).withAlpha(1.0f);
  ret.back.specular=Color(0.6f, 0.6f, 0.6f, 1.0f);
  ret.back.emission=Colors::Black;
  ret.back.shininess=10;

  return ret;
}



///////////////////////////////////////////////
void GLMaterial::write(Archive& ar) const
{
  ar.writeObject("front", front);
  ar.writeObject("back", back);
}

///////////////////////////////////////////////
void GLMaterial::read(Archive& ar)
{
  ar.readObject("front", front);
  ar.readObject("back", back);
}

} //namespace

