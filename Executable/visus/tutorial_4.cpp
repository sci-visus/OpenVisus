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

using namespace Visus;

////////////////////////////////////////////////////////////////////////////////////
//this example show how to add samples dynamically in specific regions
////////////////////////////////////////////////////////////////////////////////////
void Tutorial_4(String default_layout)
{
  IdxFile idxfile;

  //each sample is made of an unsigned int of 8 bit (i.e. unsigned char)
  {
    Field field("field",DTypes::UINT8);
    field.default_compression="zip";
    field.default_layout=default_layout;
    idxfile.fields.push_back(field);
  }

  {
    Field field("bitmask",DTypes::UINT1);
    field.default_compression="zip";
    field.default_layout=default_layout;
    idxfile.fields.push_back(field);
  }

  idxfile.blocksperfile=8;

  //the box has dimension (8x4) which can grow in Y dinamically
  idxfile.box=NdBox(NdPoint(0,0),NdPoint::one(8,4));

  //this is the bitmask I will use (NOTE: the regular expression {1}*, the data can grow in Y)
  idxfile.bitmask=DatasetBitmask("V01010{1}*");
  
  //all the 5 bits without the regex will be stored in a single block
  idxfile.bitsperblock=5;

  VisusReleaseAssert(idxfile.save("temp/tutorial_4.idx"));

  //create the Dataset, since I'm using a bitmask with regular expression, I need to use VisusCreateEx
  auto dataset=Dataset::loadDataset("temp/tutorial_4.idx");
  VisusReleaseAssert(dataset && dataset->valid());
  Field field=dataset->getDefaultField();

  //write same samples in the box (8x4), i.e. do not use the regex {1}*
  {
    auto access=dataset->createAccess();

    NdBox write_box=dataset->getBox();

    Uint8 wbuffer[8*4]=
    {
       0, 1, 2, 3, 4, 5, 6, 7,
       8, 9,10,11,12,13,14,15,
      16,17,18,19,20,21,22,23,
      24,25,26,27,28,29,30,31
    };

    Uint8 wbitmask[]={255,255,255,255}; //all full

    //write field
    auto write_field=std::make_shared<Query>(dataset.get(),'w');
    write_field->field=dataset->getFieldByName("field");
    write_field->position=write_box;
    VisusReleaseAssert(dataset->beginQuery(write_field));
    VisusReleaseAssert(write_field->nsamples.innerProduct()==8*4);
    VisusReleaseAssert(write_field->field.dtype.getByteSize(write_field->nsamples)==sizeof(wbuffer));
    write_field->buffer=Array(write_field->nsamples,DTypes::UINT8,HeapMemory::createUnmanaged(wbuffer,sizeof(wbuffer)));
    VisusReleaseAssert(dataset->executeQuery(access,write_field));

    //write bitmask
    auto write_bitmask=std::make_shared<Query>(dataset.get(),'w');
    write_bitmask->field=dataset->getFieldByName("bitmask");
    write_bitmask->position=write_box;
    VisusReleaseAssert(dataset->beginQuery(write_bitmask));
    VisusReleaseAssert(write_bitmask->nsamples.innerProduct()==8*4);
    VisusReleaseAssert(write_bitmask->field.dtype.getByteSize(write_bitmask->nsamples)==sizeof(wbitmask));
    write_bitmask->buffer=Array(write_bitmask->nsamples,DTypes::UINT1,HeapMemory::createUnmanaged(wbitmask,sizeof(wbitmask)));
    VisusReleaseAssert(dataset->executeQuery(access,write_bitmask));

  }

  //now I have the following data on disk (all full)
  /*
  field

    000  001  002  003  004  005  006  007
    008  009  010  011  012  013  014  015 
    016  017  018  019  020  021  022  023
    024  025  026  027  028  029  030  031

  bitmask

    1  1  1  1  1  1  1  1
    1  1  1  1  1  1  1  1 
    1  1  1  1  1  1  1  1
    1  1  1  1  1  1  1  1
  
  */

  //add some details to the Dataset using the first regex level from {1}*
  //the "traditional" box had dimension (8*4), now adding a level in Y the "virtual" box will have dimension (8*8)
  {
    auto access=dataset->createAccess();

    //this is the region where I'm adding more detals (the box is expressed in the region of the "virtual" box)
    NdBox fine_box(NdPoint(4,3),NdPoint::one(8,8));

    //I'm using 1 bit more from the regex of the bitmask "V01010{1}*"
    int MaxH=dataset->getMaxResolution()+1;

    Uint8 wbuffer[4*5]=
    {
      100,101,102,103,
      104,105,106,107,
      108,109,110,111,
      112,113,114,115,
      116,117,118,119
    };

    Uint8 wbitmask[]={255,255,255};

    //write field
    auto write_field=std::make_shared<Query>(dataset.get(),'w');
    write_field->field=dataset->getFieldByName("field");
    write_field->position=fine_box;
    write_field->end_resolutions={MaxH};
    write_field->max_resolution=MaxH;
    VisusReleaseAssert(dataset->beginQuery(write_field));
    VisusReleaseAssert(write_field->nsamples.innerProduct()==4*5);
    VisusReleaseAssert(write_field->field.dtype.getByteSize(write_field->nsamples)==sizeof(wbuffer));
    write_field->buffer=Array(write_field->nsamples,DTypes::UINT8,HeapMemory::createUnmanaged(wbuffer,sizeof(wbuffer)));
    VisusReleaseAssert(dataset->executeQuery(access,write_field));

    //write the bitmask
    auto write_bitmask=std::make_shared<Query>(dataset.get(),'w');
    write_bitmask->field=dataset->getFieldByName("bitmask");
    write_bitmask->position=fine_box;
    write_bitmask->end_resolutions={MaxH};
    write_bitmask->max_resolution=MaxH;
    VisusReleaseAssert(dataset->beginQuery(write_bitmask));
    VisusReleaseAssert(write_bitmask->nsamples.innerProduct()==4*5);
    VisusReleaseAssert(write_bitmask->field.dtype.getByteSize(write_bitmask->nsamples)==sizeof(wbitmask));
    write_bitmask->buffer=Array(write_bitmask->nsamples,DTypes::UINT1,HeapMemory::createUnmanaged(wbitmask,sizeof(wbitmask)));
    VisusReleaseAssert(dataset->executeQuery(access,write_bitmask));
  }

  //now I have the following data on disk (I'm showing only where bitmask is 1)
  /*

           0    1    2    3    4    5    6    7
         ---------------***********************
   0 |   000  001  002  003  004  005  006  007
   1 |   
   2 *   008  009  010  011  012  013  014  015 
   3 *                       100  101  102  103
   4 *   016  017  018  019  104  105  106  107
   5 *                       108  109  110  111
   6 *   024  025  026  027  112  113  114  115
   7 |                       116  117  118  119

  */

  //Now I want to read samples from disk, first I read all samples in the traditional bitmask (i.e. not using the regular expression)....
  {
    auto access=dataset->createAccess();

    //I want to use one more bit from the bitmask regular expression
    int MaxH=dataset->getMaxResolution()+1;

    //P1 is equivalent to (3,1,0) with one less Y level
    //P2 is equivalent to (7,3,0) with one less Y level
    NdBox box(NdPoint(3,2),NdPoint::one(8,7));

    //read all samples without the regex
    auto read_field=std::make_shared<Query>(dataset.get(),'r');
    read_field->field=dataset->getFieldByName("field");
    read_field->position=box;
    read_field->max_resolution=MaxH;
    read_field->end_resolutions={MaxH-1,MaxH};
    read_field->merge_mode=Query::InterpolateSamples;//since the fine resolution is not everywhere I need to use the bInterpolate=true in merge
    VisusReleaseAssert(dataset->beginQuery(read_field));
    VisusReleaseAssert(read_field->nsamples.innerProduct()==5*3);
    VisusReleaseAssert(dataset->executeQuery(access,read_field));
    VisusReleaseAssert(dataset->nextQuery(read_field));
    VisusReleaseAssert(read_field->nsamples.innerProduct()==5*5);

    auto read_bitmask=std::make_shared<Query>(dataset.get(),'r');
    read_bitmask->field=dataset->getFieldByName("bitmask");
    read_bitmask->position=box;
    read_bitmask->max_resolution=MaxH;
    read_bitmask->end_resolutions={MaxH-1,MaxH};
    read_bitmask->merge_mode=Query::InterpolateSamples;//since the fine resolution is not everywhere I need to use the bInterpolate=true in merge
    ;
    VisusReleaseAssert(dataset->beginQuery(read_bitmask));
    VisusReleaseAssert(read_bitmask->nsamples.innerProduct()==5*3);
    VisusReleaseAssert(dataset->executeQuery(access,read_bitmask));
    VisusReleaseAssert(dataset->nextQuery(read_bitmask));
    VisusReleaseAssert(read_bitmask->nsamples.innerProduct()==5*5);

    //check that interpolation works...
    {
      unsigned char check_field_buffer[5*5]=
      {
         11,   12,   13,   14,   15, 
         11,   12,   13,   14,   15, 
         19,  104,  105,  106,  107,
         19,  104,  105,  106,  107,   
         27,  112,  113,  114,  115
      };
      Array check_field(5,5,DTypes::UINT8,SharedPtr<HeapMemory>(HeapMemory::createUnmanaged(check_field_buffer,sizeof(check_field_buffer))));
      VisusReleaseAssert(CompareSamples(read_field->buffer,0,check_field,0,25))
    }

    //insert the new samples...
    VisusReleaseAssert(dataset->executeQuery(access,read_field));
    VisusReleaseAssert(dataset->executeQuery(access,read_bitmask));

    //check the data is correct
    {
      unsigned char check[5*5]=
      {
         11,   12,   13,   14,   15,
         11,  100,  101,  102,  103,
         19,  104,  105,  106,  107,
         19,  108,  109,  110,  111,
         27,  112,  113,  114,  115
      };

      //BROKEN
      //
      //VisusReleaseAssert(CompareSamples(*read_fine_field->data,0,Array(check,5,5,DTypes::UINT8),0,25));
    }
  }

  if (auto vf=std::dynamic_pointer_cast<IdxDataset>(dataset))
    vf->removeFiles(dataset->getMaxResolution()+1);
}


