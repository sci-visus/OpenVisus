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

#include <Visus/Field.h>

namespace Visus {

////////////////////////////////////////////////////
void Field::write(Archive& ar) const
{
  ar.write("name", name);
  ar.write("description", description);
  ar.write("index", index);
  ar.write("default_compression", default_compression);
  ar.write("default_layout", default_layout);
  ar.write("default_value", default_value);
  ar.write("filter", filter);
  ar.write("dtype", dtype);

  //params
  if (!params.empty())
  {
    auto params = ar.addChild("params");
    for (auto it : this->params)
      params->write(it.first, it.second);
  }
}

////////////////////////////////////////////////////
void Field::read(Archive& ar)
{
  ar.read("name", name);
  ar.read("description", description);
  ar.read("index", index);
  ar.read("default_compression", default_compression);
  ar.read("default_layout", default_layout);
  ar.read("default_value", default_value);
  ar.read("filter", filter);
  ar.read("dtype", dtype);

  this->params.clear();
  if (auto params= ar.getChild("params"))
  {
    for (auto param : params->childs)
    {
      if (param->isHash()) continue;
      String key = param->name;
      String value;
      param->read(key,value);
      this->params.setValue(key,value);
    }
  }

}

} //namespace Visus 

