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
#include <Visus/IdxHzOrder.h>

namespace Visus {

////////////////////////////////////////////////////////
class IdxBoxQueryHzAddressConversion
{
public:

  //_______________________________________________________________
  class Level
  {
  public:

    int                   num = 0;// total number of cachable elements
    int                   pdim = 0;// what is the space I need to work
    SharedPtr<HeapMemory> cached_points = std::make_shared<HeapMemory>();

    //constructor
    Level(const DatasetBitmask& bitmask, int H, int numbits = 10)
    {
      VisusAssert(bitmask.valid());
      --H; //yes! it is correct

      numbits = std::max(0, std::min(numbits, H));

      //number of bits which can be cached for current H
      this->num = 1 << numbits; VisusAssert(this->num);
      this->pdim = bitmask.getPointDim();

      if (!cached_points->resize(this->num * sizeof(PointNi), __FILE__, __LINE__))
      {
        this->clear();
        return;
      }

      cached_points->fill(0);

      PointNi* ptr = (PointNi*)cached_points->c_ptr();

      HzOrder hzorder(bitmask, H);

      //create the delta for points  
      for (BigInt zaddress = 0; zaddress < (this->num - 1); zaddress++, ptr++)
      {
        PointNi Pcur = hzorder.deinterleave(zaddress + 0);
        PointNi Pnex = hzorder.deinterleave(zaddress + 1);

        (*ptr) = PointNi(pdim);

        //store the delta
        for (int D = 0; D < pdim; D++)
          (*ptr)[D] = Pnex[D] - Pcur[D];
      }

      //i want the last (FAKE and unused) element to be zero
      (*ptr) = PointNi(pdim);
    }

    //destructor
    inline ~Level() {
      clear();
    }

    //valid
    bool valid() {
      return this->num > 0;
    }

    //memsize
    inline int memsize() const {
      return sizeof(PointNi) * (this->num);
    }

    //clear
    inline void clear() {
      this->cached_points.reset();
      this->num = 0;
      this->pdim = 0;
    }
  };

  std::vector< SharedPtr<Level> > levels;

  //constructor
  IdxBoxQueryHzAddressConversion(DatasetBitmask& bitmask)
  {
		for (int I = 0; I <= bitmask.getMaxResolution(); I++)
			this->levels.push_back(std::make_shared<Level>(bitmask, I));
  }

};

////////////////////////////////////////////////////////
class IdxPointQueryHzAddressConversion
{
public:

  Int64 memsize = 0;

  std::vector< std::pair<BigInt, Int32>* > loc;

  //create
  IdxPointQueryHzAddressConversion(DatasetBitmask bitmask)
  {
    //todo cases for which I'm using regexp
    auto MaxH = bitmask.getMaxResolution();
    BigInt last_bitmask = ((BigInt)1) << MaxH;

    HzOrder hzorder(bitmask);
    int pdim = bitmask.getPointDim();
    loc.resize(pdim);

    for (int D = 0; D < pdim; D++)
    {
      Int64 dim = bitmask.getPow2Dims()[D];
      this->loc[D] = new std::pair<BigInt, Int32>[dim];
      BigInt one = 1;
      for (int i = 0; i < dim; i++)
      {
        PointNi p(pdim);
        p[D] = i;
        auto& pair = this->loc[D][i];
        pair.first = hzorder.interleave(p);
        pair.second = 0;
        BigInt temp = pair.first | last_bitmask;
        while ((one & temp) == 0) { pair.second++; temp >>= 1; }
        temp >>= 1;
        pair.second++;
      }
    }
  }

  //destructor
  ~IdxPointQueryHzAddressConversion() {
    for (auto it : loc)
      delete[] it;
  }

};

////////////////////////////////////////////////////////
class HzAddressConversion
{
public:
  CriticalSection lock;
  std::map<String, SharedPtr<IdxBoxQueryHzAddressConversion> >   box;
  std::map<String, SharedPtr<IdxPointQueryHzAddressConversion> > point;

  //create
  void create(IdxDataset* idx)
  {
    ScopedLock lock(this->lock);

    auto bitmask = idx->getBitmask();
    auto key = bitmask.toString();

    if (!this->box.count(key))
      this->box[key] = std::make_shared<IdxBoxQueryHzAddressConversion>(bitmask);

    idx->hzconv.box = this->box[key];

    //create the loc-cache only for 3d data, in 2d I know I'm not going to use it!
    //instead in 3d I will use it a lot (consider a slice in odd position)
    if (bitmask.getPointDim() == 3)
    {
      if (!this->point.count(key))
        this->point[key] = std::make_shared<IdxPointQueryHzAddressConversion>(bitmask);

      idx->hzconv.point = this->point[key];
    }
  }
};

static HzAddressConversion HZCONV;

///////////////////////////////////////////////////////////////////////////
class ConvertHzOrderSamples
{
public:

  //execute
  template <class Sample>
  bool execute(IdxDataset* vf, BoxQuery* query, BlockQuery* block_query)
  {
    VisusAssert(query->field.dtype == block_query->buffer.dtype);

    VisusAssert(block_query->buffer.layout == "hzorder");

    bool bInvertOrder = query->mode == 'w';

    auto bitsperblock = vf->getDefaultBitsPerBlock();
    int  samplesperblock = 1 << bitsperblock;

    DatasetBitmask bitmask = vf->idxfile.bitmask;
    int            max_resolution = vf->getMaxResolution();
    BigInt         HzFrom = block_query->blockid * samplesperblock;
    BigInt         HzTo = HzFrom + samplesperblock;
    HzOrder        hzorder(bitmask);
    int            hstart = std::max(query->getCurrentResolution() + 1, block_query->blockid == 0 ? 0 : block_query->H);
    int            hend = std::min(query->getEndResolution(), block_query->H);

    VisusAssert(HzFrom == 0 || hstart == hend);

    auto Wbox = GetSamples<Sample>(query->buffer);
    auto Rbox = GetSamples<Sample>(block_query->buffer);

    if (bInvertOrder)
      std::swap(Wbox, Rbox);

    auto hzconv = vf->hzconv.box;
    VisusReleaseAssert(hzconv);

    int              numused = 0;
    int              bit;
    Int64 delta;
    BoxNi            logic_box = query->logic_samples.logic_box;
    PointNi          stride = query->getNumberOfSamples().stride();
    PointNi          qshift = query->logic_samples.shift;
    BigInt           numpoints;
    Aborted          aborted = query->aborted;

    typedef struct { 
      int    H; 
      BoxNi  box; 
    } 
    FastLoopStack;
    FastLoopStack item;
    std::vector<FastLoopStack> __stack__(max_resolution +1);
    const auto STACK = &__stack__[0];

    //layout of the block
    auto block_logic_box = block_query->getLogicBox();
    if (!block_logic_box.valid())
      return false;

    //deltas
    std::vector<Int64> deltas(max_resolution + 1);
    for (int H = 0; H <= max_resolution; H++)
      deltas[H] = H ? (vf->level_samples[H].delta[bitmask[H]] >> 1) : 0;

    for (int H = hstart; H <= hend; H++)
    {
      if (aborted())
        return false;

      LogicSamples Lsamples = vf->level_samples[H];
      PointNi  lshift = Lsamples.shift;

      BoxNi   zbox = (HzFrom != 0) ? block_logic_box : Lsamples.logic_box;
      BigInt  hz = hzorder.getAddress(zbox.p1);

      BoxNi user_box = logic_box.getIntersection(zbox);
      BoxNi box = Lsamples.alignBox(user_box);
      if (!box.isFullDim())
        continue;

      VisusReleaseAssert(hzconv->levels[H]);
      const auto& fllevel = *(hzconv->levels[H]);
      int cachable = std::min(fllevel.num, samplesperblock);

      //i need this to "split" the fast loop in two chunks
      VisusAssert(cachable > 0 && cachable <= samplesperblock);

      //push root in the kdtree
      item.box = zbox;
      item.H = H ? std::max(1, H - bitsperblock) : (0);
      auto stack = &STACK[0];
      *(stack++) = item;

      while (stack != STACK)
      {
        if (aborted())
          return false;

        item = *(--stack);

        // no intersection
        if (!item.box.strictIntersect(box))
        {
          hz += (((BigInt)1) << (H - item.H));
          continue;
        }

        // all intersection and all the samples are contained in the FastLoopCache
        numpoints = ((BigInt)1) << (H - item.H);
        if (numpoints <= cachable && box.containsBox(item.box))
        {
          Int64    hzfrom = cint64(hz - HzFrom);
          Int64    num = cint64(numpoints);
          PointNi* cc = (PointNi*)fllevel.cached_points->c_ptr();
          const PointNi  query_p1 = logic_box.p1;
          PointNi  P = item.box.p1;

          ++numused;

          VisusAssert(hz >= HzFrom && hz < HzTo);

          Int64 from = stride.dotProduct((P - query_p1).rightShift(qshift));

          auto& Windex = bInvertOrder ? hzfrom : from;
          auto& Rindex = bInvertOrder ? from : hzfrom;

          auto shift = (lshift - qshift);

          //slow version (enable it if you have problems)
#if 0
          for (; num--; ++hzfrom, ++cc)
          {
            from = stride.dotProduct((P - query_p1).rightShift(qshift));
            Wbox[Windex] = Rbox[Rindex];
            P += (cc->leftShift(lshift));
          }
          //fast version
#else

#define EXPRESSION(num) stride[num] * ((*cc)[num] << shift[num]) 
          switch (fllevel.pdim) {
          case 2: for (; num--; ++hzfrom, ++cc) { Wbox[Windex] = Rbox[Rindex]; from += EXPRESSION(0) + EXPRESSION(1); } break;
          case 3: for (; num--; ++hzfrom, ++cc) { Wbox[Windex] = Rbox[Rindex]; from += EXPRESSION(0) + EXPRESSION(1) + EXPRESSION(2); } break;
          case 4: for (; num--; ++hzfrom, ++cc) { Wbox[Windex] = Rbox[Rindex]; from += EXPRESSION(0) + EXPRESSION(1) + EXPRESSION(2) + EXPRESSION(3); } break;
          case 5: for (; num--; ++hzfrom, ++cc) { Wbox[Windex] = Rbox[Rindex]; from += EXPRESSION(0) + EXPRESSION(1) + EXPRESSION(2) + EXPRESSION(3) + EXPRESSION(4); } break;
          default: ThrowException("internal error"); break;
          }
#undef EXPRESSION
#endif

          hz += numpoints;
          continue;
        }

        //kd-traversal code
        bit = bitmask[item.H];
        delta = deltas[item.H];
        ++item.H;
        item.box.p1[bit] += delta;                            VisusAssert(item.box.isFullDim()); *(stack++) = item;
        item.box.p1[bit] -= delta; item.box.p2[bit] -= delta; VisusAssert(item.box.isFullDim()); *(stack++) = item;
      }
    }

    VisusAssert(numused > 0);
    return true;
  }

};

//////////////////////////////////////////////////////////////////////////////////////////
std::vector< SharedPtr<BlockQuery> > IdxDataset::createBlockQueriesForBoxQuery(SharedPtr<BoxQuery> query)
{
  if (blocksFullRes())
    return Dataset::createBlockQueriesForBoxQuery(query);

  std::vector< SharedPtr<BlockQuery> > ret;

  int bitsperblock = getDefaultBitsPerBlock();
  VisusAssert(bitsperblock);

  DatasetBitmask bitmask = this->idxfile.bitmask;
  int max_resolution = getMaxResolution();

  typedef struct { 
    int H; 
    BoxNi box; 
  } 
  FastLoopStack;

  FastLoopStack  item;
  std::vector<FastLoopStack> __stack__(max_resolution + 1);
  const auto STACK = &__stack__[0];

  std::vector<Int64> deltas(max_resolution + 1);
  for (auto H = 0; H <= max_resolution; H++)
    deltas[H] = H ? (this->level_samples[H].delta[bitmask[H]] >> 1) : 0;

  HzOrder hzorder(bitmask);
  int cur_resolution = query->getCurrentResolution();
  int end_resolution = query->end_resolution;

  for (int H = cur_resolution + 1; H <= end_resolution; H++)
  {
    if (query->aborted())
      return {};

    LogicSamples Lsamples = this->level_samples[H];
    BoxNi box = Lsamples.alignBox(query->logic_samples.logic_box);
    if (!box.isFullDim())
      continue;

    //push first item
    BigInt hz = hzorder.getAddress(Lsamples.logic_box.p1);

    item.box = Lsamples.logic_box;
    item.H = H ? 1 : 0;
    auto stack = STACK;
    *(stack++) = item;

    while (stack != STACK)
    {
      item = *(--stack);

      // no intersection
      if (!item.box.strictIntersect(box))
      {
        hz += (((BigInt)1) << (H - item.H));
        continue;
      }

      // intersection with hz-block!
      if ((H - item.H) <= bitsperblock)
      {
        auto blockid = hz >> bitsperblock;
        ret.push_back(createBlockQuery(blockid, query->field, query->time, 'r', query->aborted));

        // I know that block 0 convers several hz-levels from [0 to bitsperblock]
        if (blockid == 0)
        {
          H = bitsperblock;
          break;
        }

        hz += ((BigInt)1) << (H - item.H);
        continue;
      }

      //kd-traversal code
      int bit = bitmask[item.H];
      Int64 delta = deltas[item.H];
      ++item.H;
      item.box.p1[bit] += delta;                            *(stack++) = item;
      item.box.p1[bit] -= delta; item.box.p2[bit] -= delta; *(stack++) = item;

    } //while 

  } //for levels

  return ret;
}

//////////////////////////////////////////////////////////////////////////////////////////
bool IdxDataset::mergeBoxQueryWithBlockQuery(SharedPtr<BoxQuery> query, SharedPtr<BlockQuery> block_query)
{
  if (bool is_hz_order = !block_query->buffer.layout.empty())
  {
    //block query is hzorder, query is rowmajor
    if (!query->allocateBufferIfNeeded())
      return false;

    ConvertHzOrderSamples op;
    return NeedToCopySamples(op, query->field.dtype, this, query.get(), block_query.get());
  }

  return Dataset::mergeBoxQueryWithBlockQuery(query, block_query);
}

///////////////////////////////////////////////////////////////////////////////////////
std::vector< SharedPtr<BlockQuery> > IdxDataset::createBlockQueriesForPointQuery(SharedPtr<PointQuery> query)
{
  if (blocksFullRes())
    return Dataset::createBlockQueriesForPointQuery(query);

  std::vector< SharedPtr<BlockQuery> > ret;

  query->offsets.clear();

  auto pdim = getPointDim(); VisusReleaseAssert(pdim==3);
  auto bounds = this->getLogicBox();
  auto last_bitmask = ((BigInt)1) << (getMaxResolution());
  auto hzorder = HzOrder(idxfile.bitmask);
  auto depth_mask = hzorder.getLevelP2Included(query->end_resolution);
  auto bitsperblock = getDefaultBitsPerBlock();
  auto samplesperblock = 1 << bitsperblock;

  BigInt zaddress, blockid, block_offset;
  int    shift, H;
  PointNi p(pdim),stride;
  auto SRC = (Int64*)query->points->c_ptr();
  BigInt hzaddress;
  LogicSamples block_samples;

  auto hzconv = this->hzconv.point ? &this->hzconv.point->loc : nullptr;
  if (!hzconv)
  {
    PrintWarning("The hzaddress_conversion_pointquery has not been created, so loc-by-loc queries will be a lot slower!!!!");
    VisusAssert(false); //so you investigate why it's happening! 
  }

  for (int N = 0, Tot = (int)query->getNumberOfPoints().innerProduct(); N < Tot; N++, SRC += pdim)
  {
    if (query->aborted()) {
      query->setFailed("query aborted");
      return {};
    }

    p[0] = SRC[0] & depth_mask[0];
    p[1] = SRC[1] & depth_mask[1];
    p[2] = SRC[2] & depth_mask[2];

    if (!(
      p[0] >= bounds.p1[0] && p[0] < bounds.p2[0] &&
      p[1] >= bounds.p1[1] && p[1] < bounds.p2[1] &&
      p[2] >= bounds.p1[2] && p[2] < bounds.p2[2]))
    {
      continue;
    }

    if (!hzconv)
    {
      hzaddress = hzorder.getAddress(p);
    }
    else
    {
      shift = Utils::min((*hzconv)[0][p[0]].second, (*hzconv)[1][p[1]].second, (*hzconv)[2][p[2]].second);
      zaddress = (*hzconv)[0][p[0]].first | (*hzconv)[1][p[1]].first | (*hzconv)[2][p[2]].first;
      hzaddress = ((zaddress | last_bitmask) >> shift);
    }

    blockid = hzaddress >> bitsperblock;
    if (!query->offsets.count(blockid))
    {
      query->offsets[blockid]=std::make_shared<PointQuery::Offsets>();
      ret.push_back(createBlockQuery(blockid, query->field, query->time, 'r', query->aborted));
    }

    block_samples = getBlockQuerySamples(blockid, H);
    stride = block_samples.nsamples.stride();

    //todo: is there a better way to get this?
    block_offset =
      stride[0] * ((p[0] - block_samples.logic_box.p1[0]) / block_samples.delta[0]) +
      stride[1] * ((p[1] - block_samples.logic_box.p1[1]) / block_samples.delta[1]) +
      stride[2] * ((p[2] - block_samples.logic_box.p1[2]) / block_samples.delta[2]);

    query->offsets[blockid]->push_back(std::make_pair(N, block_offset));
  }

  return ret;
}

////////////////////////////////////////////////////////////////////
void IdxDataset::readDatasetFromArchive(Archive& ar)
{
  Dataset::readDatasetFromArchive(ar);
  HZCONV.create(this);
}

} //namespace Visus
