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

#include <Visus/MultiplexAccess.h>
#include <Visus/BlockQuery.h>
#include <Visus/Dataset.h>

namespace Visus {

///////////////////////////////////////////////////////
MultiplexAccess::MultiplexAccess(Dataset* dataset, StringTree config)
{
  this->name = config.readString("name");
  this->dataset = dataset; //i need to create new BlockQuery
  this->can_read = true;
  this->can_write = false;
  this->bitsperblock = 0;

  for (auto child_config : config.getChilds())
  {
    if (child_config->name != "access")
      continue;

    auto child = dataset->createAccessForBlockQuery(*child_config);
    if (!child)
      ThrowException("wrong child access");

    this->addChild(child,*child_config);

  }

  //NOTE: 
  //I must use a thread because Access class are not thread-enabled
  //so I need to make sure that readBlock/writeBlock are called from the same thread 
  //Counter-example: writeBlock for caching could happen in a diffeent thread 

  this->thread = Thread::start("Multiplex thread", [this]() {
    runInBackground();
  });
}

///////////////////////////////////////////////////////
MultiplexAccess::~MultiplexAccess()
{
  //safe exit
  bExit = true;
  something_happened.up();
  Thread::join(this->thread);

}

///////////////////////////////////////////////////////
void MultiplexAccess::addChild(SharedPtr<Access> child,StringTree child_config)
{
  int bpb = child->bitsperblock;
  this->bitsperblock = (dw_access.empty()) ? bpb : std::min(this->bitsperblock, bpb);
  this->dw_access.push_back(SharedPtr<Access>(child));



  //adding support for filters
  Filter filter;
  if (child_config.hasAttribute("shard"))
  {
    //example: 
    //   <access shard="0/3 16" />   blocks [0 ,16) belongs to shard=0
    //   <access shard="1/3 16" />   blocks [16,32) belongs to shard=1
    //   <access shard="2/3 16" />   blocks [32,48) belongs to shard=2

    auto v = StringUtils::split(child_config.getAttribute("shard"));
    VisusReleaseAssert(v.size() == 2);

    auto v1 = StringUtils::split(v[0], "/");
    VisusReleaseAssert(v1.size() == 2);

    auto shard_id   = cbigint(v1[0]);
    auto tot_shards = cbigint(v1[1]);
    auto num_blocks = cbigint(v[1]);

    filter.A = num_blocks * shard_id; // offset  (ie. left overall limit)
    filter.B = NumericLimits<BigInt>::highest(); //right overall limit
    filter.F = num_blocks; //number of full blocks
    filter.S = num_blocks * tot_shards; //step

  }
  dw_filter.push_back(filter);
}

///////////////////////////////////////////////////////
void MultiplexAccess::printStatistics()
{
  PrintInfo("type", "MultiplexAccess");

  Access::printStatistics();

  PrintInfo("nchilds", dw_access.size());
  for (int i = 0; i < (int)dw_access.size(); i++)
    dw_access[i]->printStatistics();
}

///////////////////////////////////////////////////////
bool MultiplexAccess::passThought(int index, BigInt blockid)
{
  Filter filter = dw_filter[index];

  //not set
  if (filter.A == 0 && filter.B == 0 && filter.F == 0 && filter.S == 0)
    return true;

  // basically I need that the requested range is inside a full range, e.g.:
  // [A+K*step,A+K*step+full) (for some K)
  BigInt K = (blockid - filter.A) / filter.S;
  BigInt a = filter.A + K * filter.S;
  BigInt b = a + filter.F;
  return (a >= filter.A && b < filter.B) && (blockid >= a && blockid < b);
}

///////////////////////////////////////////////////////
void MultiplexAccess::scheduleOp(int mode, int index, SharedPtr<BlockQuery> up_query)
{
  auto blockid = up_query->blockid;
  
  if (mode == 'r')
  {
    //find the first who can read
    while (isGoodIndex(index) && (!dw_access[index]->can_read || !passThought(index, blockid)))
      index++;

    //invalid index
    if (!isGoodIndex(index))
      return readFailed(up_query, "wrong index");
    else
      VisusAssert(dw_access[index]->can_read && passThought(index, blockid));
  }
  else
  {
    VisusAssert(mode == 'w');

    //find the first who can write
    while (isGoodIndex(index) && (!dw_access[index]->can_write || !passThought(index, blockid)))
      index--;

    //even if the writing fails, the return code for the reading is ok
    if (!isGoodIndex(index)) 
      return readOk(up_query);
    else
      VisusAssert(dw_access[index]->can_write && passThought(index, blockid));
  }

  //PrintInfo("!!!!", blockid, dw_access[index]->name);

  auto dw_query = dataset->createBlockQuery(up_query->blockid, up_query->field, up_query->time, mode, up_query->aborted);
  VisusAssert(dw_query->getNumberOfSamples() == up_query->getNumberOfSamples());
  VisusAssert(dw_query->logic_samples == up_query->logic_samples);
  dw_query->buffer = up_query->buffer;

  {
    ScopedLock lock(this->lock);

    Pending pending;
    pending.index = index;
    pending.up_query = up_query;
    pending.dw_query = dw_query;
    pendings.push_back(pending);
    something_happened.up();
  }
}


///////////////////////////////////////////////////////
void MultiplexAccess::runInBackground()
{
  while (true)
  {
    std::vector<Pending> pendings;

    while (true)
    {
      this->lock.lock();

      if (this->pendings.empty())
      {
        this->lock.unlock();

        if (bExit)
          return;

        something_happened.down();
        continue;
      }

      pendings = this->pendings;
      this->pendings.clear();
      this->lock.unlock();
      break;
    }

    std::vector<int> new_modes(dw_access.size(), 0);
    for (auto pending : pendings)
    {
      VisusAssert(isGoodIndex(pending.index));
      new_modes[pending.index] = pending.dw_query->mode;
    }

    for (int index = 0; index < (int)dw_access.size(); index++)
    {
      auto cur_mode = dw_access[index]->getMode();
      auto new_mode = new_modes[index];

      if (new_mode != cur_mode)
      {
        if (cur_mode)
          dw_access[index]->endIO();

        if (new_mode)
          dw_access[index]->beginIO(new_mode);
      }
    }

    for (auto it : pendings)
    {
      auto index = it.index;
      auto up_query = it.up_query;
      auto dw_query = it.dw_query;

      //need to read
      if (dw_query->mode == 'r')
      {
        dataset->executeBlockQuery(dw_access[index], dw_query);
        dw_query->done.when_ready([this, up_query, dw_query, index](Void)
        {
          //if fails try the next index
          if (dw_query->failed())
          {
            scheduleOp('r', index + 1, up_query);
          }
          //I need to write to upper access (i.e. for caching the reading)
          //NOTE: the operation is readBlock and the caching happens in the previous dw_access
          else
          {
            VisusAssert(dw_query->ok());
            VisusAssert(up_query->blockid == dw_query->blockid);
            VisusAssert(up_query->getNumberOfSamples() == dw_query->getNumberOfSamples());
            VisusAssert(up_query->logic_samples == dw_query->logic_samples);

            up_query->buffer = dw_query->buffer;
            scheduleOp('w', index - 1, up_query);
          }
        });
      }

      //need to writing (e.g. cachning a block
      else
      {
        VisusAssert(dw_query->mode == 'w');

        //if fails or not I don't care, I try to cache to upper levels anyway
        dataset->executeBlockQuery(dw_access[index], dw_query);
        dw_query->done.when_ready([this, up_query, dw_query, index](Void) {
          scheduleOp('w', index - 1, up_query);
        });
      }
    }

    //end IO only for down access that do not have activity in the next cycle
    //otherwise it's better to 'group' IO in the same begin/end
    {
      ScopedLock lock(this->lock);
      pendings = this->pendings;
    }

    new_modes = std::vector<int>(dw_access.size(), 0);
    for (auto pending : pendings)
    {
      VisusAssert(isGoodIndex(pending.index));
      new_modes[pending.index] = pending.dw_query->mode;
    }

    for (int index = 0; index < (int)dw_access.size(); index++)
    {
      auto cur_mode = dw_access[index]->getMode();
      auto new_mode = new_modes[index];

      if (!new_mode && cur_mode)
        dw_access[index]->endIO();
    }
  }
}

} //namespace Visus 

