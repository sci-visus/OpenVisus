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

#include <Visus/Idx.h>
#include <Visus/IdxDataset.h>
#include <Visus/DatasetFilter.h>

namespace Visus   {
namespace Private {

///////////////////////////////////////////////////////////////////////////////
template <typename CppType,class FilterClass>
static bool ComputeFilter(Dataset* dataset,Query* query,const FilterClass* filter,bool bInverse)
{
  const Field& field=query->field;

  int H= query->cur_resolution;

  //nothing to do for very coarse resolution (H=0)
  if (H==0)
    return true;

  LogicBox         logic_box  = query->logic_box;
  DType            dtype      = field.dtype;
  int              ncomponents= dtype.ncomponents();
  DatasetBitmask   bitmask    = dataset->getBitmask();
  int              bit        = bitmask[H];
  PointNi          dims       = query->nsamples;
  PointNi          stride     = dims.stride();
  int              filter_size = filter->getSize();
  PointNi          filterstep = filter->getFilterStep(H);
  Int64 FILTERSTEP = filterstep[bit];
  BoxNi            filter_domain  = query->filter.domain;

  int pdim = bitmask.getPointDim();
  
  //cannot apply the filter, too few samples
  if (dims[bit]<filter_size) 
    return true; 

  //align again to filter (this is needed again for certain types of queries, such as query for visus blocks!)
  BoxNi box=logic_box;

  //important! take only the good "samples", i.e. I do not want to do any filtering with samples
  //outside the valid region of the dataset
  box= box.getIntersection(filter_domain);

  if (!box.isFullDim())
    return true;

  for (int D=0;D<pdim;D++) 
  {
    //what is the world step of the filter at the current resolution
    Int64 FILTERSTEP=filterstep[D];

    //means only one sample
    if (FILTERSTEP==1) 
      continue;

    //align the samples
    Int64 P1incl=Utils::alignLeft(box.p1[D]  ,(Int64)0,FILTERSTEP);
    Int64 P2incl=Utils::alignLeft(box.p2[D]-1,(Int64)0,FILTERSTEP);
    
    //if is the bit for the filter I need to be sure that all the filter window is available (see query-filter-explanation.gif)
    //i.e. is the window is made of 3 samples, the compute filter needs all 3 samples!
    if (D==bit) 
      P2incl+=FILTERSTEP - (FILTERSTEP/filter_size);

    //if the box is outside the bounding box of the data is stored in the query, fix it!
    if (P1incl< box.p1[D]) P1incl+=FILTERSTEP; VisusAssert(P1incl>=box.p1[D]);
    if (P2incl>=box.p2[D]) P2incl-=FILTERSTEP; VisusAssert(P2incl< box.p2[D]);

    box.p1[D]=P1incl;
    box.p2[D]=P2incl+logic_box.delta[D];
  }

  //invalid box obtained!
  if (!box.isFullDim())
    return true;

  PointNi from = logic_box.logicToPixel(box.p1);
  PointNi to   = logic_box.logicToPixel(box.p2);

  //see map... I can do this only because I know that filterstep is multiple of 2^query->shift
  PointNi step = filterstep.rightShift(logic_box.shift);

  //I'm going to to the for loop for the filter nested inside
  Int64 FROM   = from[bit]; 
  Int64 TO     = to  [bit]; to[bit  ]=FROM+1;
  Int64 STEP   = step[bit]; step[bit]=1;
  
  //need to take care of physical distance among samples
  //Example. FilterSize==3
  //
  //  sample0---samples1---sample2    sample3---sample4---sample5
  //  |         | 
  //  |---------| this is PHY_NEXT_SAMPLE
  //
  //  |-------------------------------|
  //  |                               | this is PHY_FILTER_WINDOW
  //
  Int64 PHY_FILTER_WINDOW  = STEP * ncomponents * stride[bit]; 
  Int64 PHY_NEXT_SAMPLE    = PHY_FILTER_WINDOW/filter_size;

  //todo!
  VisusAssert(filter_size==2);

  Uint8* buffer = query->buffer.c_ptr();

  for (auto loc = ForEachPoint(from, to, step); !loc.end(); loc.next())
  {
    if (query->aborted()) 
      return false;
    
    CppType* va = ((CppType*)buffer)+ncomponents*stride.dotProduct(loc.pos);
    CppType* vb = va + PHY_NEXT_SAMPLE; 
    //CppType* vc=vb+PHY_NEXT_SAMPLE;
    //CppType* vd=vc+PHY_NEXT_SAMPLE;

    for (Int64 LOC=FROM;LOC<TO;LOC+=STEP,va+=PHY_FILTER_WINDOW,vb+=PHY_FILTER_WINDOW)
    {
      if (bInverse)
        filter->applyInverse(va,vb);
      else
        filter->applyDirect(va,vb);
    }
  }

  return true;
}

/////////////////////////////////////////////////////////////////////////////
template <typename CppType>
class IdentityFilter: public DatasetFilter
{
public:

  //constructor
  IdentityFilter(Dataset* dataset,const Field& field) : DatasetFilter(dataset,field, /*filter_size*/2,"IdentityFilter")
  {}
  
  //destructor
  virtual ~IdentityFilter(){}

  //applyDirect
  inline void applyDirect(CppType* va,CppType* vb) const
  {}

  //applyInverse
  inline void applyInverse(CppType* va,CppType* vb) const
  {}

  //computeFilter
  virtual bool computeFilter(Query* query,bool bInverse) const override
  {return ComputeFilter<CppType>(getDataset(),query,this,bInverse);}

};

/////////////////////////////////////////////////////////////////////////////
template <typename CppType>
class MinFilter : public DatasetFilter
{
public:

  int ncomponents;

  //constructor
  MinFilter(Dataset* dataset,const Field& field) : DatasetFilter(dataset,field, /*filter_size*/2,"MinFilter")
  {
    setNeedExtraComponent(true); //always need an extra sample to store the SWAP info
    VisusAssert(getDType().ncomponents()>1);
    ncomponents=field.dtype.ncomponents();
    VisusAssert((ncomponents-1)<=sizeof(CppType)*8);
  }

  //destructor
  virtual ~MinFilter()
  {}

  //applyDirect
  inline void applyDirect(CppType* va,CppType* vb) const
  {

    CppType sign_a =(CppType)0;
    CppType sign_b =(CppType)0;
    for (int N=0;N<(ncomponents-1);N++)
    {
      CppType a   = va[N];
      CppType b   = vb[N];
      CppType m = std::min(a,b);
      CppType M = std::max(a,b);
      if (m!=a) 
        Utils::setBit((unsigned char*)&sign_b,N,1); //swap information
      va[N]=m; 
      vb[N]=M; 
    }
    va[ncomponents-1]=sign_a;
    vb[ncomponents-1]=sign_b;
  }

  //applyInverse
  inline void applyInverse(CppType* va,CppType* vb) const
  {
    CppType sign_a  = va[ncomponents-1];
    CppType sign_b  = vb[ncomponents-1];
    for (int N=0;N<(ncomponents-1);N++)
    {
      bool doSwap=Utils::getBit((unsigned char*)&sign_b,N)?true:false;
      CppType a = doSwap? vb[N] : va[N];
      CppType b = doSwap? va[N] : vb[N];
      va[N]=a; 
      vb[N]=b; 
    }
    va[ncomponents-1]=(CppType)0;
    vb[ncomponents-1]=(CppType)0;
  }

  //computeFilter
  virtual bool computeFilter(Query* query,bool bInverse) const override
  {return ComputeFilter<CppType>(getDataset(),query,this,bInverse);}

};

/////////////////////////////////////////////////////////////////////////////
template <typename CppType>
class MaxFilter : public DatasetFilter
{
public:

  //ncomponents
  int ncomponents;

  //constructor
  MaxFilter(Dataset* dataset,const Field& field) : DatasetFilter(dataset,field, /*filter_size*/2,"MaxFilter")
  {
    setNeedExtraComponent(true); //always need an extra sample to store the SWAP info
    VisusAssert(getDType().ncomponents()>1);
    ncomponents=field.dtype.ncomponents();
    VisusAssert((ncomponents-1)<=sizeof(CppType)*8);
  }

  //destructor
  virtual ~MaxFilter()
  {}

  //applyDirect
  inline void applyDirect(CppType* va,CppType* vb) const
  {

    CppType sign_a =(CppType)0;
    CppType sign_b =(CppType)0;
    for (int N=0;N<(ncomponents-1);N++)
    {
      CppType a   = va[N];
      CppType b   = vb[N];
      CppType m = std::min(a,b);
      CppType M = std::max(a,b);
      if (M!=a) 
        Utils::setBit((unsigned char*)&sign_b,N,1); //swap information
      va[N]=M; 
      vb[N]=m; 
    }
    va[ncomponents-1]=sign_a;
    vb[ncomponents-1]=sign_b;
  }

  //applyInverse
  inline void applyInverse(CppType* va,CppType* vb) const
  {
    CppType sign_a  = va[ncomponents-1];
    CppType sign_b  = vb[ncomponents-1];
    for (int N=0;N<(ncomponents-1);N++)
    {
      bool doSwap=Utils::getBit((unsigned char*)&sign_b,N)?true:false;
      CppType a = doSwap? vb[N] : va[N];
      CppType b = doSwap? va[N] : vb[N];
      va[N]=a; 
      vb[N]=b; 
    }
    va[ncomponents-1]=(CppType)0;
    vb[ncomponents-1]=(CppType)0;
  }

  //computeFilter
  virtual bool computeFilter(Query* query,bool bInverse) const override
  {return ComputeFilter<CppType>(getDataset(),query,this,bInverse);}

};

/////////////////////////////////////////////////////////////////////////////
template <typename CppType,typename R>
class DeHaarDiscreteFilter  : public DatasetFilter
{
public:

  //ncomponents
  int ncomponents;

  //constructor
  DeHaarDiscreteFilter(Dataset* dataset,const Field& field) : DatasetFilter(dataset,field,/*filter_size*/2,"DeHaarDiscreteFilter")
  {
    setNeedExtraComponent(true); //always need an extra sample to store the sign
    VisusAssert(getDType().ncomponents()>1); 
    ncomponents=field.dtype.ncomponents();
    VisusAssert((ncomponents-1)<=sizeof(CppType)*8);
    VisusAssert(sizeof(R)>=sizeof(CppType)); 
  }

  //destructor
  virtual ~DeHaarDiscreteFilter()
  {}

  //applyDirect
  inline void applyDirect(CppType* va,CppType* vb) const
  {

    CppType sign_a =(CppType)0;
    CppType sign_b =(CppType)0;
    for (int N=0;N<(ncomponents-1);N++)
    {
      R a = (R)va[N];
      R b = (R)vb[N];
      R low =(a+b)>>1;//this is a Nbit to Nbit
      R high=(a-b);   //this is a Nbit to (N+1) bit, I need to store the SIGN
      bool neg=high<0?true:false;
      if (neg) {high=-high; Utils::setBit((Uint8*)&sign_b,N,1);}
      va[N] =(CppType)low ; VisusAssert(va[N]==low );
      vb[N] =(CppType)high; VisusAssert(vb[N]==high);
    }
    va[ncomponents-1]=sign_a;
    vb[ncomponents-1]=sign_b;
  }

  //applyInverse
  inline void applyInverse(CppType* va,CppType* vb) const
  {
    CppType sign_a  = va[ncomponents-1];
    CppType sign_b  = vb[ncomponents-1];
    for (int N=0;N<(ncomponents-1);N++)
    {
      R low  = (R)va[N];
      R high = (R)vb[N];
      bool neg= Utils::getBit((Uint8*)&sign_b,N)?true:false;
      R a  =(((low<<1)+((high & 1)?1:0))+(neg?-high:high))>>1;
      R b  =(((low<<1)+((high & 1)?1:0))-(neg?-high:high))>>1;
      va[N]=(CppType)a; VisusAssert(va[N]==a);
      vb[N]=(CppType)b; VisusAssert(vb[N]==b);
    }
    va[ncomponents-1]=(CppType)0;
    va[ncomponents-1]=(CppType)0;
  }

  //computeFilter
  virtual bool computeFilter(Query* query,bool bInverse) const override
  {return ComputeFilter<CppType>(getDataset(),query,this,bInverse);}

};

/////////////////////////////////////////////////////////////////////////////
template <typename CppType,typename R>
class DeHaarContinuousFilter : public DatasetFilter
{
public:

  //ncomponents
  int ncomponents;

  //constructor
  DeHaarContinuousFilter(Dataset* dataset,const Field& field) : DatasetFilter(dataset,field,/*filter_size*/2,"DeHaarContinuousFilter")
  {
    VisusAssert(sizeof(R)>=sizeof(CppType));
    ncomponents=field.dtype.ncomponents();
  }
  
  //destructor
  virtual ~DeHaarContinuousFilter(){}


  //applyDirect
  inline void applyDirect(CppType* va,CppType* vb) const
  {
    for (int N=0;N<ncomponents;N++)
    {
      R a    = (R)va[N];
      R b    = (R)vb[N];
      R low  = ((R)0.5)*(a+b); 
      R high = ((R)0.5)*(a-b);
      va[N]=(CppType)low; 
      vb[N]=(CppType)high;
    }
  }

  //applyInverse
  inline void applyInverse(CppType* va,CppType* vb) const
  {
    for (int N=0;N<ncomponents;N++)
    {
      R low  = (R)va[N];
      R high = (R)vb[N];
      R a    = (low+high);
      R b    = (low-high);
      va[N]=(CppType)a;
      vb[N]=(CppType)b;
    }
  }

  //computeFilter
  virtual bool computeFilter(Query* query,bool bInverse) const override
  {return ComputeFilter<CppType>(getDataset(),query,this,bInverse);}

};

} //namespace private


/////////////////////////////////////////////////////////////////////////////////////
SharedPtr<DatasetFilter> IdxDataset::createQueryFilter(const Field& field) 
{
  String filter_name=field.filter;

  if (filter_name.empty())
    return SharedPtr<DatasetFilter>();

  if (filter_name=="identity" || filter_name=="IdentityFilter")
  {
    if (field.dtype.isVectorOf(DTypes::UINT8  )) return std::make_shared< Private::IdentityFilter<Uint8  > >(this,field);
    if (field.dtype.isVectorOf(DTypes::UINT16 )) return std::make_shared< Private::IdentityFilter<Uint16 > >(this,field);
    if (field.dtype.isVectorOf(DTypes::FLOAT32)) return std::make_shared< Private::IdentityFilter<Float32> >(this,field);
    if (field.dtype.isVectorOf(DTypes::FLOAT64)) return std::make_shared< Private::IdentityFilter<Float64> >(this,field);
  }

  if (filter_name=="min" || filter_name=="MinFilter")
  {
    if (field.dtype.isVectorOf(DTypes::UINT8  )) return std::make_shared< Private::MinFilter<Uint8  > >(this,field);
    if (field.dtype.isVectorOf(DTypes::UINT16 )) return std::make_shared< Private::MinFilter<Uint16 > >(this,field);
    if (field.dtype.isVectorOf(DTypes::FLOAT32)) return std::make_shared< Private::MinFilter<Float32> >(this,field);
    if (field.dtype.isVectorOf(DTypes::FLOAT64)) return std::make_shared< Private::MinFilter<Float64> >(this,field);
  }

  if (filter_name=="max" || filter_name=="MaxFilter")
  {
    if (field.dtype.isVectorOf(DTypes::UINT8  )) return std::make_shared< Private::MaxFilter<Uint8  > >(this,field);
    if (field.dtype.isVectorOf(DTypes::UINT16 )) return std::make_shared< Private::MaxFilter<Uint16 > >(this,field);
    if (field.dtype.isVectorOf(DTypes::FLOAT32)) return std::make_shared< Private::MaxFilter<Float32> >(this,field);
    if (field.dtype.isVectorOf(DTypes::FLOAT64)) return std::make_shared< Private::MaxFilter<Float64> >(this,field);
  }

  if (filter_name=="wavelet" || filter_name=="dehaar" || filter_name=="discretedehaar" || filter_name=="DeHaarDiscreteFilter")
  {
    if (field.dtype.isVectorOf(DTypes::UINT8 )) return std::make_shared< Private::DeHaarDiscreteFilter<Uint8 , Int32> >(this,field);
    if (field.dtype.isVectorOf(DTypes::UINT16)) return std::make_shared< Private::DeHaarDiscreteFilter<Uint16, Int32> >(this,field);
  }
  
  if (filter_name=="wavelet" || filter_name=="dehaar" || filter_name=="continuousdehaar" || filter_name=="DeHaarContinuousFilter")
  {
    if (field.dtype.isVectorOf(DTypes::FLOAT32)) return std::make_shared< Private::DeHaarContinuousFilter<Float32, Float32> >(this,field);
    if (field.dtype.isVectorOf(DTypes::FLOAT64)) return std::make_shared< Private::DeHaarContinuousFilter<Float64, Float64> >(this,field);
  }

  VisusWarning()<<"Cannot create filter, wrong name("<<filter_name<<")";
  return SharedPtr<DatasetFilter>();
}

} //namespace Visus
