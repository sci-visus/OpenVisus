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

#include <Visus/Dataset.h>
#include <Visus/DiskAccess.h>
#include <Visus/MultiplexAccess.h>
#include <Visus/ModVisusAccess.h>
#include <Visus/CloudStorageAccess.h>
#include <Visus/RamAccess.h>
#include <Visus/FilterAccess.h>
#include <Visus/NetService.h>
#include <Visus/StringTree.h>
#include <Visus/Polygon.h>
#include <Visus/IdxFile.h>
#include <Visus/File.h>
#include <Visus/GoogleMapsDataset.h>
#include <Visus/GoogleMapsAccess.h>

namespace Visus {

VISUS_IMPLEMENT_SINGLETON_CLASS(DatasetFactory)



////////////////////////////////////////////////////////////////////
SharedPtr<Access> Dataset::createAccess(StringTree config,bool bForBlockQuery)
{
  if (!config.valid())
    config = getDefaultAccessConfig();

  String type =StringUtils::toLower(config.readString("type"));

  //I always need an access
  if (auto google = dynamic_cast<GoogleMapsDataset*>(this))
  {
    if (type.empty() || type == "GoogleMapsAccess")
    {
      SharedPtr<NetService> netservice;
      if (!bServerMode)
      {
        int nconnections = config.readInt("nconnections", 8);
        netservice = std::make_shared<NetService>(nconnections);
      }
      return std::make_shared<GoogleMapsAccess>(this, google->tiles_url, netservice);
    }
  }

  if (!config.valid()) {
    VisusAssert(!bForBlockQuery);
    return SharedPtr<Access>(); //pure remote query
  }

  if (type.empty()) {
    VisusAssert(false); //please handle this case in your dataset since I dont' know how to handle this situation here
    return SharedPtr<Access>();
  }

  //DiskAccess
  if (type=="diskaccess")
    return std::make_shared<DiskAccess>(this, config);

  // MULTIPLEX 
  if (type=="multiplex" || type=="multiplexaccess")
    return std::make_shared<MultiplexAccess>(this, config);
  
  // RAM CACHE 
  if (type == "lruam" || type == "ram" || type == "ramaccess")
  {
    auto ret = std::make_shared<RamAccess>(getDefaultBitsPerBlock());
    ret->can_read = StringUtils::contains(config.readString("chmod", Access::DefaultChMod), "r");
    ret->can_write = StringUtils::contains(config.readString("chmod", Access::DefaultChMod), "w");
    ret->setAvailableMemory(StringUtils::getByteSizeFromString(config.readString("available", "128mb")));
    return ret;
  }

  // NETWORK 
  if (type=="network" || type=="modvisusaccess")
    return std::make_shared<ModVisusAccess>(this, config);

  //CloudStorageAccess
  if (type=="cloudstorageaccess")
    return std::make_shared<CloudStorageAccess>(this, config);
  
  // FILTER 
  if (type=="filter" || type=="filteraccess")
    return std::make_shared<FilterAccess>(this, config);

  //problem here
  VisusAssert(false);
  return SharedPtr<Access>();
}




///////////////////////////////////////////////////////////
Field Dataset::getFieldEx(String fieldname) const
{
  //remove any params (they will be used in queries)
  ParseStringParams parse(fieldname);

  auto it=find_field.find(parse.without_params);
  if (it!=find_field.end())
  {
    Field ret=it->second;
    ret.name=fieldname; //important to keep the params! example "temperature?time=30"
    ret.params=parse.params;
    return ret;
  }

  //not found
  return Field();
}

///////////////////////////////////////////////////////////
Field Dataset::getField(String name) const {
  try {
    return getFieldEx(name);
  }
  catch (std::exception ex) {
    return Field();
  }
}



////////////////////////////////////////////////////////////////////////////////////
StringTree FindDatasetConfig(StringTree ar, String url)
{
  auto all_datasets = ar.getAllChilds("dataset");
  for (auto it : all_datasets)
  {
    if (it->readString("name") == url) {
      VisusAssert(it->hasAttribute("url"));
      return *it;
    }
  }

  for (auto it : all_datasets)
  {
    if (it->readString("url") == url)
      return *it;
  }

  auto ret = StringTree("dataset");
  ret.write("url", url);
  return ret;
}


/////////////////////////////////////////////////////////////////////////
SharedPtr<Dataset> LoadDatasetEx(StringTree ar)
{
  String url = ar.readString("url");
  if (!Url(url).valid())
    ThrowException("LoadDataset", url, "failed. Not a valid url");

  auto content = Utils::loadTextDocument(url);

  if (content.empty())
    ThrowException("empty content");

  //enrich ar by loaded document (ar has precedence)
  auto doc = StringTree::fromString(content);
  if (doc.valid())
  {
    //backward compatible
    if (doc.name == "midx")
    {
      doc.name = "dataset";
      doc.write("typename","IdxMultipleDataset");
    }

    StringTree::merge(ar, doc); //example <dataset tyname="IdxMultipleDataset">...</dataset>
    VisusReleaseAssert(ar.hasAttribute("typename"));
  }
  else
  {
    // backward compatible, old idx text format 
    ar.write("typename", "IdxDataset");

    IdxFile old_format;
    old_format.readFromOldFormat(content);
    ar.writeObject("idxfile", old_format);
  }

  auto TypeName = ar.getAttribute("typename");
  VisusReleaseAssert(!TypeName.empty());
  auto ret= DatasetFactory::getSingleton()->createInstance(TypeName);
  if (!ret)
    ThrowException("LoadDataset",url,"failed. Cannot DatasetFactory::getSingleton()->createInstance",TypeName);

  ret->readDatasetFromArchive(ar);
  return ret; 
}


////////////////////////////////////////////////
SharedPtr<Dataset> LoadDataset(String url) {
  auto ar = FindDatasetConfig(*DbModule::getModuleConfig(), url);
  return LoadDatasetEx(ar);
}


////////////////////////////////////////////////
SharedPtr<BoxQuery> Dataset::createBoxQuery(BoxNi logic_box, Field field, double time, int mode, Aborted aborted)
{
  auto ret = std::make_shared<BoxQuery>();
  ret->dataset = this;
  ret->field = field;
  ret->time = time;
  ret->mode = mode; VisusAssert(mode == 'r' || mode == 'w');
  ret->aborted = aborted;
  ret->logic_box = logic_box;
  ret->filter.domain = this->getLogicBox();
  return ret;
}


////////////////////////////////////////////////
std::vector<int> Dataset::guessBoxQueryEndResolutions(Frustum logic_to_screen, Position logic_position, int quality, int progression)
{
  if (!logic_position.valid())
    return {};

  auto maxh = getMaxResolution();
  auto endh = maxh;
  auto pdim = getPointDim();

  if (logic_to_screen.valid())
  {
    //important to work with orthogonal box
    auto logic_box = logic_position.toAxisAlignedBox();

    FrustumMap map(logic_to_screen);

    std::vector<Point2d> screen_points;
    for (auto p : logic_box.getPoints())
      screen_points.push_back(map.projectPoint(p.toPoint3()));

    //project on the screen
    std::vector<double> screen_distance = { 0,0,0 };

    for (auto edge : BoxNi::getEdges(pdim))
    {
      auto axis = edge.axis;
      auto s0 = screen_points[edge.index0];
      auto s1 = screen_points[edge.index1];
      auto Sd = s0.distance(s1);
      screen_distance[axis] = std::max(screen_distance[axis], Sd);
    }

    const int max_3d_texture_size = 2048;

    auto nsamples = logic_box.size().toPoint3();
    while (endh > 0)
    {
      std::vector<double> samples_per_pixel = {
        nsamples[0] / screen_distance[0],
        nsamples[1] / screen_distance[1],
        nsamples[2] / screen_distance[2]
      };

      std::sort(samples_per_pixel.begin(), samples_per_pixel.end());

      auto quality = sqrt(samples_per_pixel[0] * samples_per_pixel[1]);

      //note: in 2D samples_per_pixel[2] is INF; in 3D with an ortho view XY samples_per_pixel[2] is INF (see std::sort)
      bool bGood = quality < 1.0;

      if (pdim == 3 && bGood)
        bGood =
        nsamples[0] <= max_3d_texture_size &&
        nsamples[1] <= max_3d_texture_size &&
        nsamples[2] <= max_3d_texture_size;

      if (bGood)
        break;

      //by decreasing resolution I will get half of the samples on that axis
      auto bit = bitmask[endh];
      nsamples[bit] *= 0.5;
      --endh;
    }
  }

  //consider quality and progression
  endh = Utils::clamp(endh + quality, 0, maxh);

  std::vector<int> ret = { Utils::clamp(endh - progression, 0, maxh) };
  while (ret.back() < endh)
    ret.push_back(Utils::clamp(ret.back() + pdim, 0, endh));

  if (auto google = dynamic_cast<GoogleMapsDataset*>(this))
  {
    for (auto& it : ret)
      it = (it >> 1) << 1; //TODO: google maps does not have odd resolutions
  }

  return ret;
}


////////////////////////////////////////////////////////////////////
SharedPtr<BlockQuery> Dataset::createBlockQuery(BigInt blockid, Field field, double time, int mode, Aborted aborted)
{
  VisusReleaseAssert(bBlocksAreFullRes);

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
    ret->logic_samples = LogicSamples(BoxNi(p0, p1), block_samples[H].delta);
  }

  return ret;
}

////////////////////////////////////////////////
void Dataset::executeBlockQuery(SharedPtr<Access> access,SharedPtr<BlockQuery> query)
{
  VisusAssert(access->isReading() || access->isWriting());

  int mode = query->mode; 
  auto failed = [&](String reason) {

    if (!access)
      query->setFailed();
    else
      mode == 'r'? access->readFailed(query) : access->writeFailed(query);
   
    PrintInfo("executeBlockQUery failed", reason);
    return;
  };

  if (!access)
    return failed("no access");

  if (!query->field.valid())
    return failed("field not valid");

  if (query->blockid < 0)
    return failed("address range not valid");

  if ((mode == 'r' && !access->can_read) || (mode == 'w' && !access->can_write))
    return failed("rw not enabled");

  if (!query->logic_samples.valid())
    return failed("logic_samples not valid");

  if (mode == 'w' && !query->buffer.valid())
    return failed("no buffer to write");

  // override time  from from field
  if (query->field.hasParam("time"))
    query->time = cdouble(query->field.getParam("time"));

  query->setRunning();

  if (mode == 'r')
  {
    access->readBlock(query);
    BlockQuery::readBlockEvent();
  }
  else
  {
    access->writeBlock(query);
    BlockQuery::writeBlockEvent();
  }

  return;
}


////////////////////////////////////////////////////////////////////////////////////
bool Dataset::insertSamples(
  LogicSamples Wsamples, Array Wbuffer,
  LogicSamples Rsamples, Array Rbuffer,
  Aborted aborted)
{
  if (!Wsamples.valid() || !Rsamples.valid())
    return false;

  if (Wbuffer.dtype != Rbuffer.dtype || Wbuffer.dims != Wsamples.nsamples || Rbuffer.dims != Rsamples.nsamples)
  {
    VisusAssert(false);
    return false;
  }

  //cannot find intersection (i.e. no sample to merge)
  BoxNi box = Wsamples.logic_box.getIntersection(Rsamples.logic_box);
  if (!box.isFullDim())
    return false;

  /*
  Example of the problem to solve:

  Wsamples:=-2 + kw*6         -2,          4,          10,            *16*,            22,            28,            34,            40,            *46*,            52,            58,  ...)
  Rsamples:=-4 + kr*5    -4,         1,        6,         11,         *16*,          21,       25,            ,31         36,          41,         *46*,          51,         56,       ...)

  give kl,kw,kr independent integers all >=0

  leastCommonMultiple(2,6,5)= 2*3*5 =30

  First "common" value (i.e. minimum value satisfying all 3 conditions) is 16
  */

  int pdim = Rbuffer.getPointDim();
  PointNi delta(pdim);
  for (int D = 0; D < pdim; D++)
  {
    Int64 lcm = Utils::leastCommonMultiple(Rsamples.delta[D], Wsamples.delta[D]);

    Int64 P1 = box.p1[D];
    Int64 P2 = box.p2[D];

    while (
      !Utils::isAligned(P1, Wsamples.logic_box.p1[D], Wsamples.delta[D]) ||
      !Utils::isAligned(P1, Rsamples.logic_box.p1[D], Rsamples.delta[D]))
    {
      //NOTE: if the value is already aligned, alignRight does nothing
      P1 = Utils::alignRight(P1, Wsamples.logic_box.p1[D], Wsamples.delta[D]);
      P1 = Utils::alignRight(P1, Rsamples.logic_box.p1[D], Rsamples.delta[D]);

      //cannot find any alignment, going beyond the valid range
      if (P1 >= P2)
        return false;

      //continue in the search IIF it's not useless
      if ((P1 - box.p1[D]) >= lcm)
      {
        //should be acceptable to be here, it just means that there are no samples to merge... 
        //but since 99% of the time Visus has pow-2 alignment it is high unlikely right now... adding the VisusAssert just for now
        VisusAssert(false);
        return false;
      }
    }

    delta[D] = lcm;
    P2 = Utils::alignRight(P2, P1, delta[D]);
    box.p1[D] = P1;
    box.p2[D] = P2;
  }

  VisusAssert(box.isFullDim());

  auto wfrom = Wsamples.logicToPixel(box.p1); auto wto = Wsamples.logicToPixel(box.p2); auto wstep = delta.rightShift(Wsamples.shift); 
  auto rfrom = Rsamples.logicToPixel(box.p1); auto rto = Rsamples.logicToPixel(box.p2); auto rstep = delta.rightShift(Rsamples.shift);

  VisusAssert(PointNi::max(wfrom, PointNi(pdim)) == wfrom); 
  VisusAssert(PointNi::max(rfrom, PointNi(pdim)) == rfrom); 

  wto = PointNi::min(wto, Wbuffer.dims); wstep = PointNi::min(wstep, Wbuffer.dims);
  rto = PointNi::min(rto, Rbuffer.dims); rstep = PointNi::min(rstep, Rbuffer.dims);

  //first insert samples in the right position!
  if (!ArrayUtils::insert(Wbuffer, wfrom, wto, wstep, Rbuffer, rfrom, rto, rstep, aborted))
    return false;

  return true;
}



//////////////////////////////////////////////////////////////
void Dataset::beginBoxQuery(SharedPtr<BoxQuery> query)
{
  VisusReleaseAssert(bBlocksAreFullRes);

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

  //google has only even resolution
  if (auto google = dynamic_cast<GoogleMapsDataset*>(this))
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
    if (setBoxQueryEndResolution(query, end_resolution))
      return query->setRunning();
  }

  query->setFailed();
}



//////////////////////////////////////////////////////////////
bool Dataset::executeBoxQuery(SharedPtr<Access> access, SharedPtr<BoxQuery> query)
{
  VisusReleaseAssert(bBlocksAreFullRes);

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
    auto top = stack.top();
    stack.pop();

    auto box = std::get<0>(top);
    auto blockid = std::get<1>(top);
    auto H = std::get<2>(top);

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
bool Dataset::mergeBoxQueryWithBlockQuery(SharedPtr<BoxQuery> query, SharedPtr<BlockQuery> blockquery)
{
  return insertSamples(query->logic_samples, query->buffer, blockquery->logic_samples, blockquery->buffer, query->aborted);
}

//////////////////////////////////////////////////////////////
void Dataset::nextBoxQuery(SharedPtr<BoxQuery> query)
{
  VisusReleaseAssert(bBlocksAreFullRes);

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
bool Dataset::setBoxQueryEndResolution(SharedPtr<BoxQuery> query, int value)
{
  VisusReleaseAssert(bBlocksAreFullRes);

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

/// ////////////////////////////////////////////////////////////////
void Dataset::readDatasetFromArchive(Archive& ar)
{
  VisusAssert(bBlocksAreFullRes);

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
    block_samples.push_back(LogicSamples(BoxNi(PointNi::zero(pdim), block_nsamples.innerMultiply(delta)), delta));

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



} //namespace Visus 
