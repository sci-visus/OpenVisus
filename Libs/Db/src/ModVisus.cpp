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

#include <Visus/ModVisus.h>
#include <Visus/Dataset.h>
#include <Visus/File.h>
#include <Visus/TransferFunction.h>
#include <Visus/NetService.h>
#include <Visus/StringTree.h>
#include <Visus/IdxDataset.h>
#include <Visus/IdxMultipleDataset.h>
#include <Visus/IdxFilter.h>
#include <Visus/IdxMultipleDataset.h>

namespace Visus {

////////////////////////////////////////////////////////////////////////////////
static NetResponse CreateNetResponseError(int status, String errormsg, String file, int line)
{
  return NetResponse(status, errormsg + " __FILE__(" + file + ") __LINE__(" + cstring(line) + ")");
}

#define NetResponseError(status,errormsg) CreateNetResponseError(status,errormsg,__FILE__,__LINE__)

////////////////////////////////////////////////////////////////////////////////
class ModVisus::Datasets
{
public:

  //constructor
  Datasets(const StringTree& config) 
  {
    StringTree datasets("datasets");
    addPublicDatasets(datasets, config);
    datasets_xml_body  = datasets.toXmlString();
    datasets_json_body = datasets.toJSONString();
  }

  //destructor
  ~Datasets() {
  }

  //getNumberOfDatasets
  int getNumberOfDatasets() const {
    return (int)datasets_map.size();
  }

  //createPublicUrl
  String createPublicUrl(String name) const {
    return "$(protocol)://$(hostname):$(port)/mod_visus?action=readdataset&dataset=" + name;
  }

  //getDatasetsBody
  String getDatasetsBody(String format = "xml") const
  {
    if (format == "json")
      return datasets_json_body;
    else
      return datasets_xml_body;
  }

  //findDataset
  SharedPtr<Dataset> findDataset(String name) const
  {
    auto it = datasets_map.find(name);
    return (it != datasets_map.end()) ? it->second : SharedPtr<Dataset>();
  }

private:

  VISUS_NON_COPYABLE_CLASS(Datasets)

  typedef std::map<String, SharedPtr<Dataset > > DatasetMap;

  DatasetMap        datasets_map;
  String            datasets_xml_body;
  String            datasets_json_body;

  //addPublicDataset
  int addPublicDataset(StringTree& dst, String name, SharedPtr<Dataset> dataset) 
  {
    int ret = 1;
    datasets_map[name] = dataset;
    dataset->setServerMode(true);
    
    StringTree public_dataset("dataset");
    public_dataset.write("name", name);
    public_dataset.write("url", createPublicUrl(name));
    dst.addChild(public_dataset);

    //automatically add the childs of a multiple datasets
    if (auto midx=std::dynamic_pointer_cast<IdxMultipleDataset>(dataset))
    {
      for (auto it : midx->down_datasets)
        ret += addPublicDataset(public_dataset, name + "/" + it.first, it.second);
    }

    return ret;
  }

  //addPublicDatasets
  int addPublicDatasets(StringTree& dst, const StringTree& cursor)
  {
    int ret = 0;

    //I want to maintain the group hierarchy!
    if (cursor.name == "group")
    {
      StringTree group(cursor.name);
      group.attributes = cursor.attributes;
      for (auto child : cursor.getChilds())
        ret += addPublicDatasets(group, *child);
      if (ret)
        dst.addChild(group);
      return ret;
    }

    //flattening the hierarchy!
    if (cursor.name != "dataset") 
    {
      for (auto child : cursor.getChilds())
        ret += addPublicDatasets(dst, *child);
      return ret;
    }

    //just ignore those with empty names or not public
    String name = cursor.readString("name");
    String url  = cursor.readString("url");
    bool is_public = StringUtils::contains(cursor.readString("permissions"), "public");
    if (name.empty() || !is_public || !Url(url).valid())
      return 0;

    SharedPtr<Dataset> dataset;
    try
    {
      dataset = LoadDatasetEx(cursor);
    }
    catch(...) {
      PrintWarning("dataset name", name, "load failed, skipping it");
      return 0;
    }

    if (datasets_map.find(name) != datasets_map.end()) {
      PrintWarning("dataset name", name, "already exists, skipping it");
      return 0;
    }

    return addPublicDataset(dst, name, dataset);
  }

};

////////////////////////////////////////////////////////////////////////////////
ModVisus::ModVisus()
{
}

////////////////////////////////////////////////////////////////////////////////
ModVisus::~ModVisus()
{ 
  if (dynamic)
  {
    bExit = true;
    config_thread->join();
    config_thread.reset();
  }
}

////////////////////////////////////////////////////////////////////////////////
SharedPtr<ModVisus::Datasets> ModVisus::getDatasets()
{
  if (dynamic) rw_lock.enterRead();
  auto ret = m_datasets;
  if (dynamic) rw_lock.exitRead();
  return ret;
}

////////////////////////////////////////////////////////////////////////////////
bool ModVisus::configureDatasets(const ConfigFile& config)
{
  this->dynamic = config.readBool("Configuration/ModVisus/dynamic", false);
  this->config_filename = config.getFilename();

  //for dynamic I need to reload the file
  if (config_filename.empty() && this->dynamic)
  {
    PrintInfo("Switching ModVisus to non-dynamic content since the config file is not stored on disk");
    this->dynamic = false;
  }

  SharedPtr<Datasets> datasets = std::make_shared<Datasets>(config);
  this->m_datasets = datasets;
  
  if (dynamic)
  {
    this->config_timestamp = FileUtils::getTimeLastModified(this->config_filename);

    this->config_thread = Thread::start("Check config thread", [this]() 
    {
      while (!bExit)
      {
        auto timestamp = FileUtils::getTimeLastModified(this->config_filename);
        bool bReload = false;
        {
          ScopedReadLock lock(rw_lock);
          bReload = this->config_timestamp != timestamp;
        }

        if (bReload)
          reload();

        Thread::sleep(1000);
      }
    });
  }

  PrintInfo("ModVisus::configure dynamic",dynamic,"config_filename",config_filename,"...");
  PrintInfo("/mod_visus?action=list\n",datasets->getDatasetsBody());

  return true;
}

///////////////////////////////////////////////////////////////////////////
bool ModVisus::reload()
{
  if (!dynamic)
    return false;

  ConfigFile config;
  if (!config.load(this->config_filename))
  {
    PrintInfo("Reload modvisus config_filename", this->config_filename,"failed");
    return false;
  }

  auto datasets = std::make_shared<Datasets>(config);
  {
    ScopedWriteLock lock(this->rw_lock);
    this->m_datasets = datasets;
    this->config_timestamp = FileUtils::getTimeLastModified(this->config_filename);
  }

  PrintInfo("modvisus config file changed config_filename",this->config_filename,"#datasets",datasets->getNumberOfDatasets());
  return true;
}


///////////////////////////////////////////////////////////////////////////
NetResponse ModVisus::handleAddDataset(const NetRequest& request)
{
  //not supported
  if (!this->dynamic)
    return NetResponseError(HttpStatus::STATUS_BAD_REQUEST, "Mod visus is in non-dynamic mode");

  auto datasets = getDatasets();

  StringTree stree;
  if (request.url.hasParam("name"))
  {
    auto name = request.url.getParam("name");
    auto url = request.url.getParam("url");

    stree = StringTree("dataset");
    stree.write("name", name);
    stree.write("url", url);
    stree.write("permissions", "public");
  }
  else if (request.url.hasParam("xml"))
  {
    String content = request.url.getParam("xml");
    stree = StringTree::fromString(content);
    if (!stree.valid())
      return NetResponseError(HttpStatus::STATUS_BAD_REQUEST, "Cannot decode xml");
  }

  //add the dataset
  {
    //need to use a file_lock to make sure I don't loose any addPublicDataset 
    ScopedFileLock file_lock(this->config_filename);

    String name = stree.readString("name");

    if (name.empty())
      return NetResponseError(HttpStatus::STATUS_BAD_REQUEST, "Empty name");

    if (m_datasets->findDataset(name))
      return NetResponseError(HttpStatus::STATUS_CONFLICT, "Cannot add dataset(" + name + ") because it already exists");

    ConfigFile config;
    if (!config.load(this->config_filename,/*bEnablePostProcessing*/false))
    {
      PrintWarning("Cannot load",this->config_filename);
      VisusAssert(false);//TODO rollback
      return NetResponseError(HttpStatus::STATUS_BAD_REQUEST, "Add dataset failed");
    }

    config.addChild(stree);

    try
    {
      config.save();
    }
    catch (...)
    {
      PrintWarning("Cannot save", config.getFilename());
      return NetResponseError(HttpStatus::STATUS_BAD_REQUEST, "Add dataset failed");
    }

    if (!reload()) {
      PrintWarning("Cannot reload modvisus config");
      return NetResponseError(HttpStatus::STATUS_BAD_REQUEST, "Reload failed");
    }
  }

  return NetResponse(HttpStatus::STATUS_OK);
}

///////////////////////////////////////////////////////////////////////////
NetResponse ModVisus::handleReload(const NetRequest& request)
{
  if (!reload())
    return NetResponseError(HttpStatus::STATUS_INTERNAL_SERVER_ERROR, "Cannot reload");
  else
    return NetResponse(HttpStatus::STATUS_OK);
}


///////////////////////////////////////////////////////////////////////////
NetResponse ModVisus::handleReadDataset(const NetRequest& request)
{
  String dataset_name = request.url.getParam("dataset");

  auto datasets=getDatasets();
  auto dataset = datasets->findDataset(dataset_name);
  if (!dataset)
    return NetResponseError(HttpStatus::STATUS_NOT_FOUND, "Cannot find dataset(" + dataset_name + ")");

  NetResponse response(HttpStatus::STATUS_OK);
  response.setHeader("visus-git-revision", OpenVisus_GIT_REVISION);
  response.setHeader("visus-typename", dataset->getDatasetTypeName());

  auto body = dataset->getDatasetBody();

  //backward compatible
  bool bPreferOldIdxFormat = true;

  if (dataset->getDatasetTypeName()=="IdxDataset" && bPreferOldIdxFormat)
  {
    auto idxfile = std::dynamic_pointer_cast<IdxDataset>(dataset)->idxfile;
    String content=idxfile.writeToOldFormat();
    response.setTextBody(content,/*bHasBinary*/true);
  }
  else 
  {
    //remap urls...
    std::stack< std::pair<String, StringTree*> > stack;
    stack.push(std::make_pair("", &body));
    while (!stack.empty())
    {
      auto prefix = stack.top().first;
      auto cur = stack.top().second;
      stack.pop();
      if (cur->name == "dataset" && !cur->readString("name").empty())
      {
        prefix += prefix.empty() ? "" : "/";
        prefix += cur->readString("name");
        cur->write("url", datasets->createPublicUrl(prefix));
      }
      for (auto child : cur->getChilds())
        stack.push(std::make_pair(prefix, child.get()));
    }

    response.setTextBody(body.toString(),/*bHasBinary*/true);
  }

  return response;
}

///////////////////////////////////////////////////////////////////////////
NetResponse ModVisus::handleGetListOfDatasets(const NetRequest& request)
{
  String format = request.url.getParam("format", "xml");
  String hostname = request.url.getParam("hostname"); //trick if you want $(localhost):$(port) to be replaced with what the client has
  String port = request.url.getParam("port");

  NetResponse response(HttpStatus::STATUS_OK);

  auto datasets=getDatasets();

  if (format == "xml")
    response.setXmlBody(datasets->getDatasetsBody(format));
  else if (format == "json")
    response.setJSONBody(datasets->getDatasetsBody(format));
  else
    return NetResponseError(HttpStatus::STATUS_NOT_FOUND, "wrong format(" + format + ")");

  if (!hostname.empty())
    response.setTextBody(StringUtils::replaceAll(response.getTextBody(), "$(hostname)", hostname));

  if (!port.empty())
    response.setTextBody(StringUtils::replaceAll(response.getTextBody(), "$(port)", port));

  return response;
}

///////////////////////////////////////////////////////////////////////////
//deprecated
#if 0
NetResponse ModVisus::handleHtmlForPlugin(const NetRequest& request)
{
  String htmlcontent =
    "<HTML>\r\n"
    "<HEAD><TITLE>Visus Plugin</TITLE><STYLE>body{margin:0;padding:0;}</STYLE></HEAD><BODY>\r\n"
    "  <center>\r\n"
    "  <script>\r\n"
    "    document.write('<embed  id=\"plugin\" type=\"application/npvisusplugin\" src=\"\" width=\"100%%\" height=\"100%%\"></embed>');\r\n"
    "    document.getElementById(\"plugin\").open(location.href);\r\n"
    "  </script>\r\n"
    "  <noscript>NPAPI not enabled</noscript>\r\n"
    "  </center>\r\n"
    "</BODY>\r\n"
    "</HTML>\r\n";

  NetResponse response(HttpStatus::STATUS_OK);
  response.setHtmlBody(htmlcontent);
  return response;
}
#endif


///////////////////////////////////////////////////////////////////////////
NetResponse ModVisus::handleBlockQuery(const NetRequest& request)
{
  auto datasets=getDatasets();

  String dataset_name = request.url.getParam("dataset");

  auto dataset = datasets->findDataset(dataset_name);
  if (!dataset)
    return NetResponseError(HttpStatus::STATUS_NOT_FOUND, "Cannot find dataset(" + dataset_name + ")");

  String compression = request.url.getParam("compression");
  String fieldname = request.url.getParam("field", dataset->getField().name);
  double time = cdouble(request.url.getParam("time", cstring(dataset->getTime())));

  auto bitsperblock = dataset->getDefaultBitsPerBlock();

  std::vector<BigInt> blocks;

  if (request.url.hasParam("block"))
  {
    for (auto it : StringUtils::split(request.url.getParam("block", "0")))
      blocks.push_back(cbigint(it));
  }
  else if (request.url.hasParam("from"))
  {
    for (auto it : StringUtils::split(request.url.getParam("from", "0")))
      blocks.push_back(cbigint(it)>>bitsperblock);
  }

  if (blocks.empty())
    return NetResponseError(HttpStatus::STATUS_NOT_FOUND, "blocks empty()");

  Field field = fieldname.empty() ? dataset->getField() : dataset->getField(fieldname);
  if (!field.valid())
    return NetResponseError(HttpStatus::STATUS_NOT_FOUND, "Cannot find field(" + fieldname + ")");

  bool bHasFilter = !field.filter.empty();

  auto access = dataset->createAccessForBlockQuery();

  WaitAsync< Future<Void> > wait_async;
  access->beginRead();
  Aborted aborted;

  std::vector<NetResponse> responses;
  for (auto blockid : blocks)
  {
    auto block_query = dataset->createBlockQuery(blockid, field, time, 'r', aborted);
    dataset->executeBlockQuery(access, block_query);
    wait_async.pushRunning(block_query->done).when_ready([block_query, &responses, dataset, compression](Void) {

      if (block_query->failed())
      {
        responses.push_back(NetResponseError(HttpStatus::STATUS_NOT_FOUND, "block_query->executeAndWait failed"));
        return;
      }

      //encode data
      NetResponse response(HttpStatus::STATUS_OK);
      if (!response.setArrayBody(compression, block_query->buffer))
      {
        //maybe i need to convert to row major to compress
        if (!(dataset->convertBlockQueryToRowMajor(block_query) && response.setArrayBody(compression, block_query->buffer)))
        {
          responses.push_back(NetResponseError(HttpStatus::STATUS_INTERNAL_SERVER_ERROR, "Encoding converting to row major failed"));
          return;
        }
      }

      responses.push_back(response);
    });
  }
  access->endRead();

  wait_async.waitAllDone();

  return NetResponse::compose(responses);
}

///////////////////////////////////////////////////////////////////////////
NetResponse ModVisus::handleBoxQuery(const NetRequest& request)
{
  auto dataset_name = request.url.getParam("dataset");
  auto fromh = cint(request.url.getParam("fromh"));
  auto endh = cint(request.url.getParam("toh"));
  auto maxh = cint(request.url.getParam("maxh"));
  auto time = cdouble(request.url.getParam("time"));
  auto compression = request.url.getParam("compression");

  auto datasets = getDatasets();

  auto dataset = datasets->findDataset(dataset_name);
  if (!dataset)
    return NetResponseError(HttpStatus::STATUS_NOT_FOUND, "Cannot find dataset(" + dataset_name + ")");

  int pdim = dataset->getPointDim();

  String fieldname = request.url.getParam("field");
  Field field = fieldname.empty() ? dataset->getField() : dataset->getField(fieldname);
  if (!field.valid())
    return NetResponseError(HttpStatus::STATUS_BAD_REQUEST, "Cannot find fieldname(" + fieldname + ")");

  //TODO: how can I get the aborted from network?

  Array buffer;

  bool   bDisableFilters = cbool(request.url.getParam("disable_filters"));
  bool   bKdBoxQuery = request.url.getParam("kdquery") == "box";

  auto logic_box = BoxNi::parseFromOldFormatString(pdim, request.url.getParam("box"));;
  auto query = dataset->createBoxQuery(logic_box, field, time, 'r', Aborted());
  query->setResolutionRange(fromh, endh);

  //I apply the filter on server side only for the first coarse query (more data need to be processed on client side)
  query->disableFilters();
  if (auto idx = std::dynamic_pointer_cast<IdxDataset>(dataset))
  {
    if (fromh == 0 && !bDisableFilters)
    {
      query->enableFilters();
      query->filter.domain = (bKdBoxQuery ? idx->idxfile.bitmask.getPow2Box() : dataset->getLogicBox());
    }
  }

  dataset->beginBoxQuery(query);

  if (!query->isRunning())
    return NetResponseError(HttpStatus::STATUS_BAD_REQUEST, "dataset->beginBoxQuery() failed " + query->errormsg);

  auto access = dataset->createAccess();
  if (!dataset->executeBoxQuery(access, query))
    return NetResponseError(HttpStatus::STATUS_BAD_REQUEST, "dataset->executeBoxQuery() failed " + query->errormsg);

  buffer = query->buffer;

  //useful for kdquery=box (for example with discrete wavelets, don't want the extra channel)
  if (bKdBoxQuery)
  {
    if (auto filter = query->filter.dataset_filter)
      buffer = filter->dropExtraComponentIfExists(buffer);
  }


  String palette = request.url.getParam("palette");
  if (!palette.empty() && buffer.dtype.ncomponents() == 1)
  {
    auto tf=TransferFunction::getDefault(palette);
    if (!tf)
    {
      VisusAssert(false);
      PrintInfo("invalid palette specified",palette);
      PrintInfo("use one of:");
      std::vector<String> tf_defaults = TransferFunction::getDefaults();
      for (int i = 0; i < tf_defaults.size(); i++)
        PrintInfo("\t",tf_defaults[i]);
    }
    else
    {
      double palette_min = cdouble(request.url.getParam("palette_min"));
      double palette_max = cdouble(request.url.getParam("palette_max"));

      if (palette_min != palette_max)
      {
        tf->beginTransaction();
        tf->setInputRange(Range(palette_min, palette_max, 0));
        tf->setInputNormalizationMode(ArrayUtils::UseFixedRange);
        tf->endTransaction();
      }

      buffer = ArrayUtils::applyTransferFunction(tf, buffer);
      if (!buffer)
        return NetResponseError(HttpStatus::STATUS_INTERNAL_SERVER_ERROR, "palette failed");
    }
  }

  NetResponse response(HttpStatus::STATUS_OK);
  if (!response.setArrayBody(compression, buffer))
    return NetResponseError(HttpStatus::STATUS_INTERNAL_SERVER_ERROR, "NetResponse encodeBuffer failed");

  return response;

}


///////////////////////////////////////////////////////////////////////////
NetResponse ModVisus::handlePointQuery(const NetRequest& request)
{
  auto dataset_name = request.url.getParam("dataset");
  auto fromh = cint(request.url.getParam("fromh"));
  auto endh = cint(request.url.getParam("toh"));
  auto maxh = cint(request.url.getParam("maxh"));
  auto time = cdouble(request.url.getParam("time"));
  auto compression = request.url.getParam("compression");

  auto datasets = getDatasets();

  auto dataset = datasets->findDataset(dataset_name);
  if (!dataset)
    return NetResponseError(HttpStatus::STATUS_NOT_FOUND, "Cannot find dataset(" + dataset_name + ")");

  int pdim = dataset->getPointDim();

  String fieldname = request.url.getParam("field");
  Field field = fieldname.empty() ? dataset->getField() : dataset->getField(fieldname);
  if (!field.valid())
    return NetResponseError(HttpStatus::STATUS_BAD_REQUEST, "Cannot find fieldname(" + fieldname + ")");

  //TODO: how can I get the aborted from network?

  Array buffer;

  auto nsamples = PointNi::fromString(request.url.getParam("nsamples"));
  VisusAssert(nsamples.getPointDim() == 3);

  VisusAssert(fromh == 0);

  auto logic_position = Position(
    Matrix::fromString(4, request.url.getParam("matrix")),
    BoxNd::fromString(request.url.getParam("box"),/*bInterleave*/false).withPointDim(3));

  auto query = dataset->createPointQuery(logic_position, field, time);
  query->end_resolutions = { endh };

  dataset->beginPointQuery(query);

  if (!query->isRunning())
    return NetResponseError(HttpStatus::STATUS_BAD_REQUEST, "dataset->beginBoxQuery() failed " + query->errormsg);

  if (!query->setPoints(nsamples))
    return NetResponseError(HttpStatus::STATUS_BAD_REQUEST, "dataset->setPoints failed " + query->errormsg);

  auto access = dataset->createAccess();
  if (!dataset->executePointQuery(access, query))
    return NetResponseError(HttpStatus::STATUS_BAD_REQUEST, "dataset->executeBoxQuery() failed " + query->errormsg);

  buffer = query->buffer;

  String palette = request.url.getParam("palette");
  if (!palette.empty() && buffer.dtype.ncomponents() == 1)
  {
    auto tf=TransferFunction::getDefault(palette);
    if (!tf)
    {
      VisusAssert(false);
      PrintInfo("invalid palette specified",palette);
      PrintInfo("use one of:");
      std::vector<String> tf_defaults = TransferFunction::getDefaults();
      for (int i = 0; i < tf_defaults.size(); i++)
        PrintInfo("\t",tf_defaults[i]);
    }
    else
    {
      double palette_min = cdouble(request.url.getParam("palette_min"));
      double palette_max = cdouble(request.url.getParam("palette_max"));

      if (palette_min != palette_max)
      {
        tf->setInputNormalizationMode(ArrayUtils::UseFixedRange);
        tf->setInputRange(Range(palette_min, palette_max, 0));
      }

      buffer = ArrayUtils::applyTransferFunction(tf, buffer);
      if (!buffer)
        return NetResponseError(HttpStatus::STATUS_INTERNAL_SERVER_ERROR, "palette failed");
    }
  }

  NetResponse response(HttpStatus::STATUS_OK);
  if (!response.setArrayBody(compression, buffer))
    return NetResponseError(HttpStatus::STATUS_INTERNAL_SERVER_ERROR, "NetResponse encodeBuffer failed");

  return response;
}

///////////////////////////////////////////////////////////////////////////
NetResponse ModVisus::handleRequest(NetRequest request)
{
  Time t1 = Time::now();

  //default action
  if (request.url.getParam("action").empty())
  {
    String user_agent = StringUtils::toLower(request.getHeader("User-Agent"));

    bool bSpecifyDataset = request.url.hasParam("dataset");
    //bool bCommercialBrower = !user_agent.empty() && !StringUtils::contains(user_agent, "visus");

    if (bSpecifyDataset)
    {
      request.url.setParam("action", "readdataset");
    }
    else
    {
      request.url.setParam("action", "list");
      request.url.setParam("format", "xml"); //"html"
    }
  }

  String action = request.url.getParam("action");

  NetResponse response;

  if (action == "rangequery" || action == "blockquery")
    response = handleBlockQuery(request);

  else if (action == "query" || action == "boxquery")
    response = handleBoxQuery(request);

  else if (action == "pointquery")
    response = handlePointQuery(request);

  else if (action == "readdataset" || action == "read_dataset")
    response = handleReadDataset(request);

  else if (action == "list")
    response = handleGetListOfDatasets(request);

  else if (action == "configure_datasets" || action == "configure" || action == "reload")
    response = handleReload(request);

  else if (action == "AddDataset" || action == "add_dataset")
    response = handleAddDataset(request);

  else if (action == "ping")
  {
    response = NetResponse(HttpStatus::STATUS_OK);
    response.setHeader("block-query-support-aggregation", "1");
  }

  else
    response = NetResponseError(HttpStatus::STATUS_NOT_FOUND, "unknown action(" + action + ")");

  PrintInfo(
    "request", request.url,
    "status", response.getStatusDescription(), "body", StringUtils::getStringFromByteSize(response.body ? response.body->c_size() : 0), "msec", t1.elapsedMsec());

  //add some standard header
  response.setHeader("git_revision", OpenVisus_GIT_REVISION);
  response.setHeader("version", OpenVisus_VERSION);

  //expose visus headers (for javascript access)
  //see https://stackoverflow.com/questions/35240520/fetch-answer-empty-due-to-the-preflight
  {
    std::vector<String> exposed_headers;
    exposed_headers.reserve(response.headers.size());
    for (auto header : response.headers) {
      if (StringUtils::startsWith(header.first, "visus"))
        exposed_headers.push_back(header.first);
    }
    response.setHeader("Access-Control-Expose-Headers", StringUtils::join(exposed_headers, ","));
  }

  return response;
}

} //namespace Visus
