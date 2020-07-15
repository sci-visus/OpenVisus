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
#include <Visus/GoogleMapsAccess.h>

namespace Visus {


////////////////////////////////////////////////////////////////////
SharedPtr<BlockQuery> GoogleMapsDataset::createBlockQuery(BigInt blockid, Field field, double time, int mode, Aborted aborted)
{
  auto ret = std::make_shared<BlockQuery>();
  ret->dataset = this;
  ret->field = field;
  ret->time = time;
  ret->mode = mode; VisusAssert(mode == 'r' || mode == 'w');
  ret->aborted = aborted;
  ret->blockid = blockid;

  auto bitsperblock = this->getDefaultBitsPerBlock();

  //logic samples
  {
    //Get H from blockid. Example:
    //  bitsperblock=16  
    //  bitmask V010101010101010101010101010101010101010101010101010101010101
    //
    //  blockid=0 H=16+Utils::getLog2(1+0)=16+0=16
    //  blockid=1 H=16+Utils::getLog2(1+1)=16+1=17
    //  blockid=2 H=16+Utils::getLog2(1+2)=16+1=17
    //  blockid=3 H=16+Utils::getLog2(1+3)=16+2=18
    //  ....
    auto H = bitsperblock + Utils::getLog2(1 + blockid);

    //Example:
    // H=bitsperblock+0   first_block_in_level=(1<<0)-1=0
    // H=bitsperblock+1   first_block_in_level=(1<<1)-1=1
    // H=bitsperblock+2   first_block_in_level=(1<<2)-1=3
    Int64 first_block_in_level = (((Int64)1) << (H - bitsperblock)) - 1;

    auto coord = bitmask.deinterleave(blockid - first_block_in_level, H - bitsperblock);
    auto p0 = coord.innerMultiply(block_samples[H].logic_box.size());
    auto p1 = p0 + block_samples[H].logic_box.size();

    ret->H = H;
    ret->logic_samples = LogicSamples(BoxNi(p0, p1) , block_samples[H].delta);
  }

  return ret;
}


//////////////////////////////////////////////////////////////
bool GoogleMapsDataset::setBoxQueryEndResolution(SharedPtr<BoxQuery> query,int value)
{
  VisusAssert(query->end_resolution < value);
  query->end_resolution = value;

  auto end_resolution = query->end_resolution;
  auto user_box = query->logic_box.getIntersection(this->getLogicBox());
  VisusAssert(user_box.isFullDim());

  auto Lsamples = level_samples[end_resolution];
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
  std::set<int> good;
  for (auto it : query->end_resolutions)
  {
    auto value = (it >> 1) << 1;
    good.insert(Utils::clamp(value, getDefaultBitsPerBlock(), getMaxResolution()));
  }

  query->end_resolutions = std::vector<int>(good.begin(), good.end());

  for (auto end_resolution : query->end_resolutions)
  {
    if (setBoxQueryEndResolution(query, end_resolution))
      return query->setRunning();
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

  int index = Utils::find(query->end_resolutions, query->end_resolution);

  if (!setBoxQueryEndResolution(query, query->end_resolutions[index + 1]))
    VisusReleaseAssert(false); //should not happen

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
    query->setFailed("query aborted");
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
    access = createAccessForBlockQuery();

  int end_resolution = query->end_resolution;

  WaitAsync< Future<Void> > wait_async;

  BoxNi box = this->getLogicBox();

  std::stack< std::tuple<BoxNi, BigInt, int> > stack;
  stack.push({ box ,0,this->getDefaultBitsPerBlock() });

  access->beginRead();
  while (!stack.empty() && !query->aborted())
  {
    auto top=stack.top();
    stack.pop();

    auto box     = std::get<0>(top);
    auto blockid = std::get<1>(top);
    auto H       = std::get<2>(top);

    if (!box.getIntersection(query->logic_box).isFullDim())
      continue;

    //is the resolution I need?
    if (H == end_resolution)
    {
      auto block_query = createBlockQuery(blockid, query->field, query->time, 'r', query->aborted);

      executeBlockQuery(access, block_query);
      wait_async.pushRunning(block_query->done).when_ready([this, query, block_query](Void) 
      {
        if (query->aborted() || !block_query->ok())
          return;

        mergeBoxQueryWithBlockQuery(query, block_query);
      });
    }
    else
    {
      int bitsperblock = this->getDefaultBitsPerBlock();
      auto split_bit = bitmask[1 + H - bitsperblock];
      auto middle = (box.p1[split_bit] + box.p2[split_bit]) >> 1;
      auto lbox = box; lbox.p2[split_bit] = middle;
      auto rbox = box; rbox.p1[split_bit] = middle;
      stack.push(std::make_tuple(rbox, blockid * 2 + 2, H + 1));
      stack.push(std::make_tuple(lbox, blockid * 2 + 1, H + 1));
    }
  }
  access->endRead();

  wait_async.waitAllDone();

  if (query->aborted())
  {
    query->setFailed("query aborted"); 
    return false;
  }

  query->setCurrentResolution(query->end_resolution);
  return true;
}

//////////////////////////////////////////////////////////////
bool GoogleMapsDataset::mergeBoxQueryWithBlockQuery(SharedPtr<BoxQuery> query, SharedPtr<BlockQuery> blockquery)
{
  return insertSamples(query->logic_samples, query->buffer, blockquery->logic_samples, blockquery->buffer, query->aborted);
}

//////////////////////////////////////////////////////////////
SharedPtr<Access> GoogleMapsDataset::createAccess(StringTree config, bool bForBlockQuery)
{
  if (!config.valid())
    config = getDefaultAccessConfig();

  String type = StringUtils::toLower(config.readString("type"));

  //I always need an access
  if (type.empty() || type == "GoogleMapsAccess")
  {
    SharedPtr<NetService> netservice;
    if (!bServerMode)
    {
      int nconnections = config.readInt("nconnections", 8);
      netservice = std::make_shared<NetService>(nconnections);
    }
    return std::make_shared<GoogleMapsAccess>(this, netservice);
  }

  return Dataset::createAccess(config, bForBlockQuery);
}


//////////////////////////////////////////////////////////////
void GoogleMapsDataset::readDatasetFromArchive(Archive& ar)
{
  String url = ar.readString("url");

  //example: 22 levels, each tile has resolution 256*256 (==8bit*8bit)
  //bitsperblock=16
  // bitmask will be (22+8==30 '0' and 22+8==30 '1') 
  //V010101010101010101010101010101010101010101010101010101010101

  String dtype = "uint8[3]";
  int nlevels = 22;
  int tile_width, tile_height;

  ar.read("tiles", this->tiles_url, "http://mt1.google.com/vt/lyrs=s");
  ar.read("tile_width" , tile_width,  256);
  ar.read("tile_height", tile_height, 256);

  VisusReleaseAssert(tile_width>0 && tile_height>0);

  //any google level double the dimensions in x and y (i.e. i don't have even resolutions)
  auto W= tile_width  * (((Int64)1)<<nlevels);
  auto H= tile_height * (((Int64)1)<<nlevels);

  this->setDatasetBody(ar);
  this->setKdQueryMode(KdQueryMode::fromString(ar.readString("kdquery")));
  this->bitmask=DatasetBitmask::guess(PointNi(W,H)); 
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

  Field field("DATA", DType::fromString(dtype));
  field.default_compression = "jpg";
  addField(field);

  //UseBoxQuery not supported? actually yes, but it's a nonsense since a query it's really a block query
  if (getKdQueryMode() == KdQueryMode::UseBoxQuery)
    setKdQueryMode(KdQueryMode::UseBlockQuery);

  //layout of levels and blocks
  {
    //auto bitmask = DatasetBitmask::fromString("V0011");

    int pdim = bitmask.getPointDim();
    int bitsperblock = getDefaultBitsPerBlock();
    auto MaxH = bitmask.getMaxResolution();

    level_samples.push_back(LogicSamples(bitmask.getPow2Box(), bitmask.getPow2Dims()));
    block_samples.push_back(LogicSamples(bitmask.getPow2Box(), bitmask.getPow2Dims()));

    auto level_nsamples = PointNi::one(pdim);
    auto block_nsamples = PointNi::one(pdim);
    for (int H = 1; H <= MaxH; H++)
    {
      auto bit = bitmask[H];
      level_nsamples[bit] *= 2;
      block_nsamples[bit] *= 2;

      //bit exit from bitsperblock window
      if (H - bitsperblock > 0)
        block_nsamples[bitmask[H - bitsperblock]] /= 2;

      auto delta = bitmask.getPow2Dims().innerDiv(level_nsamples);

      level_samples.push_back(LogicSamples(bitmask.getPow2Box(), delta));
      block_samples.push_back(LogicSamples(BoxNi(PointNi::zero(pdim), block_nsamples.innerMultiply(delta)),delta));

#if 0
      if (H >= bitsperblock)
      {
        PrintInfo("H", H,
          "level_samples[H].logic_box", level_samples[H].logic_box,
          "level_samples[H].delta", level_samples[H].delta,
          "block_samples[H].logic_box", block_samples[H].logic_box,
          "block_samples[H].delta", block_samples[H].delta);
      }
#endif
    }
  }
}


} //namespace Visus

