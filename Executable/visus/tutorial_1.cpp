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
#include <Visus/Encoder.h>

using namespace Visus;

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//TUTORIAL_1 create a dataset of dimension (16,16,16) and write some data
//////////////////////////////////////////////////////////////////////////////////////////////////////////
void Tutorial_1(String default_layout)
{
  //each sample is made of one uint32 (so a sample has 1*32 bits=32 bits)
  //each atomic data is a 32 bit unsigned integer 
  //valid dtypes are in the format [u](int|float)nbits
  //I want to write data on disk compressed using zlib
  //possible flag values are [compressed]  (space separated!)
  //IMPORTANT you can have a multifield file if you want. For example
  //"MY_SCALAR_FIELD 1*uint8 compressed + MY_VECTORIAL_FIELD 3*float32 "
  //in this case, when you read or write, specify the field you want Dataset::field/Dataset::field

  String filename="temp/tutorial_1.idx";

  //the data will be in the bounding box  p1(0,0,0) p2(16,16,16) (p1 included, p2 excluded) 
  {
    IdxFile idxfile;
    idxfile.logic_box=BoxNi(PointNi(0,0,0),PointNi(16,16,16));
    {
      Field field("myfield",DTypes::UINT32);
      field.default_compression= "lz4";
      field.default_layout=default_layout;
      idxfile.fields.push_back(field);
    }
    VisusReleaseAssert(idxfile.save(filename));
  }

  //now create a Dataset, save it and reopen from disk
  auto dataset=LoadDataset(filename);
  VisusReleaseAssert(dataset && dataset->valid());

  //any time you need to read/write data from/to a Dataset I need a Access
  auto access=dataset->createAccess();

  //for example I want to write data by slices 
  int cont=0;
  for (int nslice=0;nslice<16;nslice++)
  {
    //this is the bounding box of the region I'm going to write
    BoxNi slice_box=dataset->getLogicBox().getZSlab(nslice,nslice+1);

    //prepare the write query
    auto query=std::make_shared<BoxQuery>(dataset.get(), dataset->getDefaultField(), dataset->getDefaultTime(), 'w');
    query->logic_position=slice_box;
    VisusReleaseAssert(dataset->beginQuery(query));
    VisusReleaseAssert(query->nsamples.innerProduct()==16*16);

    //fill the buffers
    Array buffer(query->nsamples,query->field.dtype);
    unsigned int* Dst=(unsigned int*)buffer.c_ptr();
    for (int I=0;I<16*16;I++) *Dst++=cont++;
    query->buffer=buffer;

    //execute the writing
    VisusReleaseAssert(dataset->executeQuery(access,query));
  }
}
