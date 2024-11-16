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

#include <Visus/IdxDataset.h>


namespace Visus {

////////////////////////////////////////////////////////////////////////////////////
//read specific region of Dataset from tutorial_1 "merging" partial results
////////////////////////////////////////////////////////////////////////////////////
void CppSamples_ReadIdxLevels(String default_layout)
{
  auto dataset = LoadDataset("tmp/tutorial_1/visus.idx");

  BoxNi world_box = dataset->getLogicBox();

  //any time you need to read/write data from/to a Dataset I need a Access
  auto access = dataset->createAccess();

  Field field = dataset->getField();

  //this is the maximum resolution of the Dataset 

  //in the bitmask "V012012012012" the very last bit of the bitmask is at position MaxH=12 
  VisusReleaseAssert(dataset->getMaxResolution() == 12);

  //I want to read data from first slice Z=0
  BoxNi slice_box = world_box.getZSlab(0, 1);

  //create and read data for resolutions [8,12] (12==MaxH which is the very last available on disk)
  auto query = dataset->createBoxQuery(slice_box, 'r');
  query->end_resolutions = { 8,12 };

  dataset->beginBoxQuery(query);
  VisusReleaseAssert(dataset->executeBoxQuery(access, query));
  VisusReleaseAssert(query->getCurrentResolution() == 8);

  dataset->nextBoxQuery(query);
  VisusReleaseAssert(dataset->executeBoxQuery(access, query));
  VisusReleaseAssert(query->getCurrentResolution() == 12);

  //I can verify the data is correct
  GetSamples<Uint32> SRC(query->buffer);
  for (int I = 0; I < 16 * 16; I++)
    VisusReleaseAssert(SRC[I] == I);
}


} //namespace Visus