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

#include <Visus/GoogleMapsDataset.h>
#include <Visus/GoogleMapsAccess.h>

namespace Visus {


//////////////////////////////////////////////////////////////
void GoogleMapsDataset::readDatasetFromArchive(Archive& ar)
{
  String url = ar.readString("url");

  //example: 22 levels, each tile has resolution 256*256 (==8bit*8bit)
  //bitsperblock=16
  // bitmask will be (22+8==30 '0' and 22+8==30 '1') 
  //V010101010101010101010101010101010101010101010101010101010101

  const int nlevels = 22;
  const int tile_width = 256;
  const int tile_height = 256;
  ar.read("tiles", this->tiles_url, "http://mt1.google.com/vt/lyrs=s");

  auto W = tile_width  * (((Int64)1) << nlevels);
  auto H = tile_height * (((Int64)1) << nlevels);

  ar.write("kdquery", "UseBlockQuery");
  
  if (!ar.getChild("bitmask"))
  {
    auto value = DatasetBitmask::guess(PointNi(W, H));
    ar.addChild("bitmask")->write("value", value);
  }

  if (!ar.getChild("box"))
  {
    auto value = BoxNi(PointNi(0, 0), PointNi(W, H));
    ar.addChild("box")->write("value", value);
  }

  if (!ar.getChild("bitsperblock"))
  {
    auto value = Utils::getLog2(tile_width * tile_height);
    ar.addChild("bitsperblock")->write("value", value);
  }

  if (!ar.getChild("field"))
  {
    auto child = ar.addChild("field");
    child->setAttribute("name","DATA");
    child->setAttribute("dtype", "uint8[3]");
    child->setAttribute("default_compression", "jpg");
    child->setAttribute("default_layout", "rowmajor");
  }

  if (!ar.getChild("timestep"))
  {
    auto child = ar.addChild("timestep");
    child->setAttribute("when", "0");
  }

  Dataset::readDatasetFromArchive(ar);
}


} //namespace Visus

