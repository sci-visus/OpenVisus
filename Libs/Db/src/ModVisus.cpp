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

namespace Visus {

////////////////////////////////////////////////////////////////////////////////
static NetResponse CreateNetResponseError(int status, String errormsg, String file, int line)
{
  return NetResponse(status, errormsg + " __FILE__(" + file + ") __LINE__(" + cstring(line) + ")");
}

#define NetResponseError(status,errormsg) CreateNetResponseError(status,errormsg,__FILE__,__LINE__)

////////////////////////////////////////////////////////////////////////////////
class ModVisus::PublicDatasets
{
public:

  VISUS_NON_COPYABLE_CLASS(PublicDatasets)

  ModVisus* owner;

  //constructor
  PublicDatasets(ModVisus* owner_) : owner(owner_),datasets("datasets"){
  }

  //constructor
  PublicDatasets(ModVisus* owner, const StringTree& config) : PublicDatasets(owner) {
    addPublicDatasets(config);
  }

  //destructor
  ~PublicDatasets() {
  }

  //addPublicDatasets
  void addPublicDatasets(const StringTree& config)
  {
    this->addPublicDatasets(this->datasets, config);
    this->datasets_xml_body = this->datasets.toXmlString();
    this->datasets_json_body = this->datasets.toJSONString();
  }

  //getNumberOfDatasets
  int getNumberOfDatasets() const {
    return (int)dataset_map.size();
  }

  //findDataset
  SharedPtr<Dataset> findDataset(String name) 
  {
    // first remove any temp datasets older than 5 minutes
    for (auto it = temp_dataset_map.cbegin(); it != temp_dataset_map.cend(); /* no increment */) {
      if (it->second.second.elapsedMsec() > 5*60*1000 &&
          it->first != name) {
        PrintInfo("releasing temp dataset", it->first);
        it = temp_dataset_map.erase(it);
      }
      else {
        ++it;
      }
    }
    
    // return dataset from visus.config, if it exists
    auto it = dataset_map.find(name);
    if (it != dataset_map.end()) {
      return it->second;
    }

    // return dataset from already loaded temp datasets, update timestamp if it's there
    auto itt = temp_dataset_map.find(name);
    if (itt != temp_dataset_map.end()) {
      PrintInfo("reusing temp dataset", itt->first);
      itt->second.second = Time::now();
      return itt->second.first;
    }
    
    // search the filesystem for the dataset
    Path homePath(KnownPaths::VisusHome);
    Path idxPath = homePath.getChild("converted/"+name+"/visus.idx");
    if (FileUtils::existsFile(idxPath)) {
      PrintInfo("creating temp dataset", name, idxPath.toString());
      
      StringTree stree("dataset");
      stree.write("name", name);
      stree.write("url", "file://" + idxPath.toString());
      stree.write("permissions", "public");

      try
      {
        auto d = LoadDatasetEx(stree);
        temp_dataset_map[name] = { d, Time::now() };
        return d;
      }
      catch(...) {
        PrintWarning("dataset name", name, "load failed");
      }
    }

    // couldn't find the dataset
    return SharedPtr<Dataset>();
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

private:

  StringTree                              datasets;
  std::map<String, SharedPtr<Dataset > >  dataset_map;
  std::map<String, std::pair<SharedPtr<Dataset>, Time>> temp_dataset_map;
  String                                  datasets_xml_body;
  String                                  datasets_json_body;

  //addPublicDataset
  int addPublicDataset(StringTree& dst, String name, SharedPtr<Dataset> dataset)
  {
    this->dataset_map[name] = dataset;
    dataset->setServerMode(true);

    auto child= dst.addChild("dataset");
    child->write("name", name);
    child->write("url", createPublicUrl(name));

    //automatically add the childs of a multiple datasets
    int ret = 1;
    if (auto midx = std::dynamic_pointer_cast<IdxMultipleDataset>(dataset))
    {
      for (auto it : midx->down_datasets)
      {
        auto child_name    = it.first;
        auto child_dataset = it.second;
        ret += addPublicDataset(*child, name + "/" + child_name, child_dataset);
      }
    }

    return ret;
  }

  //addPublicDatasets
  int addPublicDatasets(StringTree& dst, const StringTree& cursor)
  {
    //I want to maintain the group hierarchy!
    if (cursor.name == "group")
    {
      int ret = 0;
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
      int ret = 0;
      for (auto child : cursor.getChilds())
        ret += addPublicDatasets(dst, *child);
      return ret;
    }

    String url = cursor.readString("url");
    if (!Url(url).valid())
      return 0;

    bool is_public = owner->default_public || StringUtils::contains(cursor.readString("permissions"), "public");
    if (!is_public)
      return 0;

    String name = cursor.readString("name");
    if (name.empty())
      return 0;
    
    SharedPtr<Dataset> dataset;
    try
    {
      PrintInfo("Loading dataset",concatenate("url(",url,")"),concatenate("name(",name,")"),"...");
      dataset = LoadDatasetEx(cursor);
      PrintInfo("...","ok");
    }
    catch (...) {
      PrintWarning("... failed, skipping it");
      return 0;
    }

    if (dataset_map.count(name)) {
      PrintWarning("...", name, "already exists, skipping it");
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
  if (dynamic.enabled)
  {
    dynamic.exit_thread = true;
    dynamic.thread->join();
    dynamic.thread.reset();
  }
}

////////////////////////////////////////////////////////////////////////////////
SharedPtr<ModVisus::PublicDatasets> ModVisus::getDatasets()
{
  if (dynamic.enabled)
  {
    ScopedReadLock lock(dynamic.lock);
    return m_datasets;
  }
  else
  {
    return m_datasets;
  }
}

////////////////////////////////////////////////////////////////////////////////
void ModVisus::trackConfigChangesInBackground()
{
  PrintInfo("Tracking config changes", this->dynamic.filename, this->dynamic.msec);
  auto TIMESTAMP = FileUtils::getTimeLastModified(this->dynamic.filename);

  while (!this->dynamic.exit_thread)
  {
    auto timestamp = FileUtils::getTimeLastModified(this->dynamic.filename);

    if (TIMESTAMP == timestamp)
    {
      Thread::sleep(dynamic.msec);
      continue;
    }

    PrintInfo("config file", this->dynamic.filename,"changed");

    ConfigFile config;
    if (!config.load(this->config_filename))
    {
      PrintInfo("Reload", this->config_filename, "failed");
    }
    else
    {
      auto datasets = std::make_shared<PublicDatasets>(this, config);
      PrintInfo("Reload", this->config_filename, "ok", "#datasets", datasets->getNumberOfDatasets());

      //make this as fast as possible
      {
        ScopedWriteLock lock(dynamic.lock);
        this->m_datasets = datasets;
        TIMESTAMP = timestamp;
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
bool ModVisus::configureDatasets(const ConfigFile& config)
{
  this->dynamic.enabled = false;
  this->config_filename = config.getFilename();

  auto datasets = std::make_shared<PublicDatasets>(this, config);
  this->m_datasets = datasets;

  PrintInfo("ModVisus::configure", config_filename, "...");
  PrintInfo("/mod_visus?action=list\n", datasets->getDatasetsBody());

  //for in-memory configueation file I cannot reload from disk, so it does not make sense to configure dynamic
  if (!this->config_filename.empty())
  {
    if (config.getChild("Configuration/ModVisus/Dynamic"))
    {
      this->dynamic.enabled = config.readBool("Configuration/ModVisus/Dynamic/enabled", false);
      this->dynamic.msec = config.readInt("Configuration/ModVisus/Dynamic/msec", 3000);
      this->dynamic.filename = config.readString("Configuration/ModVisus/Dynamic/filename", this->config_filename);
    }
    else
    {
      this->dynamic.enabled = config.readBool("Configuration/ModVisus/dynamic", false);
      this->dynamic.filename = this->config_filename;
      this->dynamic.msec = 3000;
    }

    if (this->dynamic.enabled)
    {
      this->dynamic.thread = Thread::start("modvisus-trackConfigChangesInBackground", [this]() {
        this->trackConfigChangesInBackground();
      });
    }
  }

  return true;
}


///////////////////////////////////////////////////////////////////////////
NetResponse ModVisus::handleDynamicAddDataset(const NetRequest& request)
{
  //only for dynamic mode
  if (!this->dynamic.enabled)
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

  //using a file lock to make sure I have no collision on the same file
  //a reload will happen in the background thread soon or later
  //(lazy add-dataset)
  {
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

    //add a <dataset> child to the file
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
  }

  return NetResponse(HttpStatus::STATUS_OK);
}

///////////////////////////////////////////////////////////////////////////
NetResponse ModVisus::handleDynamicReload(const NetRequest& request)
{
  //only for dynamic mode
  if (!this->dynamic.enabled)
    return NetResponseError(HttpStatus::STATUS_BAD_REQUEST, "Mod visus is in non-dynamic mode");

  ConfigFile config;
  if (!config.load(this->config_filename))
  {
    PrintInfo("Reload modvisus config_filename", this->config_filename, "failed");
    return NetResponseError(HttpStatus::STATUS_INTERNAL_SERVER_ERROR, "Cannot reload");
  }

  auto datasets = std::make_shared<PublicDatasets>(this, config);

  //make this as fast as possible
  {
    ScopedWriteLock lock(dynamic.lock);
    this->m_datasets = datasets;
  }

  PrintInfo("reload done", this->config_filename, "#datasets", datasets->getNumberOfDatasets());
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

  //backward compatible (i.e. prefer the old format)
  if (dataset->getDatasetTypeName()=="IdxDataset" && request.url.getParam("format")!="xml")
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
  bool rowmajor = cbool(request.url.getParam("rowmajor", "0"));

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

  WaitAsync< Future<Void> > wait_async(/*max_running*/0);
  access->beginRead();
  Aborted aborted;

  std::vector<NetResponse> responses;
  for (auto blockid : blocks)
  {
    auto block_query = dataset->createBlockQuery(blockid, field, time, 'r', aborted);
    dataset->executeBlockQuery(access, block_query);
    wait_async.pushRunning(block_query->done,[block_query, &responses, dataset, compression, rowmajor](Void) {

      if (block_query->failed())
      {
        responses.push_back(NetResponseError(HttpStatus::STATUS_NOT_FOUND, "block_query->executeAndWait failed"));
        return;
      }

      //by default i return the block as it is,unless the users specified rowmajor in headers
      if (rowmajor)
        dataset->convertBlockQueryToRowMajor(block_query);

      //encode data
      NetResponse response(HttpStatus::STATUS_OK);
      if (!response.setArrayBody(compression, block_query->buffer))
      {
        responses.push_back(NetResponseError(HttpStatus::STATUS_INTERNAL_SERVER_ERROR, "Encoding converting to row major failed"));
        return;
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


  double accuracy = request.url.hasParam("accuracy")? 
    cdouble(request.url.getParam("accuracy")) :
      dataset->getDefaultAccuracy();

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

  //this is needed by VisusSlam (ref John and Steve)
  //this was the old code:
  //https://github.com/sci-visus/OpenVisus/commit/0c0ccf7235f8f5547bb2a4808cea60a697dac895
#if 1
  String palette = request.url.getParam("palette");
  if (!palette.empty() && buffer.dtype.ncomponents() == 1)
  {
    auto tf = TransferFunction::getDefault(palette);
    if (!tf)
    {
      VisusAssert(false);
      PrintInfo("invalid palette specified", palette);
      PrintInfo("use one of:");
      std::vector<String> tf_defaults = TransferFunction::getDefaults();
      for (int i = 0; i < tf_defaults.size(); i++)
        PrintInfo("\t", tf_defaults[i]);
    }
    else
    {
      double palette_min = cdouble(request.url.getParam("palette_min"));
      double palette_max = cdouble(request.url.getParam("palette_max"));

      if (palette_min != palette_max)
      {
        tf->beginTransaction();
        tf->setUserRange(Range(palette_min, palette_max, 0));
        tf->setNormalizationMode(TransferFunction::UserRange);
        tf->endTransaction();
      }

      buffer = tf->applyToArray(buffer);
      if (!buffer.valid())
        return NetResponseError(HttpStatus::STATUS_INTERNAL_SERVER_ERROR, "palette failed");
    }
  }
#endif

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


  auto accuracy = request.url.hasParam("accuracy") ?
    cdouble(request.url.getParam("accuracy")) :
    dataset->getDefaultAccuracy();

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
  query->accuracy = accuracy;

  dataset->beginPointQuery(query);

  if (!query->isRunning())
    return NetResponseError(HttpStatus::STATUS_BAD_REQUEST, "dataset->beginBoxQuery() failed " + query->errormsg);

  if (!query->setPoints(nsamples))
    return NetResponseError(HttpStatus::STATUS_BAD_REQUEST, "dataset->setPoints failed " + query->errormsg);

  auto access = dataset->createAccess();
  if (!dataset->executePointQuery(access, query))
    return NetResponseError(HttpStatus::STATUS_BAD_REQUEST, "dataset->executeBoxQuery() failed " + query->errormsg);

  buffer = query->buffer;

#if 1
  String palette = request.url.getParam("palette");
  if (!palette.empty() && buffer.dtype.ncomponents() == 1)
  {
    auto tf = TransferFunction::getDefault(palette);
    if (!tf)
    {
      VisusAssert(false);
      PrintInfo("invalid palette specified", palette);
      PrintInfo("use one of:");
      std::vector<String> tf_defaults = TransferFunction::getDefaults();
      for (int i = 0; i < tf_defaults.size(); i++)
        PrintInfo("\t", tf_defaults[i]);
    }
    else
    {
      double palette_min = cdouble(request.url.getParam("palette_min"));
      double palette_max = cdouble(request.url.getParam("palette_max"));

      if (palette_min != palette_max)
      {
        tf->setNormalizationMode(TransferFunction::UserRange);
        tf->setUserRange(Range(palette_min, palette_max, 0));
      }

      buffer = tf->applyToArray(buffer);
      if (!buffer.valid())
        return NetResponseError(HttpStatus::STATUS_INTERNAL_SERVER_ERROR, "palette failed");
    }
  }
#endif

  NetResponse response(HttpStatus::STATUS_OK);
  if (!response.setArrayBody(compression, buffer))
    return NetResponseError(HttpStatus::STATUS_INTERNAL_SERVER_ERROR, "NetResponse encodeBuffer failed");

  return response;
}

///////////////////////////////////////////////////////////////////////////
NetResponse ModVisus::handleRequest(NetRequest request)
{
  Time t1 = Time::now();

  String action = request.url.getParam("action");

  //default action
  if (action.empty())
    action= request.url.hasParam("dataset") ? "readdataset" : "list";

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

  else if (action == "ping")
  {
    response = NetResponse(HttpStatus::STATUS_OK);
    response.setHeader("block-query-support-aggregation", "1");
  }

  ////////////////////////// DEPRECATED, do not use. Use COnfiguration/ModVisus/Dynamic instead
#if 1

  else if (action == "configure_datasets" || action == "configure" || action == "reload")
    response = handleDynamicReload(request);

  else if (action == "AddDataset" || action == "add_dataset")
    response = handleDynamicAddDataset(request);

#endif

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
