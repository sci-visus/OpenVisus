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

namespace Visus {

VISUS_IMPLEMENT_SINGLETON_CLASS(DatasetFactory)


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
SharedPtr<BlockQuery> Dataset::createBlockQuery(BigInt blockid, Field field, double time, int mode, Aborted aborted)
{
  auto ret = std::make_shared<BlockQuery>();
  ret->dataset = this;
  ret->field = field;
  ret->time = time;
  ret->mode = mode; VisusAssert(mode == 'r' || mode == 'w');
  ret->aborted = aborted;
  ret->done = Promise<Void>().get_future();
  ret->blockid = blockid;
  ret->logic_samples = getBlockSamples(blockid);
  return ret;
}

////////////////////////////////////////////////////////////////////
SharedPtr<Access> Dataset::createAccess(StringTree config,bool bForBlockQuery)
{
  if (!config.valid())
    config = getDefaultAccessConfig();

  if (!config.valid()) {
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
    auto can_read  = StringUtils::contains(config.readString("chmod", Access::DefaultChMod), "r");
    auto can_write = StringUtils::contains(config.readString("chmod", Access::DefaultChMod), "w");
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


////////////////////////////////////////////////////////////////////
bool Dataset::executeBoxQueryOnServer(SharedPtr<BoxQuery> query)
{
  auto request = createBoxQueryRequest(query);

  if (!request.valid())
  {
    query->setFailed("cannot create box query request");
    return false;
  }

  auto response = NetService::getNetResponse(request);

  if (!response.isSuccessful())
  {
    query->setFailed(cstring("network request failed",cnamed("errormsg",response.getErrorMessage())));
    return false;
  }

  auto decoded = response.getCompatibleArrayBody(query->getNumberOfSamples(), query->field.dtype);
  if (!decoded) {
    query->setFailed("failed to decode body");
    return false;
  }

  query->buffer = decoded;
  query->setCurrentResolution(query->end_resolution);
  return true;
}

////////////////////////////////////////////////////////////////////
bool Dataset::executePointQueryOnServer(SharedPtr<PointQuery> query)
{
  auto request = createPointQueryRequest(query);

  if (!request.valid())
  {
    query->setFailed("cannot create point query request");
    return false;
  }

  auto response = NetService::getNetResponse(request);

  if (!response.isSuccessful())
  {
    query->setFailed(cstring("network request failed ",cnamed("errormsg", response.getErrorMessage())));
    return false;
  }

  auto decoded = response.getCompatibleArrayBody(query->getNumberOfSamples(), query->field.dtype);
  if (!decoded) {
    query->setFailed("failed to decode body");
    return false;
  }

  query->buffer = decoded;
  query->setOk();
  return true;
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

  if (mode == 'w' && !query->buffer)
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

  //important
#if 1
  nsamples = nsamples.compactDims(); 
  nsamples.setPointDim(3, 1);
#endif

  return nsamples;
}

/////////////////////////////////////////////////////////
SharedPtr<PointQuery> Dataset::createPointQuery(Position logic_position, Field field, double time, Aborted aborted)
{
  auto ret = std::make_shared<PointQuery>();
  ret->dataset = this;
  ret->field = field = field;
  ret->time = time;
  ret->mode = 'r';
  ret->aborted = aborted;
  ret->logic_position = logic_position;
  return ret;
}



} //namespace Visus 
