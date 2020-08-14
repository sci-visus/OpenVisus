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
#include <Visus/IdxFile.h>

namespace Visus {


//////////////////////////////////////////////////////////////
void GoogleMapsDataset::readDatasetFromArchive(Archive& ar)
{
  //example: 22 levels, each tile has resolution 256*256 (==8bit*8bit)
  // bitsperblock=16
  // bitmask will be (22+8==30 '0' and 22+8==30 '1') 
  // V010101010101010101010101010101010101010101010101010101010101

  ar.read("nlevels", this->nlevels, this->nlevels);
  ar.read("tile_width", this->tile_width, this->tile_width);
  ar.read("tile_height", this->tile_height, this->tile_height);
  ar.read("tiles", this->tiles_url, this->tiles_url);

  if (!ar.getChild("idxfile"))
  {
    IdxFile idxfile;
    auto W = tile_width  * (((Int64)1) << nlevels);
    auto H = tile_height * (((Int64)1) << nlevels);
    idxfile.logic_box = BoxNi(PointNi(0, 0), PointNi(W, H));
    idxfile.bitmask = DatasetBitmask::guess('F', PointNi(W, H));
    idxfile.bitsperblock = Utils::getLog2(Int64(tile_width) * tile_height);
    idxfile.blocksperfile = 1;
    idxfile.fields.push_back(Field::fromString("DATA uint8[3] layout(rowmajor) compression(jpg)"));
    idxfile.missing_blocks = false;

    ar.writeObject("idxfile", idxfile);
  }

  Dataset::readDatasetFromArchive(ar);
}

} //namespace Visus

