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

#include <Visus/GoogleMapsDataset.h>
#include <Visus/Access.h>
#include <Visus/NetService.h>


namespace Visus {

//////////////////////////////////////////////////////////////
class GoogleMapsAccess : public Access
{
public:

  GoogleMapsDataset* dataset;

  StringTree             config;
  Url                    tiles;
  SharedPtr<NetService>  netservice;

  //constructor
  GoogleMapsAccess(GoogleMapsDataset* dataset_,StringTree config_=StringTree()) 
    : dataset(dataset_),config(config_) 
  {
    this->name = "GoogleMapsAccess";
    this->can_read  = StringUtils::find(config.readString("chmod", DefaultChMod), "r") >= 0;
    this->can_write = StringUtils::find(config.readString("chmod", DefaultChMod), "w") >= 0;
    this->bitsperblock = cint(config.readString("bitsperblock", cstring(dataset->getDefaultBitsPerBlock()))); VisusAssert(this->bitsperblock>0);
    this->tiles = dataset->tiles; 


    bool disable_async = config.readBool("disable_async", dataset->isServerMode());

    if (int nconnections = disable_async ? 0 : config.readInt("nconnections", 8))
      this->netservice = std::make_shared<NetService>(nconnections);
  }

  //destructor
  virtual ~GoogleMapsAccess(){
  }

  //readBlock
  virtual void readBlock(SharedPtr<BlockQuery> query) override
  {
    auto coord=dataset->getBlockCoordinate(query->blockid);

    auto X=coord[0];
    auto Y=coord[1];
    auto Z=coord[2];

    //mirror along Y
    Y=(int)((Int64(1)<<Z)-Y-1);
      
    auto url=Url(this->tiles);
    url.setParam("x",cstring(X));
    url.setParam("y",cstring(Y));
    url.setParam("z",cstring(Z));

    auto request=NetRequest(url);

    if (!request.valid())
      return readFailed(query);

    request.aborted=query->aborted;

    //note [...,query] keep the query in memory
    NetService::push(netservice, request).when_ready([this, query](NetResponse response) {

      PointNi nsamples = PointNi::one(2);
      nsamples[0] = dataset->tile_width;
      nsamples[1] = dataset->tile_height;

      response.setHeader("visus-compression", dataset->compression);
      response.setHeader("visus-nsamples", nsamples.toString());
      response.setHeader("visus-dtype", query->field.dtype.toString());
      response.setHeader("visus-layout", "");

      if (query->aborted() || !response.isSuccessful())
        return readFailed(query);

      auto decoded = response.getCompatibleArrayBody(query->getNumberOfSamples(), query->field.dtype);
      if (!decoded)
        return readFailed(query);

      query->buffer = decoded;

      return readOk(query);
    });
  }

  //writeBlock
  virtual void writeBlock(SharedPtr<BlockQuery> query) override
  {
    VisusAssert(false);//not supported
    writeFailed(query);
  }

  //printStatistics
  virtual void printStatistics() override {
    PrintInfo(name, "url", dataset->getUrl());
    Access::printStatistics();
  }

};


//////////////////////////////////////////////////////////////
bool GoogleMapsDataset::setEndResolution(SharedPtr<BoxQuery> query,int value)
{
  VisusAssert(query->end_resolution < value);
  query->end_resolution = value;

  auto end_resolution = query->end_resolution;
  auto user_box = query->logic_box.getIntersection(this->getLogicBox());
  VisusAssert(user_box.isFullDim());

  auto Lsamples = getLevelSamples(end_resolution);
  auto box = Lsamples.alignBox(user_box);

  if (!box.isFullDim())
    return false;

  query->logic_samples = LogicSamples(box, Lsamples.delta);
  return true;

}

//////////////////////////////////////////////////////////////
void GoogleMapsDataset::beginBoxQuery(SharedPtr<BoxQuery> query)
{
  if (!query)
    return;

  if (query->getStatus() != Query::QueryCreated)
    return;

  if (query->mode == 'w')
    return query->setFailed("Writing mode not suppoted");

  if (query->aborted())
    return query->setFailed("query aborted");

  if (!query->logic_box.valid())
    return query->setFailed("query logic position not valid");

  if (!query->logic_box.getIntersection(this->getLogicBox()).isFullDim())
    return query->setFailed("user_box not valid");

  if (query->start_resolution != 0)
    return query->setFailed("query start position wrong");

  if (query->end_resolutions.empty())
    query->end_resolutions = { this->getMaxResolution() };

  //only even resolution
  {
    std::set<int> good;
    for (auto it : query->end_resolutions)
    {
      auto value = (it >> 1) << 1;
      good.insert(Utils::clamp(value, getDefaultBitsPerBlock(), getMaxResolution()));
    }

    query->end_resolutions = std::vector<int>(good.begin(), good.end());
  }

  for (auto end_resolution : query->end_resolutions)
  {
    if (setEndResolution(query, end_resolution))
    {
      query->setRunning();
      return;
    }
  }

  query->setFailed();
}

//////////////////////////////////////////////////////////////
void GoogleMapsDataset::nextBoxQuery(SharedPtr<BoxQuery> query)
{
  if (!query)
    return;

  if (!(query->isRunning() && query->getCurrentResolution() == query->getEndResolution()))
    return;

  //reached the end?
  if (query->end_resolution == query->end_resolutions.back())
    return query->setOk();

  int I = Utils::find(query->end_resolutions, query->end_resolution) + 1;
  auto end_resolution = query->end_resolutions[I];
  VisusReleaseAssert(setEndResolution(query, end_resolution));

  //merging is not supported, so I'm resetting the buffer
  query->buffer = Array();
}


//////////////////////////////////////////////////////////////
bool GoogleMapsDataset::executeBoxQuery(SharedPtr<Access> access, SharedPtr<BoxQuery> query)
{
  if (!query)
    return false;

  if (!(query->isRunning() && query->getCurrentResolution() < query->getEndResolution()))
    return false;

  if (query->aborted())
  {
    query->setFailed("query aboted");
    return false;
  }

  //for 'r' queries I can postpone the allocation
  if (query->mode == 'w')
  {
    query->setFailed("write not supported");
    return false;
  }

  if (!query->allocateBufferIfNeeded())
  {
    query->setFailed("cannot allocate buffer");
    return false;
  }

  //always need an access.. the google server cannot handle pure remote queries (i.e. compose the tiles on server side)
  if (!access)
    access = std::make_shared<GoogleMapsAccess>(this);

  int end_resolution = query->end_resolution;
  VisusAssert(end_resolution % 2 == 0);

  WaitAsync< Future<Void> > wait_async;

  BoxNi box = this->getLogicBox();

  std::vector< SharedPtr<BlockQuery> > block_queries;
  kdTraverse(block_queries, query, box,/*id*/1,/*H*/this->getDefaultBitsPerBlock(), end_resolution);

  access->beginRead();
  {
    for (auto block_query : block_queries)
    {
      executeBlockQuery(access, block_query);
      wait_async.pushRunning(block_query->done).when_ready([this, query, block_query](Void) {

        if (!query->aborted() && block_query->ok())
          mergeBoxQueryWithBlockQuery(query, block_query);
        });
    }
  }
  access->endRead();

  wait_async.waitAllDone();

  VisusAssert(query->buffer.dims == query->getNumberOfSamples());
  query->setCurrentResolution(query->end_resolution);

  return true;
}


//////////////////////////////////////////////////////////////
void GoogleMapsDataset::kdTraverse(std::vector< SharedPtr<BlockQuery> >& block_queries,SharedPtr<BoxQuery> query,BoxNi box,BigInt id,int H,int end_resolution)
{
  if (query->aborted()) 
    return;

  if (!box.getIntersection(query->logic_box).isFullDim())
    return;

  int samplesperblock=1<<this->getDefaultBitsPerBlock();

  if (H==end_resolution)
  {
    VisusAssert(H % 2==0);
    auto block_query=createBlockQuery(id - 1, query->field,query->time,'r', query->aborted);
    block_queries.push_back(block_query);
    return;
  }

  DatasetBitmask bitmask=this->getBitmask();
  int split_bit=bitmask[1+H - this->getDefaultBitsPerBlock()];
  Int64 middle=(box.p1[split_bit]+box.p2[split_bit])>>1;

  auto left_box  =box; left_box .p2[split_bit]=middle; 
  auto right_box =box; right_box.p1[split_bit]=middle; 

  kdTraverse(block_queries,query,left_box ,id*2+0,H+1,end_resolution);
  kdTraverse(block_queries,query,right_box,id*2+1,H+1,end_resolution);
}


//////////////////////////////////////////////////////////////
bool GoogleMapsDataset::mergeBoxQueryWithBlockQuery(SharedPtr<BoxQuery> query,SharedPtr<BlockQuery> blockquery)
{
  return LogicSamples::merge(query->logic_samples, query->buffer, blockquery->logic_samples, blockquery->buffer, MergeMode::InsertSamples, query->aborted);
}

//////////////////////////////////////////////////////////////
SharedPtr<Access> GoogleMapsDataset::createAccess(StringTree config, bool bForBlockQuery)
{
  if (!config.valid())
    config = getDefaultAccessConfig();

  String type = StringUtils::toLower(config.readString("type"));

  //I always need an access
  if (type.empty() || type == "legacyaccess" || type=="GoogleMapsAccess")
    return std::make_shared<GoogleMapsAccess>(this, config);

  return Dataset::createAccess(config, bForBlockQuery);
}



//////////////////////////////////////////////////////////////
Point3i GoogleMapsDataset::getBlockCoordinate(BigInt blockid)
{
  int bitsperblock=this->getDefaultBitsPerBlock();
  int samplesperblock=((BigInt)1)<<bitsperblock;
  auto bitmask = getBitmask();

  int    H=bitsperblock+Utils::getLog2(1+ blockid);
  VisusAssert((H % 2)==0);
  Int64  first_block_in_level=(((Int64)1)<<(H-bitsperblock))-1;
  PointNi tile_coord=bitmask.deinterleave(blockid -first_block_in_level,H-bitsperblock);

  return Point3i(
    (int)(tile_coord[0]),
    (int)(tile_coord[1]),
    (H-bitsperblock)>>1);
}

//////////////////////////////////////////////////////////////
LogicSamples GoogleMapsDataset::getBlockSamples(BigInt blockid)
{
  auto coord= getBlockCoordinate(blockid);

  auto X=coord[0];
  auto Y=coord[1];
  auto Z=coord[2];

  int tile_width =(int)(this->getLogicBox().p2[0])>>Z;
  int tile_height=(int)(this->getLogicBox().p2[1])>>Z;

  PointNi delta=PointNi::one(2);
  delta[0]=tile_width /this->tile_width;
  delta[1]=tile_height/this->tile_height;

  BoxNi box(PointNi(2), PointNi::one(2));
  box.p1[0] = tile_width  * (X + 0); box.p2[0] = tile_width  * (X + 1);
  box.p1[1] = tile_height * (Y + 0); box.p2[1] = tile_height * (Y + 1);

  return LogicSamples(box,delta);
}

//////////////////////////////////////////////////////////////
void GoogleMapsDataset::read(Archive& ar)
{
  String url = ar.readString("url");

  ar.read("tiles", this->tiles, "http://mt1.google.com/vt/lyrs=s");
  ar.read("tile_width", tile_width, 256);
  ar.read("tile_height", tile_height, 256);
  ar.read("nlevels", this->nlevels, 22);
  ar.read("compression", this->compression, "jpg");
  ar.read("dtype",this->dtype,DTypes::UINT8_RGB);

  VisusReleaseAssert(tile_width>0 && tile_height>0);
  VisusAssert(nlevels > 0);
  VisusAssert(dtype.valid());
  VisusAssert(!compression.empty());

  //any google level double the dimensions in x and y (i.e. i don't have even resolutions)
  auto W= this->tile_width  * (((Int64)1)<<nlevels);
  auto H= this->tile_height * (((Int64)1)<<nlevels);

  this->setDatasetBody(ar);
  this->setKdQueryMode(KdQueryMode::fromString(ar.readString("kdquery")));
  this->setBitmask(DatasetBitmask::guess(PointNi(W,H)));
  this->setDefaultBitsPerBlock(Utils::getLog2(tile_width*tile_height));
  this->setLogicBox(BoxNi(PointNi(0,0), PointNi(W, H)));

  //using longiture [-180,+180]  latitude [-90,+90]  
  if (ar.hasAttribute("physic_box"))
  {
    auto physic_box = BoxNd::fromString(ar.getAttribute("physic_box"));
    setDatasetBounds(physic_box);
  }
  else
  {
    setDatasetBounds(BoxNi(PointNi(0, 0), PointNi(W,H)));
  }

  auto timesteps = DatasetTimesteps();
  timesteps.addTimestep(0);
  setTimesteps(timesteps);

  addField(Field("DATA",dtype));

  //UseBoxQuery not supported? actually yes, but it's a nonsense since a query it's really a block query
  if (getKdQueryMode() == KdQueryMode::UseBoxQuery)
    setKdQueryMode(KdQueryMode::UseBlockQuery);
}



//////////////////////////////////////////////////////////////
LogicSamples GoogleMapsDataset::getLevelSamples(int H)
{
  int bitsperblock=this->getDefaultBitsPerBlock();
  VisusAssert((H%2)==0 && H>=bitsperblock);
  int Z=(H-bitsperblock)>>1;
    
  int tile_width =(int)(this->getLogicBox().p2[0])>>Z;
  int tile_height=(int)(this->getLogicBox().p2[1])>>Z;
    
  int ntiles_x=(int)(1<<Z);
  int ntiles_y=(int)(1<<Z);

  PointNi delta=PointNi::one(2);
  delta[0]=tile_width /this->tile_width;
  delta[1]=tile_height/this->tile_height;
    
  BoxNi box(PointNi(0,0), PointNi(1,1));
  box.p2[0] = ntiles_x*tile_width;
  box.p2[1] = ntiles_y*tile_height;
    
  auto ret=LogicSamples(box,delta);
  VisusAssert(ret.valid());
  return ret;
}


} //namespace Visus

