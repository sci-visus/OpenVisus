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

    int              bit;
    Int64 delta;
    BoxNi            logic_box = query->logic_samples.logic_box;
    PointNi          stride = query->getNumberOfSamples().stride();
    PointNi          qshift = query->logic_samples.shift;
    Aborted          aborted = query->aborted;

    typedef struct { 
      int    H; 
      BoxNi  box; 
    } 
    KdStack;
    KdStack item;
    std::vector<KdStack> __stack__(max_resolution +1);
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
        }
        //reached the level: only one point
        else if (H == item.H)
        {
          PointNi  P = item.box.p1;
          Int64    hzfrom = cint64(hz - HzFrom);
          const PointNi  query_p1 = logic_box.p1;
          Int64 from = stride.dotProduct((P - query_p1).rightShift(qshift));
          auto& Windex = bInvertOrder ? hzfrom : from;
          auto& Rindex = bInvertOrder ? from : hzfrom;
          from = stride.dotProduct((P - query_p1).rightShift(qshift));
          Wbox[Windex] = Rbox[Rindex];

          hz++;
        }
        //kd-traversal code
        else
        {
          bit = bitmask[item.H];
          delta = deltas[item.H];
          ++item.H;
          item.box.p1[bit] += delta;                            VisusAssert(item.box.isFullDim()); *(stack++) = item;
          item.box.p1[bit] -= delta; item.box.p2[bit] -= delta; VisusAssert(item.box.isFullDim()); *(stack++) = item;
        }
      }
    }

    return true;
  }

};

//////////////////////////////////////////////////////////////////////////////////////////
std::vector<BigInt> IdxDataset::createBlockQueriesForBoxQuery(SharedPtr<BoxQuery> query)
{
  if (blocksFullRes())
    return Dataset::createBlockQueriesForBoxQuery(query);

  std::vector<BigInt> ret;

  int bitsperblock = getDefaultBitsPerBlock();
  VisusAssert(bitsperblock);

  DatasetBitmask bitmask = this->idxfile.bitmask;
  int max_resolution = getMaxResolution();

  typedef struct { 
    int H; 
    BoxNi box; 
  } 
  KdStack;

  KdStack  item;
  std::vector<KdStack> __stack__(max_resolution + 1);
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
        ret.push_back(blockid);

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
std::vector<BigInt> IdxDataset::createBlockQueriesForPointQuery(SharedPtr<PointQuery> query)
{
  if (blocksFullRes())
    return Dataset::createBlockQueriesForPointQuery(query);

  std::vector<BigInt> ret;

  query->offsets.clear();

  auto pdim = getPointDim(); VisusReleaseAssert(pdim==3);
  auto bounds = this->getLogicBox();
  auto last_bitmask = ((BigInt)1) << (getMaxResolution());
  auto hzorder = HzOrder(idxfile.bitmask);
  auto depth_mask = hzorder.getLevelP2Included(query->end_resolution);
  auto bitsperblock = getDefaultBitsPerBlock();
  auto samplesperblock = 1 << bitsperblock;

  BigInt blockid, block_offset;
  int    H;
  PointNi p(pdim),stride;
  auto SRC = (Int64*)query->points->c_ptr();
  BigInt hzaddress;
  LogicSamples block_samples;



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

    hzaddress = hzorder.getAddress(p);

    blockid = hzaddress >> bitsperblock;
    if (!query->offsets.count(blockid))
    {
      query->offsets[blockid]=std::make_shared<PointQuery::Offsets>();
      ret.push_back(blockid);
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
}

} //namespace Visus
