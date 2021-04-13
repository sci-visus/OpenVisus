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

#include <Visus/IdxMultipleAccess.h>
#include <Visus/IdxMultipleDataset.h>

namespace Visus {

///////////////////////////////////////////////////////////////////////////////////////
IdxMultipleAccess::IdxMultipleAccess(IdxMultipleDataset* VF_, StringTree CONFIG_)
  : DATASET(VF_), CONFIG(CONFIG_)
{
  this->name = CONFIG.readString("name", "IdxMultipleAccess");
  this->can_read = true;
  this->can_write = false;
  this->bitsperblock = DATASET->getDefaultBitsPerBlock();

  for (auto child : DATASET->down_datasets)
  {
    auto name = child.first;

    //see if the user specified how to access the data for each query dataset
    for (auto it : CONFIG.getChilds())
    {
      if (it->name == name || it->readString("name") == name)
        configs[std::make_pair(name, "")] = *it;
    }
  }

  bool disable_async = CONFIG.readBool("disable_async", DATASET->isServerMode());

  //TODO: special case when I can use the blocks
  //if (DATASET->childs.size() == 1 && DATASET->sameLogicSpace(DATASET->childs[0]))
  //  ;

  if (int nthreads = disable_async ? 0 : 3)
    this->thread_pool = std::make_shared<ThreadPool>("IdxMultipleAccess Worker", nthreads);
}

//destructor
IdxMultipleAccess::~IdxMultipleAccess()
{
  thread_pool.reset();
}

//////////////////////////////////////////////////////
SharedPtr<Access> IdxMultipleAccess::createDownAccess(String name, String fieldname)
{
  auto dataset = DATASET->getChild(name);
  VisusAssert(dataset);

  SharedPtr<Access> ret;

  StringTree config = dataset->getDefaultAccessConfig();

  auto it = this->configs.find(std::make_pair(name, fieldname));
  if (it == configs.end())
    it = configs.find(std::make_pair(name, ""));

  if (it != configs.end())
    config = it->second;

  //inerits attributes from CONFIG
  for (auto it : this->CONFIG.attributes)
  {
    auto key = it.first;
    auto value = it.second;
    if (!config.hasAttribute(key))
      config.setAttribute(key, value);
  }

  bool bForBlockQuery = DATASET->getKdQueryMode() & KdQueryMode::UseBlockQuery ? true : false;
  return dataset->createAccess(config, bForBlockQuery);
}

//////////////////////////////////////////////////////
void IdxMultipleAccess::readBlock(SharedPtr<BlockQuery> BLOCKQUERY)
{
  ThreadPool::push(thread_pool, [this, BLOCKQUERY]()
  {
    if (BLOCKQUERY->aborted())
      return readFailed(BLOCKQUERY,"aborted");

    /*
    TODO: can be async block query be enabled for simple cases?
      (like: I want to cache blocks for dw datasets)
      if all the childs are bSameLogicSpace I can do the blending of the blocks
      instead of the blending of the buffer of regular queries.

    To tell the truth i'm not sure if this solution would be different from what
    I'm doing right now (except that I can go async)
    */
    auto QUERY = DATASET->createEquivalentBoxQuery('r', BLOCKQUERY);
    DATASET->beginBoxQuery(QUERY);
    if (!DATASET->executeBoxQuery(shared_from_this(), QUERY))
      return readFailed(BLOCKQUERY,"box query failed");

    BLOCKQUERY->buffer = QUERY->buffer;
    return readOk(BLOCKQUERY);
  });
}

//////////////////////////////////////////////////////
void IdxMultipleAccess::writeBlock(SharedPtr<BlockQuery> BLOCKQUERY) {
  //not supported
  VisusAssert(false);
  writeFailed(BLOCKQUERY,"not supported");
}

} //namespace Visus
