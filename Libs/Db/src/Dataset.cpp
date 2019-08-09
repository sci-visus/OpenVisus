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
#include <Visus/Polygon.h>


namespace Visus {

VISUS_IMPLEMENT_SINGLETON_CLASS(DatasetFactory)


/////////////////////////////////////////////////////////////////////////////
std::vector<int> Dataset::guessEndResolutions(const Frustum& logic_to_screen,Position logic_position,QueryQuality quality,QueryProgression progression)
{
  int dataset_dim = this->getPointDim();

  if (progression == QueryGuessProgression)
    progression = (QueryProgression)(dataset_dim == 2 ? dataset_dim * 3 : dataset_dim * 4);

  auto maxh = this->getMaxResolution();
  int endh = maxh;
  DatasetBitmask bitmask = this->getBitmask();

  if (logic_to_screen.valid())
  {
    std::vector<Point3d> logic_points;
    for (auto p : logic_position.getPoints())
      logic_points.push_back(p.toPoint3());

    std::vector<Point2d> screen_points;
    FrustumMap map(logic_to_screen);
    for (auto logic_point : logic_points)
      screen_points.push_back(map.projectPoint(logic_point));

    if (dataset_dim == 2)
    {
      //scrgiorgio euristic for 2d
      VisusReleaseAssert(logic_points.size() == 4);
      VisusReleaseAssert(screen_points.size() == 4);

      std::vector<Point2d> screen_points;
      FrustumMap map(logic_to_screen);
      for (auto logic_point : logic_points)
        screen_points.push_back(map.projectPoint(Point3d(logic_point)));

      auto AREA = logic_position.computeVolume();
      auto area = Quad(screen_points).area();

      //maxh -- AREA == endh -- area
      endh = std::max(0,maxh - (int)round(log2(AREA / area)));
    }
    else if (dataset_dim == 3)
    {
      // valerio's algorithm, find the final view dependent resolution (endh)
      // (the default endh is the maximum resolution available)
      //NOTE: this works for slices and volumes
      std::pair<Point3d,Point3d> longest_edge;
      double longest_screen_distance = NumericLimits<double>::lowest();
      for (auto edge : BoxNi::getEdges(dataset_dim))
      {
        double screen_distance = (screen_points[edge.index1] - screen_points[edge.index0]).module();

        if (screen_distance > longest_screen_distance)
        {
          longest_edge = std::make_pair(logic_points[edge.index0], logic_points[edge.index1]);
          longest_screen_distance = screen_distance;
        }
      }

      //I match the highest resolution on dataset axis (it's just an euristic!)
      for (int A = 0; A < dataset_dim; A++)
      {
        double logic_distance = fabs(longest_edge.first[A] - longest_edge.second[A]);
        double factor = logic_distance / longest_screen_distance;
        Int64  num = Utils::getPowerOf2((Int64)factor);
        while (num > factor)
          num >>= 1;

        int H = maxh;
        for (; num > 1 && H >= 0; H--)
        {
          if (bitmask[H] == A)
            num >>= 1;
        }

        endh = std::min(endh, H);
      }

    }
    else
    {
      ThrowException("internal error");
    }
  }

  //quality can increase or decrease endh
  endh = std::min(maxh, quality + endh);

  std::vector<int> ret;
  ret.push_back(std::max(0, endh - progression));

  while (ret.back() != endh)
    ret.push_back(std::min(endh, ret.back() + dataset_dim));

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
  ret->setAvailableMemory(available);
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

///////////////////////////////////////////////////////////
Field Dataset::getFieldByName(String name) const {
  try {
    return getFieldByNameThrowEx(name);
  }
  catch (std::exception ex) {
    return Field();
  }
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
  config = it ? *it : StringTree();

  Url url(config.readString("url", name));
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

  ret->setUrl(url);
  ret->setConfig(config);
  ret->setKdQueryMode(KdQueryMode::fromString(config.readString("kdquery", url.getParam("kdquery"))));

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
  int samplesperblock = 1 << bitsperblock;

  BigInt total_number_of_blocks  = getTotalNumberOfBlocks();
  BigInt total_number_of_samples = total_number_of_blocks * samplesperblock;

  out<<"Visus file infos                                         "<<std::endl;
  out<<"  Logic box                                              "<< getLogicBox().toOldFormatString()<<std::endl;
  out <<" Physic position                                        "<< getPhysicPosition().toString() << std::endl;
  out<<"  Pow2 dims                                              "<<getBitmask().getPow2Dims().toString()<<std::endl;
  out<<"  number of samples                                      "<<total_number_of_samples<<std::endl;
  out<<"  number of blocks                                       "<<total_number_of_blocks<<std::endl;
  out<<"  timesteps                                              "<<std::endl<<getTimesteps().toString()<<std::endl;
  out<<"  bitmask                                                "<<bitmask.toString()<<std::endl;

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
SharedPtr<Dataset> LoadDataset(String url) {
  return LoadDatasetEx(url, *DbModule::getModuleConfig());
}


////////////////////////////////////////////////
Future<Void> Dataset::executeBlockQuery(SharedPtr<Access> access,SharedPtr<BlockQuery> query)
{
  VisusAssert(access->isReading());

  int mode = query->mode; 
  auto failed = [&]() {

    if (!access)
      query->setFailed();
    else
      mode == 'r'? access->readFailed(query) : access->writeFailed(query);
   
    return query->done;
  };

  if (!access)
    return failed();

  if (!query->field.valid())
    return failed();

  if (!(query->start_address < query->end_address))
    return failed();

  if ((mode == 'r' && !access->can_read) || (mode == 'w' && !access->can_write))
    return failed();

  if (!query->logic_samples.valid())
    return failed();

  if (mode == 'w' && !query->buffer)
    return failed();

  //auto allocate buffer
  if (!query->allocateBufferIfNeeded())
    return failed();

  // override time  from dataset url
  Url url = this->getUrl();
  if (url.hasParam("time"))
    query->time = cdouble(url.getParam("time"));

  // override time  from from field
  if (query->field.hasParam("time"))
    query->time = cdouble(query->field.getParam("time"));

  query->setRunning();
  mode=='r'? access->readBlock(query) : access->writeBlock(query);
  return query->done;
}


////////////////////////////////////////////////
std::vector<BoxNi> Dataset::generateTiles(int TileSize) const
{
  auto pdim = this->getPointDim();
  auto WindowSize = PointNi::one(pdim);
  for (int D = 0; D < pdim; D++)
    WindowSize[D] = TileSize;

  auto box = this->getLogicBox();

  auto Tot = PointNi::one(pdim);
  for (int D = 0; D < pdim; D++)
    Tot[D] = (Utils::alignRight(box.p2[D], box.p1[D], WindowSize[D]) - box.p1[D]) / WindowSize[D];

  std::vector<BoxNi> ret;
  for (auto P = ForEachPoint(box.p1, box.p2, WindowSize); !P.end(); P.next())
  {
    auto tile = BoxNi(P.pos, P.pos + WindowSize).getIntersection(this->getLogicBox());

    if (!tile.valid()) {
      VisusAssert(false);
      continue;
    }

    ret.push_back(tile);
  }
  return ret;
}

////////////////////////////////////////////////
Array Dataset::readFullResolutionData(SharedPtr<Access> access, Field field, double time, BoxNi box)
{
  if (box == BoxNi())
    box = this->logic_box;

  auto query = std::make_shared<BoxQuery>(this, field, time,  'r');
  query->logic_box = box;

  beginQuery(query);

  if (!executeQuery(access, query))
    return Array();

  return query->buffer;
}

////////////////////////////////////////////////
bool Dataset::writeFullResolutionData(SharedPtr<Access> access, Field field, double time, Array buffer, BoxNi box)
{
  if (box==BoxNi()) 
    box=BoxNi(PointNi(buffer.getPointDim()), buffer.dims);

  auto query = std::make_shared<BoxQuery>(this, field, time,'w');
  query->logic_box = box;

  beginQuery(query);

  if (!query->isRunning())
    return false;

  VisusAssert(query->getNumberOfSamples() == buffer.dims);
  query->buffer = buffer;

  if (!executeQuery(access, query))
    return false;

  return true;
}



//*********************************************************************
// valerio's algorithm, find the final view dependent resolution (endh)
// (the default endh is the maximum resolution available)
//*********************************************************************

PointNi Dataset::guessPointQueryNumberOfSamples(const Frustum& logic_to_screen, Position logic_position, int end_resolution)
{
  auto bitmask = getBitmask();
  int pdim = bitmask.getPointDim();

  if (!logic_position.valid())
    return PointNi(pdim);

  const int unit_box_edges[12][2] =
  {
    {0,1}, {1,2}, {2,3}, {3,0},
    {4,5}, {5,6}, {6,7}, {7,4},
    {0,4}, {1,5}, {2,6}, {3,7}
  };

  std::vector<Point3d> logic_points;
  for (auto p : logic_position.getPoints())
    logic_points.push_back(p.toPoint3());

  std::vector<Point2d> screen_points;
  if (logic_to_screen.valid())
  {
    FrustumMap map(logic_to_screen);
    for (int I = 0; I < 8; I++)
      screen_points.push_back(map.projectPoint(logic_points[I]));
  }

  PointNi virtual_worlddim = PointNi::one(pdim);
  for (int H = 1; H <= end_resolution; H++)
  {
    int bit = bitmask[H];
    virtual_worlddim[bit] <<= 1;
  }

  PointNi nsamples = PointNi::one(pdim);
  for (int E = 0; E < 12; E++)
  {
    int query_axis = (E >= 8) ? 2 : (E & 1 ? 1 : 0);
    Point3d P1 = logic_points[unit_box_edges[E][0]];
    Point3d P2 = logic_points[unit_box_edges[E][1]];
    Point3d edge_size = (P2 - P1).abs();

    PointNi idx_size = this->getLogicBox().size();

    // need to project onto IJK  axis
    // I'm using this formula: x/virtual_worlddim[dataset_axis] = factor = edge_size[dataset_axis]/idx_size[dataset_axis]
    for (int dataset_axis = 0; dataset_axis < 3; dataset_axis++)
    {
      double factor = (double)edge_size[dataset_axis] / (double)idx_size[dataset_axis];
      Int64 x = (Int64)(virtual_worlddim[dataset_axis] * factor);
      nsamples[query_axis] = std::max(nsamples[query_axis], x);
    }
  }

  //view dependent, limit the nsamples to what the user can see on the screen!
  if (!screen_points.empty())
  {
    PointNi view_dependent_dims = PointNi::one(pdim);
    for (int E = 0; E < 12; E++)
    {
      int query_axis = (E >= 8) ? 2 : (E & 1 ? 1 : 0);
      Point2d p1 = screen_points[unit_box_edges[E][0]];
      Point2d p2 = screen_points[unit_box_edges[E][1]];
      double pixel_distance_on_screen = (p2 - p1).module();
      view_dependent_dims[query_axis] = std::max(view_dependent_dims[query_axis], (Int64)pixel_distance_on_screen);
    }

    nsamples[0] = std::min(view_dependent_dims[0], nsamples[0]);
    nsamples[1] = std::min(view_dependent_dims[1], nsamples[1]);
    nsamples[2] = std::min(view_dependent_dims[2], nsamples[2]);
  }

  return nsamples;
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
    Wvf->getTotalNumberOfBlocks(),
    Rvf->getTotalNumberOfBlocks());

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
    auto read_block = std::make_shared<BlockQuery>(Rvf, Rfield, Rtime, Raccess->getStartAddress(block_id), Raccess->getEndAddress(block_id), 'r', aborted);

    if (!Rvf->executeBlockQueryAndWait(Raccess, read_block))
      continue; 

    auto write_block = std::make_shared<BlockQuery>(Wvf, Wfield, Wtime, Waccess->getStartAddress(block_id), Waccess->getEndAddress(block_id), 'w', aborted);
    write_block->buffer = read_block->buffer;

    if (!Wvf->executeBlockQueryAndWait(Waccess, write_block))
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
  auto bitmask = this->getBitmask();
  auto bitsperblock = access->bitsperblock;
  auto nsamplesperblock = 1 << bitsperblock;

  VisusAssert(H >= bitsperblock);

  LogicSamples Lsamples;

  if (H == bitsperblock)
    Lsamples = this->getAddressRangeSamples(0, nsamplesperblock);
  else
    Lsamples = this->getLevelSamples(H);

  auto start_block = (H == bitsperblock) ? 0 : (1 << (H - bitsperblock - 1));
  auto block_per_level = std::max(1, start_block);

  Array ret(Lsamples.nsamples, DTypes::UINT8_RGB);
  ret.fillWithValue(0);

  access->beginRead();

  for (int block = start_block; block < (start_block + block_per_level); block++)
  {
    auto hzfrom = block << bitsperblock;
    auto hzto = hzfrom + nsamplesperblock;

    auto block_query = std::make_shared<BlockQuery>(this, field, time, hzfrom, hzto, 'r', Aborted());
    

    this->executeBlockQueryAndWait(access, block_query);

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

    auto p1 = Lsamples.logicToPixel(block_query->getLogicBox().p1);
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
