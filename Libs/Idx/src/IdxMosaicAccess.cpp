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

#include <Visus/IdxMosaicAccess.h>
#include <Visus/IdxMultipleDataset.h>
#include <Visus/VisusConfig.h>
#include <Visus/ApplicationStats.h>

namespace Visus {

///////////////////////////////////////////////////////////////////////////////////////
IdxMosaicAccess::IdxMosaicAccess(IdxMultipleDataset* VF_, StringTree CONFIG)
  : VF(VF_)
{
  VisusReleaseAssert(VF->bMosaic);

  if (!VF->valid())
    ThrowException("IdxDataset not valid");

  this->name = CONFIG.readString("name", "IdxMosaicAccess");
  this->CONFIG = CONFIG;
  this->can_read  = StringUtils::find(CONFIG.readString("chmod", "rw"), "r") >= 0;
  this->can_write = StringUtils::find(CONFIG.readString("chmod", "rw"), "w") >= 0;
  this->bitsperblock = VF->getDefaultBitsPerBlock();

  auto first = VF->childs.begin()->second.dataset;
  auto dims = first->getLogicBox().p2;
  int  pdim = first->getPointDim();

  for (auto it : VF->childs)
  {
    auto vf = std::dynamic_pointer_cast<IdxDataset>(it.second.dataset); VisusAssert(vf);
    VisusAssert(false);// need to check if this is correct
    auto offset = it.second.M.getCol(pdim).castTo<PointNi>();
    auto index = offset.innerDiv(dims);
    VisusAssert(!this->childs.count(index));
    this->childs[index].dataset=vf;
  }
}

///////////////////////////////////////////////////////////////////////////////////////
IdxMosaicAccess::~IdxMosaicAccess()
{
}

///////////////////////////////////////////////////////////////////////////////////////
SharedPtr<Access> IdxMosaicAccess::getChildAccess(const Child& child) const
{
  if (child.access)
    return child.access;

  //with thousansands of childs I don't want to create ThreadPool or NetService
  auto config = StringTree();
  config.writeBool("disable_async",true); 
  auto ret = child.dataset->createAccess(config,/*bForBlockQuery*/true);
  const_cast<Child&>(child).access = ret;
  return ret;
}

///////////////////////////////////////////////////////////////////////////////////////
void IdxMosaicAccess::beginIO(String mode) {
  Access::beginIO(mode);
}

///////////////////////////////////////////////////////////////////////////////////////
void IdxMosaicAccess::endIO() 
{
  for (const auto& it : childs)
  {
    auto access = it.second.access;
    if (access && (access->isReading() || access->isWriting()))
      access->endIO();
  }

  Access::endIO();
}


///////////////////////////////////////////////////////////////////////////////////////
void IdxMosaicAccess::readBlock(SharedPtr<BlockQuery> QUERY)
{
  VisusAssert(isReading());

  auto pdim = VF->getPointDim();
  auto BLOCK = QUERY->start_address >> bitsperblock;
  auto first = childs.begin()->second.dataset;
  auto NBITS = VF->getMaxResolution() - first->getMaxResolution();
  PointNi dims = first->getLogicBox().p2;

  bool bBlockTotallyInsideSingle = (BLOCK >= ((BigInt)1 << NBITS));

  if (bBlockTotallyInsideSingle)
  {
    //forward the block read to a single child
    auto index = QUERY->logic_box.p1.innerDiv(dims);
    auto p1 = QUERY->logic_box.p1.innerMod(dims);

    auto it = childs.find(index);
    if (it == childs.end())
      return readFailed(QUERY);

    auto vf = it->second.dataset;
    VisusAssert(vf);

    auto hzfrom = HzOrder(vf->idxfile.bitmask).getAddress(p1);

    auto block_query = std::make_shared<BlockQuery>(QUERY->field, QUERY->time, hzfrom, hzfrom + ((BigInt)1 << bitsperblock), QUERY->aborted);

    auto access = getChildAccess(it->second);

    if (!access->isReading())
      access->beginIO(this->getMode());

    //TODO: should I keep track of running queries in order to wait for them in the destructor?
    vf->readBlock(access, block_query).when_ready([this, QUERY, block_query](Void) {

      if (block_query->failed())
        return readFailed(QUERY); //failed

      QUERY->buffer = block_query->buffer;
      return readOk(QUERY);
    });
  }
  else
  {
    //THIS IS GOING TO BE SLOW: i need to compose coarse blocks by executing "normal" query and merging them
    auto t1 = Time::now();

    VisusInfo()<<"IdxMosaicAccess is composing block "<<BLOCK<<" (slow)";

    //row major
    QUERY->buffer.layout = "";

    DatasetBitmask BITMASK = VF->idxfile.bitmask;
    HzOrder HZORDER(BITMASK);

    for (const auto& it : childs)
    {
      auto vf     = it.second.dataset;
      auto offset = it.first.innerMultiply(dims);
      auto access = getChildAccess(it.second);

      auto query = std::make_shared<Query>(vf.get(), 'r');
      query->time = QUERY->time;
      query->field = QUERY->field;
      query->position = QUERY->logic_box.translate(-offset);
      query->end_resolutions = { HZORDER.getAddressResolution(BITMASK, QUERY->end_address - 1) - NBITS };
      query->start_resolution = BLOCK ? query->end_resolutions[0] : 0;

      if (access->isReading() || access->isWriting())
        access->endIO();

      if (!vf->beginQuery(query))
        continue;

      if (!query->allocateBufferIfNeeded())
        continue;

      if (!vf->executeQuery(access, query))
        continue;

      auto pixel_p1 =      PointNi(pdim); auto logic_p1 = query->logic_box.pixelToLogic(pixel_p1); auto LOGIC_P1 = logic_p1 + offset; auto PIXEL_P1 = QUERY->logic_box.logicToPixel(LOGIC_P1);
      auto pixel_p2 = query->buffer.dims; auto logic_p2 = query->logic_box.pixelToLogic(pixel_p2); auto LOGIC_P2 = logic_p2 + offset; auto PIXEL_p2 = QUERY->logic_box.logicToPixel(LOGIC_P2);

      ArrayUtils::insert(
        QUERY->buffer, PIXEL_P1, PIXEL_p2, PointNi::one(pdim),
        query->buffer, pixel_p1, pixel_p2, PointNi::one(pdim),
        QUERY->aborted);
    }

    if (bool bPrintStats = false)
    {
      auto stats = ApplicationStats::io.readValues(true);
      VisusInfo() << "!!! BLOCK " << BLOCK << " inside " << (bBlockTotallyInsideSingle ? "yes" : "no")
        << " nopen(" << stats.nopen << ") rbytes(" << StringUtils::getStringFromByteSize(stats.rbytes) << ")  wbytes(" << StringUtils::getStringFromByteSize(stats.wbytes) << ")"
        << " msec(" << t1.elapsedMsec() << ")";
    }

    return QUERY->aborted() ? readFailed(QUERY) : readOk(QUERY);
  }
}

///////////////////////////////////////////////////////////////////////////////////////
void IdxMosaicAccess::writeBlock(SharedPtr<BlockQuery> QUERY)
{
  //not supported!
  VisusAssert(isWriting());
  VisusAssert(false);
  return writeFailed(QUERY);
}

} //namespace Visus









