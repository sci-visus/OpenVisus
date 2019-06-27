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


#include <Visus/KdQueryNode.h>
#include <Visus/GoogleMapsDataset.h>
#include <Visus/DatasetFilter.h>
#include <Visus/NetService.h>
#include <Visus/FieldNode.h>
#include <Visus/VisusConfig.h>
#include <Visus/ApplicationInfo.h>

namespace Visus {

////////////////////////////////////////////////////////////////
class KdQueryJob : public NodeJob 
{
public:

  Node*                         node = nullptr;

  int                           mode=KdQueryMode::NotSpecified;
  SharedPtr<Dataset>            dataset;
  SharedPtr<Access>             access;
  SharedPtr<KdArray>            kdarray;
  double                        time=0;
  Field                         field;
  Position                      position;
  int                           quality=0;
  Frustum                       viewdep;
  Time                          last_publish;
  int                           publish_interval=0;
  int                           bitsperblock=0;
  bool                          bBlocksAreFullRes=false;
  bool                          verbose=false;
  StringTree                    config;
  
  //constructor
  KdQueryJob(Node* node_,int mode_, StringTree config = StringTree()) : node(node_), mode(mode_)
  {
    this->config = config;
    this->verbose = cbool(this->config.readString("verbose", "0"));
  }

  //destructor
  virtual ~KdQueryJob() {
  }

  //validate
  virtual bool validate()
  {
    if (!dataset)
      return false;

    if (!kdarray)
      return false;

    if (!dataset->getTimesteps().containsTimestep(time))
      return false;

    if (!field.valid())
      return false;

    if (!position.valid())
      return false;

    //i need an access for accessing block
    if (mode == KdQueryMode::UseBlockQuery && !access)
      return false;

    auto pdim = dataset->getPointDim();
    this->bitsperblock=access? access->bitsperblock : dataset->getDefaultBitsPerBlock();

    //publish interval
    this->last_publish=Time::now();
    this->publish_interval = dataset->getPointDim() == 3 ? 2000 : 200;

    //find intersection with view frustum
    if (viewdep.valid())
    {
      position=Position::shrink(viewdep.getScreenBox(),FrustumMap(viewdep),position);
      if (!position.valid()) 
        return false;
    }

    //find intersection with dataset box
    position=Position::shrink(Position(dataset->getBox()).withoutTransformation().toBox3(),MatrixMap(Matrix::identity(4)),position);

    if (!position.valid()) 
      return false;

    //remove transformation
    position = Position(position.withoutTransformation().castTo<NdBox>().withPointDim(pdim).getIntersection(dataset->getBox()));
    if (!position.valid()) 
      return false;

    //failed for some reason
    std::vector<int> end_resolutions=dataset->guessEndResolutions(viewdep,position,(Query::Quality)quality,Query::NoProgression);
    if (end_resolutions.empty()) 
      return false;

    //need write lock here
    {
      ScopedWriteLock wlock(kdarray->lock);

      kdarray->query_box = position.getNdBox().withPointDim(pdim);
      kdarray->end_resolution = end_resolutions.back();

      this->bBlocksAreFullRes = std::dynamic_pointer_cast<GoogleMapsDataset>(dataset) ? true : false;

      //TODO enable also for UseBlockQuery?
      if (mode == KdQueryMode::UseQuery)
      {
        //all levels from [0,cutoff) will be cached without any memory limitations
        //there will be 2^cutoff-1 kdnodes  (8->255 , 9->511, 10->1024)
        int cutoff = 10;
        kdarray->enableCaching(cutoff, StringUtils::getByteSizeFromString("20mb"));
      }
    }

    return true;
  }

  //publish
  void publish(bool bForce)
  {
    if (aborted() || !node)
      return;

    if (bForce || last_publish.elapsedMsec() > publish_interval)
    {
      DataflowMessage msg;
      msg.writeValue("data", kdarray);
      node->publish(msg);
      last_publish = Time::now();
    }
  }

  //readRoot
  bool readRoot(ScopedReadLock& rlock)
  {
    VisusAssert(!kdarray->root);
    VisusAssert(bitsperblock);

    DatasetBitmask bitmask=dataset->getBitmask();

    int pdim = bitmask.getPointDim();

    //pow2_box
    NdPoint pow2_dims=NdPoint::one(pdim);
    for (int H = 1; H <= dataset->getMaxResolution(); H++)
      pow2_dims[bitmask[H]] <<= 1;

    NdBox pow2_box(NdPoint(pdim),pow2_dims);

    int max_resolution=dataset->getBitmask().getMaxResolution();
    VisusAssert(bitsperblock<=max_resolution);

    int end_resolution=
      (this->mode==KdQueryMode::UseBlockQuery && !bBlocksAreFullRes)? 
        std::min(bitsperblock + 1,max_resolution) : //I'm reading block 0+1 for block mode when the blocks are not fullres
        bitsperblock;

    //I use a box query to get the data
    auto query=std::make_shared<Query>(dataset.get(), 'r');
    query->time=time;
    query->field=field;
    query->position=pow2_box;
    query->end_resolutions={end_resolution};
    query->aborted=this->aborted;

    if (!dataset->beginQuery(query)) 
      return false;

    if (!dataset->executeQuery(access,query))
      return false;

    auto fullres     = query->buffer;
    auto displaydata = fullres;

    //If I'm getting the data using access, I need to apply the filters... 
    // otherwise is probably a kdquery=block with a remote url, the server will apply the filter
    if (access)
    {
      if (auto filter=dataset->createQueryFilter(field))
      {
        for (int H = 0; H <= end_resolution; H++)
        {
          query->cur_resolution=(H);
          if (!filter->computeFilter(query.get(),true))
            return false;
        }
        displaydata = filter->dropExtraComponentIfExists(fullres);
      }
    }

    {
      ScopedWriteLock wlock(rlock);

      kdarray->box = dataset->getBox();
      kdarray->clipping = position;

      kdarray->root = std::make_shared<KdArrayNode>();
      kdarray->root->resolution = end_resolution;
      kdarray->root->box = pow2_box;
      kdarray->root->id = 1; //root has id equals to 1 (important for split)
      kdarray->root->blockdata   = Array(); //it's not a single block, it is block 0+1!
      kdarray->root->fullres     = fullres;
      kdarray->root->displaydata = displaydata;
    }
    
    publish(/*bForce*/true);
    return true;
  }

  //computeFullRes
  void computeFullRes(KdArrayNode* node, ScopedReadLock& rlock)
  {
    //nothing to do
    if (mode == KdQueryMode::UseQuery)
      return;

    if (aborted() || !node)
      return;

    if (!node->fullres)
    {
      if (bBlocksAreFullRes)
      {
        if (node->blockdata)
        {
          {
            ScopedWriteLock wlock(rlock);
            node->fullres     = node->blockdata;
            node->displaydata = node->blockdata;
          }
          publish(/*bForce*/false);
        }
      }
      else
      {
        //node->fullres = node->up->fullres+node->blockdata
        if (!node->up->fullres || !node->blockdata)
          return;

        auto query = std::make_shared<Query>(dataset.get(), 'r');
        query->time = time;
        query->field = field;
        query->position = node->box;
        query->end_resolutions = { node->resolution };
        query->aborted = this->aborted;

        if (aborted() || !dataset->beginQuery(query))
          return;

        DatasetBitmask bitmask = dataset->getBitmask();
        int splitbit = bitmask[node->resolution - 1 - bitsperblock];
        int upsamplebit = bitmask[node->resolution];

        bool bLeftChild = node->up->left.get() == node ? true : false;
        auto fullres = bLeftChild ?
          ArrayUtils::splitAndGetFirst(node->up->fullres, splitbit, aborted) :
          ArrayUtils::splitAndGetSecond(node->up->fullres, splitbit, aborted);

        if (aborted() || !fullres)
          return;

        auto upsample = ArrayUtils::upSample(fullres, upsamplebit, aborted);
        if (aborted() || !upsample)
          return;

        fullres = upsample;

        VisusAssert(query->nsamples.innerProduct() == fullres.getTotalNumberOfSamples());

        //prepare to merge samples from current resolution
        query->cur_resolution = (node->resolution - 1);

        VisusAssert(fullres.dims == query->nsamples);
        query->buffer = fullres;

        auto start_address = (node->id) << bitsperblock;
        auto end_address = (node->id + 1) << bitsperblock;

        //I don't need to execute the block query, I already have it
        auto block_box = dataset->getAddressRangeBox(start_address, end_address);

        auto blockquery = std::make_shared<BlockQuery>(field, time, start_address, end_address, Aborted());
        blockquery->nsamples = block_box.nsamples;
        blockquery->logic_box = block_box;
        blockquery->buffer = node->blockdata;
        VisusAssert(blockquery->nsamples == node->blockdata.dims);

        if (aborted() || !dataset->mergeQueryWithBlock(query, blockquery))
          return;

        fullres = query->buffer;

        //this is the latest resolution! needed also for filter->applyToQuery!
        query->cur_resolution = node->resolution;

        //need to apply the filter, from now on I can display the data
        auto filter = dataset->createQueryFilter(field);
        if (aborted() || (filter && !filter->computeFilter(query.get(), true)))
          return;

        auto displaydata = filter ? filter->dropExtraComponentIfExists(fullres) : fullres;

        //store the results
        {
          ScopedWriteLock wlock(rlock);
          node->blockdata = Array(); //don't need this anymore 
          node->fullres = fullres;
          node->displaydata = displaydata;
        }

        publish(/*bForce*/false);
      }
    }

    computeFullRes(node->left .get(), rlock);
    computeFullRes(node->right.get(), rlock);
  }

  //runJobUsingBlockQuery
  void runJobUsingBlockQuery()
  {
    ScopedReadLock rlock(kdarray->lock);

    if (!kdarray->root && !readRoot(rlock))
      return;

    auto bitmask = dataset->getBitmask();

    WaitAsync< Future<Void> > wait_async;
    VisusAssert(access);
    access->beginRead();

    std::deque<KdArrayNode*> bfs;
    bfs.push_back(kdarray->root.get());
    while (!bfs.empty() && !aborted())
    {
      KdArrayNode* node = bfs.front(); bfs.pop_front();
      VisusAssert(node);

      //not visible, just drop it
      if (!kdarray->isNodeVisible(node))
      {
        ScopedWriteLock wlock(rlock);
        kdarray->clearChilds(node);
        continue;
      }

      //recursive splitting
      if (node->resolution < kdarray->end_resolution)
      {
        //make sure it's split
        if (node->isLeaf())
        {
          ScopedWriteLock wlock(rlock);
          kdarray->split(node, bitmask[1 + node->resolution - kdarray->root->resolution]); //jump the 'V'
        }

        bfs.push_back(node->left.get());
        bfs.push_back(node->right.get());
      }
      else
      {
        //drop unneeded levels
        if (!node->isLeaf())
        {
          ScopedWriteLock wlock(rlock);
          kdarray->clearChilds(node);
        }
      }

      //could be that I skipped the computation of fullres for aborted() signal
      computeFullRes(node, rlock);

      //all done for the current node
      if (node->fullres)
        continue;

      //already got the blockdata
      if (node->blockdata)
        continue;

      //for bBlocksAreFullRes I execute only final levels
      if (bBlocksAreFullRes && node->resolution != kdarray->end_resolution)
        continue;

      //retrieve the block data
      auto blocknum = bBlocksAreFullRes ? node->id - 1 : node->id; //IF !bBlocksAreFullRes node->id is the block number (considering that root has blocks 0 and 1)
      auto start_address = (blocknum) << bitsperblock;
      auto end_address = (blocknum + 1) << bitsperblock;

      auto blockquery = std::make_shared<BlockQuery>(field, time, start_address, end_address, this->aborted);
      wait_async.pushRunning(dataset->readBlock(access, blockquery)).when_ready([this, blockquery, node, &rlock](Void) {

        if (aborted() || blockquery->failed())
          return;

        VisusAssert(!node->fullres && !node->blockdata);

        //make sure is row major
        if (!blockquery->buffer.layout.empty())
          dataset->convertBlockQueryToRowMajor(blockquery);

        if (!blockquery->buffer)
          return;

        //need the write lock here
        {
          ScopedWriteLock wlock(rlock);
          node->blockdata = blockquery->buffer;
        }

        computeFullRes(node, rlock);
      });

    };

    access->endRead();

    wait_async.waitAllDone();

    if (aborted())
      return;

    publish(/*bForce*/true);

    if (verbose)
      this->access->printStatistics();
  }

  //runJobUsingQuery
  void runJobUsingQuery()
  {
    ScopedReadLock rlock(kdarray->lock);

    if (!kdarray->root && !readRoot(rlock))
      return;

    auto bitmask = dataset->getBitmask();

    //execute remote queries in async way
    SharedPtr<NetService> netservice;
    if (!access && dataset->getUrl().isRemote())
      netservice = std::make_shared<NetService>(8);

    WaitAsync< Future<NetResponse> >  wait_async;

    std::deque<KdArrayNode*> bfs;
    bfs.push_back(kdarray->root.get());
    while (!bfs.empty() && !aborted())
    {
      KdArrayNode* node = bfs.front(); bfs.pop_front();
      VisusAssert(node);

      //not visible, just drop it
      if (!kdarray->isNodeVisible(node))
      {
        ScopedWriteLock wlock(rlock);
        kdarray->clearChilds(node);
        continue;
      }

      //recursive splitting
      if (node->resolution < kdarray->end_resolution)
      {
        //make sure it's split
        if (node->isLeaf())
        {
          ScopedWriteLock wlock(rlock);
          kdarray->split(node, bitmask[1 + node->resolution - kdarray->root->resolution]); //jump the 'V'
        }

        bfs.push_back(node->left.get());
        bfs.push_back(node->right.get());
      }
      else
      {
        //drop unneeded levels
        if (!node->isLeaf())
        {
          ScopedWriteLock wlock(rlock);
          kdarray->clearChilds(node);
        }
      }

      //could be that I skipped the computation of fullres for aborted() signal
      computeFullRes(node, rlock);

      //all done for the current node
      if (node->fullres)
        continue;

      //for Query I execute only final level
      if (node->resolution != kdarray->end_resolution)
        continue;

      auto query = std::make_shared<Query>(dataset.get(), 'r');
      query->time = time;
      query->field = field;
      query->position = node->box;
      query->end_resolutions = { node->resolution };
      query->aborted = this->aborted;

      if (!dataset->beginQuery(query) || !query->allocateBufferIfNeeded())
        continue;

      //remote 'Query'
      if (netservice)
      {
        auto request = std::make_shared<NetRequest>(dataset->createPureRemoteQueryNetRequest(query));

        wait_async.pushRunning(NetService::push(netservice, *request)).when_ready([this, query, node, &rlock](NetResponse response) {

          VisusAssert(mode == KdQueryMode::UseQuery);

          if (this->aborted() || !response.isSuccessful())
            return;

          auto array = response.getArrayBody();
          if (!array)
            return;

          VisusAssert(!node->fullres);

          //need the write lock here
          {
            ScopedWriteLock wlock(rlock);
            node->fullres = array;
            node->displaydata = array;
          }

          this->publish(false);

        });
      }

      //execute in place (TODO: use thread here?)
      else if (dataset->executeQuery(access, query))
      {
        {
          ScopedWriteLock wlock(rlock);
          node->fullres = query->buffer;
          node->displaydata = query->buffer;
        }

        publish(false);
      }
    }

    wait_async.waitAllDone();

    if (aborted())
      return;

    publish(/*bForce*/true);

    if (verbose)
      this->access->printStatistics();
  }

  //runJob
  virtual void runJob() override
  {
    if (mode == KdQueryMode::UseBlockQuery)
      runJobUsingBlockQuery();
    else
      runJobUsingQuery();
  }

};

///////////////////////////////////////////////////////////////////////////////////////////////
bool KdQueryNode::processInput()
{
  abortProcessing();
   
  auto dataset         = readValue<Dataset>("dataset");
  auto fieldname       = cstring(readValue<String>("fieldname"));
  auto time            = cdouble(readValue<double>("time"));

  int kdquery_mode=dataset? dataset->getKdQueryMode() : KdQueryMode::NotSpecified;
  if (kdquery_mode==KdQueryMode::NotSpecified) 
    kdquery_mode=KdQueryMode::UseBlockQuery;

  VisusAssert(kdquery_mode==KdQueryMode::UseQuery || kdquery_mode==KdQueryMode::UseBlockQuery);

  //I cannot recycle the current kdarray
  if (!kdarray || !(dataset && this->dataset==dataset && this->time==time && this->fieldname==fieldname))
  {
    kdarray=std::make_shared<KdArray>(dataset? dataset->getPointDim() : 0);
    this->dataset      = dataset;
    this->time         = time;
    this->fieldname    = fieldname;
    this->setAccess(SharedPtr<Access>());
    if (dataset)
    {
      std::vector<StringTree*> access_configs=dataset->getAccessConfigs();
      int accessindex=getAccessIndex();
      if (accessindex>=0 && accessindex<(int)access_configs.size())
        this->setAccess(dataset->createAccess(*access_configs[accessindex]));
      else
        this->setAccess(kdquery_mode==KdQueryMode::UseBlockQuery? dataset->createAccessForBlockQuery() : dataset->createAccess());
    }
  }

  if (!dataset)
    return false;

  auto job=std::make_shared<KdQueryJob>(this, kdquery_mode);
  job->dataset=dataset;
  job->access=access;
  job->kdarray=kdarray;
  job->time=time;
  job->field=fieldname.empty()? dataset->getDefaultField() : dataset->getFieldByName(fieldname);
  job->position=getQueryPosition();
  job->quality=getQuality();
  job->viewdep=getViewDep();
  
  if (!job->validate()) 
    return false;
  
  addNodeJob(job);
  return true;
} 


} //namespace Visus



