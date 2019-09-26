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
void Field::writeToObjectStream(ObjectStream& ostream)
{
  ostream.writeValue("name",name);

  if (!description.empty())
    ostream.writeValue("description",description);

  ostream.pushContext("dtype");
  dtype.writeToObjectStream(ostream); 
  ostream.popContext("dtype");

  if (!index.empty())
   ostream.writeValue("index",index);

  if (!default_compression.empty())
   ostream.writeValue("default_compression",default_compression);

  if (!default_layout.empty())
    ostream.writeValue("default_layout",default_layout);

  if (default_value!=0)
    ostream.writeValue("default_value",cstring(default_value));

  if (!filter.empty())
    ostream.writeValue("filter",filter);

  //params
  if (!params.empty())
  {
    ostream.pushContext("params");
    {
      for (auto it=params.begin();it!=params.end();it++)
        ostream.writeValue(it->first,it->second);
    }
    ostream.popContext("params");
  }
}

////////////////////////////////////////////////////
void Field::readFromObjectStream(ObjectStream& istream)
{
  this->name=istream.readValue("name");
  this->description=istream.readValue("description");

  istream.pushContext("dtype");
  this->dtype.readFromObjectStream(istream); 
  istream.popContext("dtype");

  this->index=cint(istream.readValue("index"));
  this->default_compression=istream.readValue("default_compression");
  this->default_layout=istream.readValue("default_layout");
  this->default_value=cint(istream.readValue("default_value","0"));
  this->filter=istream.readValue("filter");

  this->params.clear();
  if (istream.pushContext("params"))
  {
    for (auto child : istream.getCurrentContext()->childs)
    {
      if (child->isHashNode())
        continue;

      String key=child->name;
      String value=istream.readValue(key);
      params.setValue(key,value);
    }
    istream.popContext("params");
  }

}

} //namespace Visus 

