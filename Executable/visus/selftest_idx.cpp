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

void Tutorial_1(String default_layout);
void Tutorial_2(String default_layout);
void Tutorial_3(String default_layout);
void Tutorial_4(String default_layout);
void Tutorial_6(String default_layout);

////////////////////////////////////////////////////////////////////////////////////
static NdBox GetRandomUserBox(int pdim,bool bFullBox)
{
  NdBox ndbox(NdPoint(pdim),NdPoint::one(pdim));

  for (int D=0;D<pdim;D++)
  {
    switch (pdim)
    {
      case 2:
        if (bFullBox)
        {
          //from 16 to 256
          ndbox.p2[D]=(((Int64)1)<<Utils::getRandInteger(4,8));
        }
        else
        {
          ndbox.p1[D]=Utils::getRandInteger(0                 , 254);
          ndbox.p2[D]=Utils::getRandInteger(2+(int)ndbox.p1[D], 256);
        }
        break;

      case 3:
        if (bFullBox)
        {
          //from 16 to 64
          ndbox.p2[D]=((Int64)1) <<Utils::getRandInteger(4,6);
        }
        else
        {
          ndbox.p1[D]=Utils::getRandInteger(0                 , 62);
          ndbox.p2[D]=Utils::getRandInteger(2+(int)ndbox.p1[D], 64);
        }
        break;

      case 4:
        if (bFullBox)
        {
          //from 16 to 32
          ndbox.p2[D]=((Int64)1) <<Utils::getRandInteger(4,5);
        }
        else
        {
          ndbox.p1[D]=Utils::getRandInteger(0                 , 30);
          ndbox.p2[D]=Utils::getRandInteger(2+(int)ndbox.p1[D], 32);
        }
        break;

      case 5:
        if (bFullBox)
        {
          ndbox.p2[D]=16;
        }
        else
        {
          ndbox.p1[D]=Utils::getRandInteger(0                 , 14);
          ndbox.p2[D]=Utils::getRandInteger(2+(int)ndbox.p1[D], 16);
        }
        break;
    }
  }
  return ndbox;
}

////////////////////////////////////////////////////////////////////////////////////
static DType GetRandomDType()
{
  bool byte_aligned=            (Utils::getRandInteger(0,1)?true:false);
  bool decimal     =            (Utils::getRandInteger(0,1)?true:false);
  bool unsign      =!decimal && (Utils::getRandInteger(0,1)?true:false);
  int  bitsize     =byte_aligned?(8*Utils::getRandInteger(1,4)):(Utils::getRandInteger(1,64));
  return DType(unsign,decimal,bitsize);
}



////////////////////////////////////////////////////////////////////////////////////
class SelfTest
{
public:

  //constructor (create random data per slice)
  SelfTest(IdxDataset* vf)
  {
    Field field=vf->getDefaultField();
    this->vf=vf;
    this->dtype=field.dtype;
    this->user_box=vf->getBox();
    this->pdim=vf->getPointDim();
    this->nslices=user_box.p2[pdim-1]-user_box.p1[pdim-1];
  
    //calculate number of samples per slice
    this->perslice=1;
    NdPoint::coord_t _stride=1;
    this->stride = NdPoint(pdim);
    for (int D=0;D<(pdim-1);D++)
    {
      NdPoint::coord_t num=user_box.p2[D]-user_box.p1[D];
      this->perslice*=num;
      stride[D]=_stride;
      _stride*=num;
    }

    VisusInfo()<<"Starting self test procedure on dataset field("<<field.name<<") pdim("<<vf->getPointDim()<<")";

    if (bool bWriteData=true)
    {
      auto access=vf->createAccess();

      int cont=0;
      for (int N=0;N<nslices;N++)
      {
        NdBox slice_box=getSliceBox(N);

        auto query=std::make_shared<Query>(vf,'w');
        query->field=field;
        query->position=slice_box;
        VisusReleaseAssert(vf->beginQuery(query));

        Array buffer(query->nsamples,field.dtype);
        for (int i=0;i<buffer.c_size();i++)
          buffer.c_ptr()[i]=cont++;
        query->buffer=buffer;

        VisusReleaseAssert(vf->executeQuery(access,query));
        this->write_queries.push_back(query);
      }
    }

    if (bool bVerifyData=true)
    {
      auto access=vf->createAccess();

      for (int N=0;N<this->nslices;N++)
      {
        auto read_slice=std::make_shared<Query>(vf,'r');
        read_slice->position=(getSliceBox(N));
        VisusReleaseAssert(vf->beginQuery(read_slice));
        VisusReleaseAssert(vf->executeQuery(access,read_slice));
        VisusReleaseAssert(read_slice->nsamples.innerProduct()==this->perslice);
        VisusReleaseAssert(CompareSamples(write_queries[N]->buffer,0,read_slice->buffer,0,perslice));
        read_slice.reset();
      }
    }
  }

  //execute a random query
  void executeRandomQuery()
  {
    Field                field         = vf->getDefaultField();
    int                  maxh          = vf->getMaxResolution();
    int                  firsth        = Utils::getRandInteger(0,maxh);
    int                  lasth         = Utils::getRandInteger(firsth,maxh);
    int                  deltah        = firsth==lasth?1:Utils::getRandInteger(1,lasth-firsth);
    NdBox                box           = Utils::getRandInteger(0,1)?vf->getBitmask().upgradeBox(vf->getBox(),maxh):getRandomBox();
    bool                 bInterpolate  = Utils::getRandInteger(0,1)?true:false;
    
    static int nactivation=0;
    nactivation++;

    auto access=vf->createAccess();

    auto query=std::make_shared<Query>(vf,'r');
    query->position=box;
    query->merge_mode=(bInterpolate?Query::InterpolateSamples : Query::InsertSamples);

    for (int h=firsth;h<=lasth;h=h+deltah)
      query->end_resolutions.push_back(h);

    Array buffer;
    NdBox h_box;
    NdPoint shift(pdim);

    //probably the bounding box cannot get samples
    if (!vf->beginQuery(query))
      return;

    while (true)
    {
      VisusReleaseAssert(vf->executeQuery(access,query));
      buffer=query->buffer;
      h_box=query->logic_box;
      shift=query->logic_box.shift;
      if (!vf->nextQuery(query))
        break;
    }

    VisusReleaseAssert(buffer);
    {
      //verify written data
      int nsample=0;
      for (auto loc = ForEachPoint(buffer.dims); !loc.end(); loc.next())
      {
        NdPoint world_point=h_box.p1+((loc.pos).leftShift(shift));

        //number of slice
        int N=(int)(world_point[pdim-1]-user_box.p1[pdim-1]);

        //position inside the slice buffer
        LogicBox samples=write_queries[N]->logic_box;
        NdPoint P=samples.logicToPixel(world_point);
        Int64 pos=stride.dotProduct(P);
        VisusReleaseAssert(CompareSamples(this->write_queries[N]->buffer,pos,buffer,nsample,1));
        nsample++;
      }

       VisusInfo()<<"done query "<<
        "first_resolution("<<firsth<<") "
        <<"last_resolution("<<lasth<<") "
        <<"delta_between_resolution("<<deltah<<") "
        <<"merge_mode("<<(bInterpolate?"interpolate":"insert")<<")";
    }
  }

  //return a random box inside the user_box (to exec read query)
  NdBox getRandomBox()
  {
    NdBox ndbox(NdPoint(pdim),NdPoint::one(pdim));
    for (int D=0;D<pdim;D++)
    {
      ndbox.p1[D]=Utils::getRandInteger((int)user_box.p1[D],(int)user_box.p2[D]-1);
      ndbox.p2[D]=Utils::getRandInteger((int)ndbox.p1[D]+1 ,(int)user_box.p2[D]  );
    }
    return ndbox;
  }

  //getSliceBox
  NdBox getSliceBox(int N) const {
    auto z1 = (int)user_box.p1[pdim - 1] + N;
    auto z2 = z1 + 1;
    return user_box.getSlab(pdim - 1, z1, z2);
  }

protected:

  IdxDataset*           vf;
  DType                 dtype;
  NdPoint::coord_t      nslices;
  Int64                 perslice;
  int                   pdim;
  NdPoint               stride;
  NdBox                 user_box;
  std::vector< SharedPtr<Query> >   write_queries;

}; //end class 


/////////////////////////////////////////////////////
void execTestIdx(int max_seconds)
{
  Time t1=Time::now();

#if 1
  for (auto rowmajor : {false,true})
  {
    String default_layout=rowmajor?"rowmajor":"hzorder";

    VisusInfo()<<"Running Tutorial_1...";
    Tutorial_1(default_layout);
    VisusInfo()<<"...done";

    VisusInfo()<<"Running Tutorial_2...";
    Tutorial_2(default_layout);
    VisusInfo()<<"...done";

    VisusInfo()<<"Running Tutorial_3...";
    Tutorial_3(default_layout);
    VisusInfo()<<"...done";

    //remove data from tutorial_1
    if (auto vf= IdxDataset::loadDataset("./temp/tutorial_1.idx")) 
      vf->removeFiles();
 
    VisusInfo()<<"Running Tutorial_4...";
    Tutorial_4(default_layout);
    VisusInfo()<<"...done";

    VisusInfo()<<"Running Tutorial_6...";
    Tutorial_6(default_layout);
    VisusInfo()<<"...done";
  }
#endif

  ////do self testing on random field
  VisusInfo()<<"Running self test procedure (max_seconds "<<max_seconds<<")...";

  //test byte aligned query (the fast loop has special sample copy for byte aligned data)
  if (true)
  {
    for (auto rowmajor : {false,true})
    {
      for (int pdim=2;pdim<=5;pdim++)
      {
        for (int nbits=8;nbits<=64;nbits+=8)
        {
          NdBox user_box = GetRandomUserBox(pdim,Utils::getRandInteger(0,1)?true:false);

          IdxFile idxfile;
          idxfile.box=user_box;
          {
            Field field("myfield",DType(true,false,nbits));
            field.default_layout=rowmajor?"rowmajor":"hzorder";
            field.default_compression=Utils::getRandInteger(0,1)?"zip":"";
            idxfile.fields.push_back(field);
          }
          VisusReleaseAssert(idxfile.save("./temp/temp.idx"));

          auto vf=IdxDataset::loadDataset("./temp/temp.idx");
          VisusReleaseAssert(vf && vf->valid());

          {
            SelfTest selftest(vf.get());
            for (int n=0;n<10;n++) 
              selftest.executeRandomQuery();
          }

          vf->removeFiles();
        }
      }
    }
  }

  while (true)
  {
    if (max_seconds>0 && t1.elapsedSec()>max_seconds)
      break;

    //test 10 queries on a random field
    {
      int      pdim         = Utils::getRandInteger(2, 5);
      NdBox    user_box     = GetRandomUserBox(pdim,Utils::getRandInteger(0,1)?true:false);

      IdxFile idxfile;
      idxfile.box=user_box;
      {
        Field field("myfield",DType(Utils::getRandInteger(1,4),GetRandomDType()));
        field.default_layout=Utils::getRandInteger(0,1)?"rowmajor":"hzorder";
        field.default_compression=Utils::getRandInteger(0,1)?"zip":"";
        idxfile.fields.push_back(field);
      }

      String idxfilename="./temp/temp.idx";

      VisusReleaseAssert(idxfile.save(idxfilename));

      auto vf=IdxDataset::loadDataset(idxfilename);
      VisusReleaseAssert(vf && vf->valid());
      
      {
        SelfTest selftest(vf.get());
        for (int n=0;n<10;n++) 
          selftest.executeRandomQuery();
      }

      vf->removeFiles();
    }
  }


  #if WIN32 && _DEBUG
    {printf("Selftest OK. Press a char to finish\n");getchar();}
  #endif
}



