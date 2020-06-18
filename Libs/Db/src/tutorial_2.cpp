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


////////////////////////////////////////////////////////////////////////
//read data from tutorial 1
////////////////////////////////////////////////////////////////////////
void Tutorial_2(String default_layout)
{
  //read Dataset from tutorial 1
  auto dataset = LoadDataset("tmp/tutorial_1/visus.idx");

  BoxNi world_box = dataset->getLogicBox();

  int pdim = 3;

  //check the data has dimension (16,16,16)
  VisusReleaseAssert(dataset->getField().dtype == (DTypes::UINT32)
    && world_box.p1 == PointNi(0, 0, 0)
    && world_box.p2 == PointNi(16, 16, 16));

  //any time you need to read/write data from/to a Dataset I need a Access
  auto access = dataset->createAccess();

  int cont = 0;
  for (int nslice = 0; nslice < 16; nslice++)
  {
    //this is the bounding box of the region I want to read (i.e. a single slice)
    BoxNi slice_box = world_box.getZSlab(nslice, nslice + 1);

    //I should get a number of samples equals to the number of samples written in tutorial 1
    auto query = dataset->createBoxQuery(slice_box, 'r');
    dataset->beginBoxQuery(query);
    VisusReleaseAssert(query->isRunning());
    VisusReleaseAssert(query->getNumberOfSamples() == PointNi(16, 16, 1));

    //read data from disk
    VisusReleaseAssert(dataset->executeBoxQuery(access, query));
    VisusReleaseAssert(query->buffer.c_size() == sizeof(int) * 16 * 16);

    GetSamples<Int32> Src(query->buffer);
    for (int I = 0; I < 16 * 16; I++, cont++)
      VisusReleaseAssert(Src[I] == cont);
  }
}

} //namespace Visus