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

#include <Visus/LegacyDataset.h>
#include <Visus/NetworkAccess.h>


namespace Visus {

//////////////////////////////////////////////////////////////
class LegacyAccess : public NetworkAccess
{
public:

  LegacyDataset* dataset;

  //constructor
  LegacyAccess(LegacyDataset* dataset_,StringTree config=StringTree()) 
    : NetworkAccess("LegacyAccess", dataset_, config), dataset(dataset_) {
  }

  //destructor
  virtual ~LegacyAccess(){
  }

  //readBlock
  virtual void readBlock(SharedPtr<BlockQuery> query) override
  {
    auto coord=dataset->getTileCoordinate(query->start_address,query->end_address);

    int X=coord.x;
    int Y=coord.y;
    int Z=coord.z;

    //mirror along Y
    Y=(int)((1<<Z)-Y-1);
      
    Url url=dataset->getUrl();
    url.params.clear();
    url.setParam("x",cstring(X));
    url.setParam("y",cstring(Y));
    url.setParam("z",cstring(Z));

    auto request=NetRequest(url);

    if (!request.valid())
      return readFailed(query);

    request.aborted=query->aborted;

    auto gotNetResponse=[this,query](NetResponse response)
    {
      NdPoint nsamples=NdPoint::one(2);
      nsamples[0]=dataset->tile_nsamples.x;
      nsamples[1]=dataset->tile_nsamples.y;

      response.setHeader("visus-compression"   , dataset->tile_compression);
      response.setHeader("visus-nsamples"      , nsamples.toString());
      response.setHeader("visus-dtype"         , query->field.dtype.toString());
      response.setHeader("visus-layout"        , "");

      //I want the decoding happens in the 'client' side
      query->setClientProcessing([this,response,query]() 
      {
        if (query->aborted() || !response.isSuccessful()) 
        {
          this->statistics.rfail++;
          return QueryFailed;
        }

        auto decoded=response.getArrayBody();
        if (!decoded)
        {
          this->statistics.rfail++;
          return QueryFailed;
        }

        VisusAssert(decoded.dims==query->nsamples);
        VisusAssert(decoded.dtype==query->field.dtype);
        query->buffer=decoded;

        this->statistics.rok++;
        return QueryOk;
      });

      //done but status not set yet
      query->future.get_promise()->set_value(true);
    };

    if (bool bAsync=this->async.netservice?true:false)
    {
      auto future_response= this->async.netservice->asyncNetworkIO(request);
      future_response.when_ready([this, future_response, query,gotNetResponse]() {
        gotNetResponse(future_response.get());
      });
    }
    else
    {
      gotNetResponse(NetService::getNetResponse(request));
    }
  }

  //writeBlock
  virtual void writeBlock(SharedPtr<BlockQuery> query) override
  {
    VisusAssert(false);//not supported
    writeFailed(query);
  }

  //printStatistics
  virtual void printStatistics() override {
    VisusInfo() << name << " hostname(" << url.getHostname() << ") port(" << url.getPort() << ") compression(" << compression << ") url(" << url.toString() << ")";
    Access::printStatistics();
  }

};


//////////////////////////////////////////////////////////////
bool LegacyDataset::beginQuery(SharedPtr<Query> query) 
{
  if (!Dataset::beginQuery(query))
    return false;

  VisusAssert(query->start_resolution==0);
  VisusAssert(query->max_resolution==this->getBitmask().getMaxResolution());

  //i don't have odd resolutions
  {
    std::vector<int> end_resolutions=query->end_resolutions;
    std::vector<int> even_end_resolutions;
    for (int I=0;I<(int)end_resolutions.size();I++)
    {
      int even_end_resolution=(end_resolutions[I]>>1)<<1;
      if (even_end_resolutions.empty() || even_end_resolutions.back()!=even_end_resolution)
        even_end_resolutions.push_back(even_end_resolution);
    }
    query->end_resolutions=even_end_resolutions;
  }

  //writing is not supported
  if (query->mode=='w')
  {
    query->setFailed("Writing mode not suppoted");
    return false;
  }

  Position position=query->position;

  //not supported
  if (!position.getTransformation().isIdentity())
  {
    query->setFailed("Position has non-identity transformation");
    return false;
  }

  auto user_box= query->position.getNdBox().getIntersection(this->getBox());

  if (!user_box.isFullDim())
  {
    query->setFailed("user_box not valid");
    return false;
  }

  query->setRunning();
  std::vector<int> end_resolutions=query->end_resolutions;
  for (query->query_cursor=0;query->query_cursor<(int)end_resolutions.size();query->query_cursor++)
  {
    if (setCurrentEndResolution(query))
      return true;
  }

  query->setFailed("Cannot find a good initial resolution");
  return false;
}


//////////////////////////////////////////////////////////////
void LegacyDataset::kdTraverse(std::vector< SharedPtr<BlockQuery> >& block_queries,SharedPtr<Query> query,NdBox box,BigInt id,int H,int end_resolution)
{
  if (query->aborted()) 
    return;

  if (!box.getIntersection(query->position.getNdBox()).isFullDim())
    return;

  int samplesperblock=1<<this->getDefaultBitsPerBlock();

  if (H==end_resolution)
  {
    VisusAssert(H % 2==0);
    BigInt start_address=(id-1)*samplesperblock;
    BigInt end_address  =start_address+samplesperblock;
    auto block_query=std::make_shared<BlockQuery>(query->field,query->time,start_address,end_address,query->aborted);
    block_queries.push_back(block_query);
    return;
  }

  DatasetBitmask bitmask=this->getBitmask();
  int split_bit=bitmask[1+H - this->getDefaultBitsPerBlock()];
  NdPoint::coord_t middle=(box.p1[split_bit]+box.p2[split_bit])>>1;

  auto left_box  =box; left_box .p2[split_bit]=middle; 
  auto right_box =box; right_box.p1[split_bit]=middle; 

  kdTraverse(block_queries,query,left_box ,id*2+0,H+1,end_resolution);
  kdTraverse(block_queries,query,right_box,id*2+1,H+1,end_resolution);
}

//////////////////////////////////////////////////////////////
bool LegacyDataset::executeQuery(SharedPtr<Access> access,SharedPtr<Query> query) 
{
  if (!Dataset::executeQuery(access,query))
    return false;

  if (!query->allocateBufferIfNeeded())
  {
    query->setFailed("cannot allocate buffer");
    return false;
  }

  //always need an access.. the google server cannot handle pure remote queries (i.e. compose the tiles on server side)
  if (!access)
    access=std::make_shared<LegacyAccess>(this);  

  int end_resolution=query->getEndResolution();
  VisusAssert(end_resolution % 2==0);

  WaitAsync< Future<bool>, SharedPtr<BlockQuery> > async;

  NdBox box=this->getBox();

  std::vector< SharedPtr<BlockQuery> > block_queries;
  kdTraverse(block_queries,query,box,/*id*/1,/*H*/this->getDefaultBitsPerBlock(),end_resolution);

  access->beginRead();
  {
    for (auto block_query : block_queries)
    {
      async.pushRunning(block_query->future, block_query);
      readBlock(access, block_query);
    }
  }
  access->endRead();

  for (int I=0,N=async.size();I<N;I++)
  {
    auto blockquery=async.popReady().second; VisusAssert(blockquery);
    if (!query->aborted() && blockquery->getStatus()==QueryOk)
      mergeQueryWithBlock(query,blockquery);
  }

  query->currentLevelReady();
  return true;
}

//////////////////////////////////////////////////////////////
bool LegacyDataset::nextQuery(SharedPtr<Query> query) 
{
  if (!Dataset::nextQuery(query))
    return false;

  //merging is not supported
  query->buffer=Array();

  if (!setCurrentEndResolution(query))
  {
    query->setFailed("cannot set end resolution");
    return false;
  }
  else
  {
    return true;
  }
}

//////////////////////////////////////////////////////////////
bool LegacyDataset::mergeQueryWithBlock(SharedPtr<Query> query,SharedPtr<BlockQuery> blockquery) 
{
  return Query::mergeSamples(*query,*blockquery,Query::InsertSamples,query->aborted);
}

//////////////////////////////////////////////////////////////
SharedPtr<Access> LegacyDataset::createAccess(StringTree config, bool bForBlockQuery)
{
  VisusAssert(this->valid());

  if (config.empty())
    config = getDefaultAccessConfig();

  String type = StringUtils::toLower(config.readString("type"));

  //I always need an access
  if (type.empty()) 
    return std::make_shared<LegacyAccess>(this, config);

  //LegacyAccess
  if (type=="legacyaccess")
    return std::make_shared<LegacyAccess>(this, config);

  return Dataset::createAccess(config, bForBlockQuery);
}


//////////////////////////////////////////////////////////////
std::vector<int> LegacyDataset::guessEndResolutions(const Frustum& viewdep,Position position,Query::Quality quality,Query::Progression progression)
{
  std::vector<int> ret=Dataset::guessEndResolutions(viewdep,position,quality,progression);
  for (int I=0;I<(int)ret.size();I++)
    ret[I]=(ret[I]>>1)<<1; //i don't have even resolution 
  return ret;
}

//////////////////////////////////////////////////////////////
Point3i LegacyDataset::getTileCoordinate(BigInt start_address,BigInt end_address)
{
  int bitsperblock=this->getDefaultBitsPerBlock();
  int samplesperblock=((BigInt)1)<<bitsperblock;

  VisusAssert(end_address==start_address+samplesperblock);

  Int64  blocknum=cint64(start_address>>bitsperblock);
  int    H=bitsperblock+Utils::getLog2(1+blocknum);
  VisusAssert((H % 2)==0);
  Int64  first_block_in_level=(((Int64)1)<<(H-bitsperblock))-1;
  NdPoint tile_coord=bitmask.deinterleave(blocknum-first_block_in_level,H-bitsperblock);

  return Point3i(
    (int)(tile_coord[0]),
    (int)(tile_coord[1]),
    (H-bitsperblock)>>1);
}

//////////////////////////////////////////////////////////////
LogicBox LegacyDataset::getAddressRangeBox(BigInt start_address,BigInt end_address)
{
  auto coord=getTileCoordinate(start_address,end_address);

  int X=coord.x;
  int Y=coord.y;
  int Z=coord.z;

  int tile_width =(int)(this->getBox().p2[0])>>Z;
  int tile_height=(int)(this->getBox().p2[1])>>Z;

  NdPoint delta=NdPoint::one(2);
  delta[0]=tile_width /this->tile_nsamples.x;
  delta[1]=tile_height/this->tile_nsamples.y;

  NdBox box(NdPoint(2), NdPoint::one(2));
  box.p1[0] = tile_width  * (X + 0); box.p2[0] = tile_width  * (X + 1);
  box.p1[1] = tile_height * (Y + 0); box.p2[1] = tile_height * (Y + 1);

  return LogicBox(box,delta);

}

//////////////////////////////////////////////////////////////
bool LegacyDataset::openFromUrl(Url url)
{
  this->tile_nsamples.x  = cint(url.getParam("tile_width" ,"0")); 
  this->tile_nsamples.y  = cint(url.getParam("tile_height","0")); 
  int   nlevels          = cint(url.getParam("nlevels","0"))    ; 
  this->tile_compression = url.getParam("compression")     ; 
  this->dtype            = DType::fromString(url.getParam("dtype")); 

  if (tile_nsamples.x<=0 || tile_nsamples.y<=0 || !nlevels || !dtype.valid() || tile_compression.empty())
  {
    VisusAssert(false);
    this->invalidate();
    return false;
  }

  //any google level double the dimensions in x and y (i.e. i don't have even resolutions)
  NdPoint overall_dims=NdPoint::one(2);
  overall_dims[0]=tile_nsamples.x * (((NdPoint::coord_t)1)<<nlevels);
  overall_dims[1]=tile_nsamples.y * (((NdPoint::coord_t)1)<<nlevels);

  this->url=url.toString();
  this->bitmask=DatasetBitmask::guess(overall_dims);
  this->default_bitsperblock=Utils::getLog2(tile_nsamples.x*tile_nsamples.y);
  this->box=NdBox(NdPoint(0,0),overall_dims);
  this->timesteps=DatasetTimesteps();
  this->timesteps.addTimestep(0);

  addField(Field("DATA",dtype));

  //UseQuery not supported? actually yes, but it's a nonsense since a query it's really a block query
  if (this->kdquery_mode==KdQueryMode::UseQuery)
    this->kdquery_mode=KdQueryMode::UseBlockQuery;

  return true;
}



//////////////////////////////////////////////////////////////
LogicBox LegacyDataset::getLevelBox(int H)
{
  int bitsperblock=this->getDefaultBitsPerBlock();
  VisusAssert((H%2)==0 && H>=bitsperblock);
  int Z=(H-bitsperblock)>>1;
    
  int tile_width =(int)(this->getBox().p2[0])>>Z;
  int tile_height=(int)(this->getBox().p2[1])>>Z;
    
  int ntiles_x=(int)(1<<Z);
  int ntiles_y=(int)(1<<Z);

  NdPoint delta=NdPoint::one(2);
  delta[0]=tile_width /this->tile_nsamples.x;
  delta[1]=tile_height/this->tile_nsamples.y;
    
  NdBox box(NdPoint(0,0), NdPoint::one(1,1));
  box.p2[0] = ntiles_x*tile_width;
  box.p2[1] = ntiles_y*tile_height;
    
  auto ret=LogicBox(box,delta);
  VisusAssert(ret.valid());
  return ret;
}

//////////////////////////////////////////////////////////////
bool LegacyDataset::setCurrentEndResolution(SharedPtr<Query> query)
{
  int end_resolution=query->getEndResolution(); 
  if (end_resolution<0) 
    return false;

  VisusAssert(end_resolution % 2==0);

  int max_resolution=query->max_resolution;
    
  //necessary condition
  VisusAssert(query->start_resolution<=end_resolution);
  VisusAssert(end_resolution<=max_resolution);
  
  auto user_box= query->position.getNdBox().getIntersection(this->getBox());
  VisusAssert(user_box.isFullDim());

  int H=end_resolution;

  LogicBox Lbox=getLevelBox(end_resolution);
  NdBox box=Lbox.alignBox(user_box);
  if (!box.isFullDim())
    return false;

  LogicBox logic_box(box,Lbox.delta);
  query->nsamples=logic_box.nsamples;
  query->logic_box=logic_box;
  query->buffer=Array();
  return true;
}

} //namespace Visus

