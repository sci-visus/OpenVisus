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
#include <Visus/VisusConfig.h>


namespace Visus {

VISUS_IMPLEMENT_SINGLETON_CLASS(DatasetFactory)



/////////////////////////////////////////////////////////////////////////////
std::vector<int> Dataset::guessEndResolutions(const Frustum& viewdep,Position position,Query::Quality quality,Query::Progression progression)
{
  int dataset_dim = this->getPointDim();

  if (progression==Query::GuessProgression)
    progression=(Query::Progression)(dataset_dim == 2 ? dataset_dim * 3 : dataset_dim * 4);

  int final_resolution  = this->getMaxResolution();
  
  // valerio's algorithm, find the final view dependent resolution (endh)
  // (the default endh is the maximum resolution available)
  if (viewdep.valid())
  {
    const int unit_box_edges[12][2]=
    {
      {0,1}, {1,2}, {2,3}, {3,0},
      {4,5}, {5,6}, {6,7}, {7,4},
      {0,4}, {1,5}, {2,6}, {3,7}
    };


    std::vector<Point3d> logic_points;
    for (int I=0;I<8;I++)
      logic_points.push_back(position.getTransformation() * position.getBox().getPoint(I));

    std::vector<Point2d> screen_points;
    FrustumMap map(viewdep);
    for (int I=0;I<8;I++)
      screen_points.push_back(map.projectPoint(logic_points[I]));
  
    int    longest_edge_on_screen           =-1;
    double longest_pixel_distance_on_screen = 0;
    for (int E=0;E<12;E++)
    {
      Point2d p1=screen_points[unit_box_edges[E][0]];
      Point2d p2=screen_points[unit_box_edges[E][1]];
      double pixel_distance_on_screen=(p2-p1).module();

      if (longest_edge_on_screen==-1 || pixel_distance_on_screen>longest_pixel_distance_on_screen)
      {
        longest_edge_on_screen=E;
        longest_pixel_distance_on_screen=pixel_distance_on_screen;
      }
    }

    //I match the highest resolution on dataset axis (it's just an euristic!)
    DatasetBitmask bitmask=this->getBitmask();
    Point3d logic_P1 = logic_points[unit_box_edges[longest_edge_on_screen][0]];
    Point3d logic_P2 = logic_points[unit_box_edges[longest_edge_on_screen][1]];
    for (int dataset_axis=0;dataset_axis<3;dataset_axis++)
    {
      double logic_distance=fabs(logic_P1[dataset_axis]-logic_P2[dataset_axis]);
      double factor =logic_distance/longest_pixel_distance_on_screen;
      Int64  num=Utils::getPowerOf2((Int64)factor);
      while (num>factor) 
        num>>=1;
      
      int H=this->getMaxResolution();
      for (;num>1 && H>=0;H--)
      {
        if (bitmask[H]==dataset_axis) 
          num>>=1;
      }

      final_resolution=std::min(final_resolution,H);
    }  
  }

  //quality
  final_resolution=std::min(getMaxResolution(),quality+final_resolution);

  std::vector<int> ret;
  ret.push_back(std::max(0,final_resolution-progression)); 
  
  while (ret.back() != final_resolution)
    ret.push_back(std::min(final_resolution, ret.back() + dataset_dim));

  return ret;
}


////////////////////////////////////////////////////////////////////
SharedPtr<Access> Dataset::createRamAccess(Int64 available, bool can_read, bool can_write)
{
  auto ret = std::make_shared<RamAccess>();

  ret->name      = "RamAccess";
  ret->can_read  = can_read;
  ret->can_write = can_write;
  ret->bitsperblock = this->getDefaultBitsPerBlock();

  if (this->ram_access)
  {
    ret->shareMemoryWith(this->ram_access);
  }
  else
  {
    ret->setAvailableMemory(available);
    this->ram_access = ret;
  }

  return ret;
}

////////////////////////////////////////////////////////////////////
SharedPtr<Access> Dataset::createAccess(StringTree config,bool bForBlockQuery)
{
  VisusAssert(this->valid());

  if (config.empty())
    config = getDefaultAccessConfig();

  if (config.empty()) {
    VisusAssert(!bForBlockQuery);
    return SharedPtr<Access>();
  }

  String type =StringUtils::toLower(config.readString("type"));

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
    auto available = StringUtils::getByteSizeFromString(config.readString("available", "128mb"));
    auto can_read = StringUtils::contains(config.readString("chmod", "rw"), "r");
    auto can_write = StringUtils::contains(config.readString("chmod", "rw"), "w");
    return createRamAccess(available, can_read, can_write);
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
Field Dataset::getFieldByNameThrowEx(String fieldname) const
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


////////////////////////////////////////////////////////////////////////////////////
static StringTree* FindDataset(String name, const StringTree& stree)
{
  auto all_datasets = stree.findAllChildsWithName("dataset");
  for (auto it : all_datasets)
  {
    if (it->readString("name") == name)
      return it;
  }

  for (auto it : all_datasets)
  {
    if (it->readString("url") == name)
      return it;
  }

  return nullptr;
}


/////////////////////////////////////////////////////////////////////////
SharedPtr<Dataset> LoadDatasetEx(String name,StringTree config)
{
  if (name.empty())
    return SharedPtr<Dataset>();

  auto it=FindDataset(name, config);

  Url url(it? it->readString("url") : name);
  if (!url.valid())
  {
    VisusWarning() << "LoadDataset(" << name << ") failed. Not a valid url";
    return SharedPtr<Dataset>();
  }

  String TypeName;

  //local
  if (url.isFile())
  {
    String extension = Path(url.getPath()).getExtension();
    TypeName = DatasetFactory::getSingleton()->getDatasetTypeNameFromExtension(extension);

    //probably not even an idx dataset
    if (TypeName.empty())
      return SharedPtr<Dataset>();
  }
  else if (StringUtils::contains(url.toString(), "mod_visus"))
  {
    url.setParam("action", "readdataset");

    auto response = NetService::getNetResponse(url);
    if (!response.isSuccessful())
    {
      VisusWarning() << "LoadDataset(" << url.toString() << ") failed errormsg(" << response.getErrorMessage() << ")";
      return SharedPtr<Dataset>();
    }

    TypeName = response.getHeader("visus-typename", "IdxDataset");
    if (TypeName.empty())
    {
      VisusWarning() << "LoadDataset(" << url.toString() << ") failed. Got empty TypeName";
      return SharedPtr<Dataset>();
    }
  }
  //legacy dataset (example google maps)
  else if (StringUtils::endsWith(url.getHostname(), ".google.com"))
  {
    TypeName = "GoogleMapsDataset";
  }
  //cloud storage
  else
  {
    TypeName = "IdxDataset"; //using cloud storage only for IDX dataset (in fact MultiDataset must have some run-time processing)
  }

  // backward compatible 
  if (TypeName == "MultipleDataset")
    TypeName = "IdxMultipleDataset";

  auto ret= DatasetFactory::getSingleton()->createInstance(TypeName);
  if (!ret) 
  {
    VisusWarning()<<"LoadDatasetEx("<<url.toString()<<") failed. Cannot DatasetFactory::getSingleton()->createInstance("<<TypeName<<")";
    return SharedPtr<Dataset>();
  }

  ret->url = url;
  ret->config = it? *it :StringTree();
  ret->kdquery_mode = KdQueryMode::fromString(ret->config.readString("kdquery", url.getParam("kdquery")));

  if (!ret->openFromUrl(url.toString())) 
  {
    VisusWarning()<<TypeName<<"::openFromUrl("<<url.toString()<<") failed";
    return SharedPtr<Dataset>();
  }

  //VisusInfo()<<ret->getDatasetInfos();
  return ret; 
}

////////////////////////////////////////////////////////////////////////
String Dataset::getDatasetInfos() const
{
  std::ostringstream out;

  int bitsperblock=getDefaultBitsPerBlock();

  BigInt total_number_of_samples = ((BigInt)1)<<bitmask.getMaxResolution();
  BigInt total_number_of_blocks  = total_number_of_samples>>bitsperblock;

  out<<"Visus file infos                                         "<<std::endl;
  out<<"  Bounds                                                 "<< getBox().toOldFormatString()<<std::endl;
  out<<"  Pow2 dims                                              "<<getBitmask().getPow2Dims().toString()<<std::endl;
  out<<"  number of samples                                      "<<total_number_of_samples<<std::endl;
  out<<"  number of blocks                                       "<<total_number_of_blocks<<std::endl;
  out<<"  timesteps                                              "<<std::endl<<getTimesteps().toString()<<std::endl;
  out<<"  bitmask                                                "<<bitmask.toString()<<std::endl;
  out<<"  resolution range                                       [0,"<<bitmask.getMaxResolution()<<"]"<<std::endl;

  out<<"  Fields:"<<std::endl;
  for (auto field : this->fields)
    out<<"    Field "<<field.name<< " dtype("<<field.dtype.toString()<<")"<<std::endl;

  out << "Inner datasets";
  for (auto it : getInnerDatasets())
    out << it.first<<" "<<std::endl<<it.second->getDatasetInfos() << std::endl;
  return out.str();

  return out.str();
}

////////////////////////////////////////////////
Future<Void> Dataset::readBlock(SharedPtr<Access> access,SharedPtr<BlockQuery> query)
{
  VisusAssert(access->isReading());

  auto failed = [&]() {
    if (!access)
      query->setFailed();
    else
      access->readFailed(query);

    return query->done;
  };

  if (!access)
    return failed();

  if (!query->field.valid())
    return failed();

  if (!(query->start_address < query->end_address))
    return failed();

  if (!access->can_read)
    return failed();

  auto logic_box = getAddressRangeBox(query->start_address, query->end_address);
  if (!logic_box.valid())
    return failed();

  query->nsamples = logic_box.nsamples;
  query->logic_box = logic_box;

  //auto allocate buffer
  if (!query->allocateBufferIfNeeded())
    return failed();

  //override time 
  {
    // dataset url
    Url url = this->getUrl();
    if (url.hasParam("time"))
      query->time = cdouble(url.getParam("time"));

    // from field
    if (query->field.hasParam("time"))
      query->time = cdouble(query->field.getParam("time"));
  }

  query->setRunning();
  access->readBlock(query);
  return query->done;
}

////////////////////////////////////////////////
Future<Void> Dataset::writeBlock(SharedPtr<Access> access, SharedPtr<BlockQuery> query)
{
  VisusAssert(access->isWriting());

  auto failed = [&]() {
    if (!access)
      query->setFailed();
    else
      access->readFailed(query);
    return query->done;
  };

  if (!access)
    return failed();

  if (!query->field.valid())
    return failed();

  if (!(query->start_address < query->end_address))
    return failed();

  if (!access->can_write)
    return failed();

  if (!query->buffer)
    return failed();

  auto logic_box = getAddressRangeBox(query->start_address, query->end_address);
  if (!logic_box.valid())
    return failed();

  query->nsamples = logic_box.nsamples;
  query->logic_box = logic_box;

  //check buffer
  if (!query->allocateBufferIfNeeded())
    return failed();

  //override time
  {
    //from dataset url
    Url url = this->getUrl();
    if (url.hasParam("time"))
      query->time = cdouble(url.getParam("time"));

    //from field
    if (query->field.hasParam("time"))
      query->time = cdouble(query->field.getParam("time"));
  }

  query->setRunning();
  access->writeBlock(query);
  return query->done;
}

////////////////////////////////////////////////
std::vector<NdBox> Dataset::generateTiles(int TileSize) const
{
  auto pdim = this->getPointDim();
  auto WindowSize = NdPoint::one(pdim);
  for (int D = 0; D < pdim; D++)
    WindowSize[D] = TileSize;

  auto box = this->getBox();

  auto Tot = NdPoint::one(pdim);
  for (int D = 0; D < pdim; D++)
    Tot[D] = (Utils::alignRight(box.p2[D], box.p1[D], WindowSize[D]) - box.p1[D]) / WindowSize[D];

  std::vector<NdBox> ret;
  for (auto P = ForEachPoint(box.p1, box.p2, WindowSize); !P.end(); P.next())
  {
    auto tile = NdBox(P.pos, P.pos + WindowSize).getIntersection(this->getBox());

    if (!tile.valid()) {
      VisusAssert(false);
      continue;
    }

    ret.push_back(tile);
  }
  return ret;
}

////////////////////////////////////////////////
Array Dataset::readFullResolutionData(SharedPtr<Access> access, Field field, double time, NdBox box)
{
  if (box == NdBox())
    box = this->box;

  auto query = std::make_shared<Query>(this, 'r');

  query->time = time;
  query->field = field;
  query->position = box;
  query->max_resolution = getMaxResolution();
  query->end_resolutions = { query->max_resolution };

  if (!beginQuery(query))
    return Array();

  if (!executeQuery(access, query))
    return Array();

  return query->buffer;
}

////////////////////////////////////////////////
bool Dataset::writeFullResolutionData(SharedPtr<Access> access, Field field, double time, Array buffer, NdBox box)
{
  if (box==NdBox()) 
    box=NdBox(NdPoint(buffer.getPointDim()), buffer.dims);

  auto query = std::make_shared<Query>(this, 'w');

  query->time = time;
  query->field = field;
  query->position = box;
  query->start_resolution = 0;
  query->max_resolution = this->getMaxResolution();
  query->end_resolutions = { query->max_resolution };

  if (!beginQuery(query))
    return false;

  VisusAssert(query->nsamples == buffer.dims);
  query->buffer = buffer;

  if (!executeQuery(access, query))
    return false;

  return true;
}

////////////////////////////////////////////////
bool Dataset::beginQuery(SharedPtr<Query> query)
{
  if (!query)
    return false;

  if (!query->canBegin())
  {
    query->setFailed("query begin called many times");
    return false;
  }

  //if you want to set a buffer for 'w' queries, please do it after begin
  VisusAssert(!query->logic_box.valid() && !query->buffer);

  if (!this->valid() )
  {
    query->setFailed("query not valid");
    return false;
  }

  if (!query->field.valid())
  {
    query->setFailed("field not valid");
    return false;
  }

  if (!query->position.valid())
  {
    query->setFailed("position not valid");
    return false;
  }

  //if (query->viewdep && !query->viewdep->valid())
  //{
  //  query->setFailed("viewdep not valid");
  //  return false;
  //}

  //override time
  {
    // from field
    if (query->field.hasParam("time"))
      query->time=cdouble(query->field.getParam("time"));  

    //from dataset url
    if (this->getUrl().hasParam("time"))
      query->time=cdouble(this->getUrl().getParam("time"));
  }


  if (!getTimesteps().containsTimestep(query->time))
  {
    query->setFailed("missing timestep");
    return false;
  }

  if (query->end_resolutions.empty())
    query->end_resolutions={this->getMaxResolution()};

  #ifdef VISUS_DEBUG
  if (!this->getBitmask().hasRegExpr())
  {  
    for (int I=0;I<(int)query->end_resolutions.size();I++)
      VisusAssert(query->end_resolutions[I]<=this->getMaxResolution());
  }
  #endif

  return true;
}

////////////////////////////////////////////////
bool Dataset::executeQuery(SharedPtr<Access> access,SharedPtr<Query> query)
{
  if (!query)
    return false;

  if (!query->canExecute())
  {
    query->setFailed("query is in non-executable status");
    return false;
  }

  if (query->aborted()) 
  {
    query->setFailed("query aboted");
    return false;
  }

  //for 'r' queries I can postpone the allocation
  if (query->mode=='w' && !query->buffer)
  {
    query->setFailed("write buffer not set");
    return false;
  }

  return true;
}

////////////////////////////////////////////////
bool Dataset::nextQuery(SharedPtr<Query> query)
{
  if (!query)
    return false;

  if (!query->canNext())
  {
    query->setFailed("query cannot next");
    return false;
  }

  if (query->aborted())
  {
    query->setFailed("query aborted");
    return false;
  }

  VisusAssert(query->isRunning());
  ++query->query_cursor;

  //reached the end?
  if (query->query_cursor==query->end_resolutions.size())
  {
    query->setOk();
    return false;
  }
  //continue running
  else
  {
    return true;
  }
}

////////////////////////////////////////////////
bool Dataset::executePureRemoteQuery(SharedPtr<Query> query)
{
  auto request =createPureRemoteQueryNetRequest(query);
  auto response=NetService::getNetResponse(request);

  if (!response.isSuccessful())
  {
    query->setFailed((StringUtils::format()<<"network request failed errormsg("<<response.getErrorMessage()<<")").str());
    return false;
  }

  auto buffer=response.getArrayBody();
  if (!buffer) {
    query->setFailed((StringUtils::format()<<"failed to decode body").str());
    return false;
  }

  VisusAssert(buffer.dims==query->nsamples);
  query->buffer=buffer;
  query->currentLevelReady();
  return true;
}

////////////////////////////////////////////////
void Dataset::copyDataset(Dataset* Wvf, SharedPtr<Access> Waccess, Field Wfield, double Wtime,
                          Dataset* Rvf, SharedPtr<Access> Raccess, Field Rfield, double Rtime)
{
  if (Rfield.dtype!=Wfield.dtype)
    ThrowException(StringUtils::format()<<"Rfield("<<Rfield.name<<") and Wfield("<<Wfield.name<<") have different dtype");

  if ((1<<Raccess->bitsperblock)!=(1<<Waccess->bitsperblock))
    ThrowException("nsamples per block of source and dest are not equal");

  Time T1=Time::now();
  Time t1=T1;

  VisusInfo()<<"Dataset::copyDataset";
  VisusInfo()<<"  Destination Wurl("+Wvf->getUrl().toString() + ") Wfield("+Wfield.name+") Wtime("+cstring(Wtime)+")";
  VisusInfo()<<"  Source      Rurl("+Rvf->getUrl().toString() + ") Rfield("+Rfield.name+") Rtime("+cstring(Rtime)+")";

  auto num_blocks=std::min(
    Wvf->getTotalnumberOfBlocks(),
    Rvf->getTotalnumberOfBlocks());

  Aborted aborted;

  Raccess->beginRead();
  Waccess->beginWrite();

  for (BigInt block_id=0; block_id<num_blocks; block_id++)
  {
    //progress
    if (t1.elapsedSec()>5)
    {
      auto perc=(100.0*block_id)/(double)num_blocks;
      VisusInfo()<<"block_id("<<block_id<<"/"<<num_blocks<<") "<<perc<<"%";
      t1=Time::now();
    }

    //don't care, could be the block missing
    auto read_block = std::make_shared<BlockQuery>(Rfield, Rtime, Raccess->getStartAddress(block_id), Raccess->getEndAddress(block_id), aborted);

    if (!Rvf->readBlockAndWait(Raccess, read_block))
      continue; 

    auto write_block = std::make_shared<BlockQuery>(Wfield, Wtime, Waccess->getStartAddress(block_id), Waccess->getEndAddress(block_id), aborted);
    write_block->buffer = read_block->buffer;

    if (!Wvf->writeBlockAndWait(Waccess, write_block))
    {
      VisusInfo()<<"FAILED to write block("+cstring(block_id)<<")";
      continue;
    }
  }

  Raccess->endRead();
  Waccess->endWrite();

  VisusInfo()<<"Done in "<<T1.elapsedSec()<< "sec";
}


/////////////////////////////////////////////////////////
Array Dataset::extractLevelImage(SharedPtr<Access> access, Field field, double time, int H)
{
  VisusAssert(access);

  auto pdim = this->getPointDim();
  auto maxh = this->getMaxResolution();
  auto bitmask = this->getBitmask();
  auto bitsperblock = access->bitsperblock;
  auto nsamplesperblock = 1 << bitsperblock;

  VisusAssert(H >= bitsperblock && H <= maxh);

  LogicBox level_box;

  if (H == bitsperblock)
    level_box = this->getAddressRangeBox(0, nsamplesperblock);
  else
    level_box = this->getLevelBox(H);

  auto start_block = (H == bitsperblock) ? 0 : (1 << (H - bitsperblock - 1));
  auto block_per_level = std::max(1, start_block);

  Array ret(level_box.nsamples, DTypes::UINT8_RGB);
  ret.fillWithValue(0);

  access->beginRead();

  for (int block = start_block; block < (start_block + block_per_level); block++)
  {
    auto hzfrom = block << bitsperblock;
    auto hzto = hzfrom + nsamplesperblock;

    auto block_query = std::make_shared<BlockQuery>(field, time, hzfrom, hzto, Aborted());

    auto block_box = getAddressRangeBox(hzfrom,hzto);
    auto p1 = level_box.logicToPixel(block_box.p1);

    this->readBlockAndWait(access, block_query);

    if (block_query->failed())
      continue;

    //make sure is row major
    if (!block_query->buffer.layout.empty())
      convertBlockQueryToRowMajor(block_query);

    auto src = block_query->buffer;

    ArrayUtils::saveImage(StringUtils::format() << "temp/block" << (block_query->start_address >> bitsperblock) << ".png", src);

    if (bool bDrawBorder = true)
    {
      VisusAssert(src.dtype == DTypes::UINT8_RGB);
      VisusAssert(pdim == 2);

      int W = (int)src.dims[0];
      int H = (int)src.dims[1];

      Uint8* samples = src.c_ptr();
      auto setBlack = [&](int R, int C) {memset(samples + (R*W + C) * 3, 0, 3); };

      for (int R = 0; R < src.dims[0]; R++) { setBlack(R, 0); setBlack(R, H - 1); }
      for (int C = 0; C < src.dims[1]; C++) { setBlack(0, C); setBlack(W - 1, C); }
    }

    ArrayUtils::paste(ret, p1, src);
  }

  access->endRead();

  return ret;
}

//////////////////////////////////////////////////////////////////////////
void Dataset::writeToObjectStream(ObjectStream& ostream)
{
  ostream.write("url",this->getUrl().toString());

  //I want to save it to retrieve it on a different computer
  if (!config.empty())
  {
    ostream.pushContext("config");
    ostream.getCurrentContext()->addChild(this->config);
    ostream.popContext("config");
  }
}

//////////////////////////////////////////////////////////////////////////
void Dataset::readFromObjectStream(ObjectStream& istream)
{
  String url = istream.read("url");

  if (istream.pushContext("config"))
  {
    VisusAssert(istream.getCurrentContext()->getNumberOfChilds() == 1);
    this->config = istream.getCurrentContext()->getChild(0);
    istream.popContext("config");
  }

  if (!this->openFromUrl(url))
    ThrowException(StringUtils::format() << "Cannot open dataset from url " << url);
}

} //namespace Visus 
