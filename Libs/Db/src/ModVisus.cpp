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
#include <Visus/Scene.h>
#include <Visus/DatasetFilter.h>
#include <Visus/File.h>
#include <Visus/TransferFunction.h>
#include <Visus/NetService.h>
#include <Visus/VisusConfig.h>
#include <Visus/ApplicationInfo.h>

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

  RWLock lock;

  //constructor
  Datasets() : stree("datasets") {
  }

  //destructor
  ~Datasets() {
  }

  //getUrl
  String getUrl(String name) {
    return "$(protocol)://$(hostname):$(port)/mod_visus?action=readdataset&dataset=" + name;
  }

  //load
  bool load(const StringTree& stree, bool bClear = true)
  {
    ScopedWriteLock write_lock(this->lock);
    if (bClear)
    {
      this->map.clear();
      this->stree = StringTree("datasets");
    }
    return addDatasets(this->stree, stree)>0;
  }

  //getBody
  String getBody(String format = "xml")
  {
    ScopedReadLock read_lock(this->lock);
    if (format == "json")
      return stree.toJSONString();
    return stree.toString();
  }

  //find
  SharedPtr<Dataset> find(String name)
  {
    if (name.empty())
      return SharedPtr<Dataset>();

    for (auto bReload : { 0,1 })
    {
      if (bReload)
      {
        if (!VisusConfig::getSingleton()->reload())
          continue;

        //TODO: (optimization for reloads) only add new public datasets by diffing entries in string tree instead of reconfiguring all datasets
        this->load(*VisusConfig::getSingleton());
      }

      {
        ScopedReadLock read_lock(this->lock);
        auto it = this->map.find(name);
        if (it != this->map.end())
          return it->second;
      }
    }

    return SharedPtr<Dataset>();
  }

private:

  VISUS_NON_COPYABLE_CLASS(Datasets)

    typedef std::map<String, SharedPtr<Dataset > > Map;

  Map               map;
  StringTree        stree;

  //addDataset (Need write lock)
  int addDataset(StringTree& dst, String name, SharedPtr<Dataset> dataset)
  {
    this->map[name] = dataset;

    StringTree* child = dst.addChild(StringTree("dataset"));

    //for example kdquery=true could be maintained!
    child->attributes = dataset->getConfig().attributes;

    child->writeString("name", name);
    child->writeString("url", getUrl(name));

    dataset->bServerMode = true;

    //automatically add the childs of a multiple datasets
    int ret = 1;
    for (auto it : dataset->getInnerDatasets())
      ret += addDataset(*child, name + "/" + it.first, it.second);
    return ret;
  }

  //addDatasets (need write lock)
  int addDatasets(StringTree& dst, const StringTree& src)
  {
    int ret = 0;

    //I want to maintain the group hierarchy!
    if (src.name == "group")
    {
      StringTree group(src.name);
      group.attributes = src.attributes;
      for (auto child : src.getChilds())
        ret += addDatasets(group, *child);
      if (ret)
        dst.addChild(group);
      return ret;
    }

    //flattening the hierarchy!
    if (src.name != "dataset") {
      for (auto child : src.getChilds())
        ret += addDatasets(dst, *child);
      return ret;
    }

    //just ignore those with empty names or not public
    String name = src.readString("name");
    bool is_public = StringUtils::contains(src.readString("permissions"), "public");
    if (name.empty() || !is_public)
      return 0;

    auto dataset = Dataset::loadDataset(name);
    if (!dataset) {
      VisusWarning() << "dataset name(" << name << ") load failed, skipping it";
      return 0;
    }

    if (map.find(name) != map.end()) {
      VisusWarning() << "dataset name(" << name << ")  already exists, skipping it";
      return 0;
    }

    return addDataset(dst, name, dataset);
  }

};


///////////////////////////////////////////////////////////////////////////////
class ModVisus::Scenes
{
public:

  //constructor
  Scenes() : stree("scenes") {
  }

  //destructor
  ~Scenes() {
  }

  //getUrl
  String getUrl(String name) {
    return "$(protocol)://$(hostname):$(port)/mod_visus?action=readscene&scene=" + name;
  }

  //load
  bool load(const StringTree& stree, bool bClear = true)
  {
    ScopedWriteLock write_lock(this->lock);
    if (bClear)
    {
      this->map.clear();
      this->stree = StringTree("scenes");
    }
    return addScenes(this->stree, stree)>0;
  }

  //getBody
  String getBody(String format = "xml")
  {
    ScopedReadLock read_lock(this->lock);
    if (format == "json")
      return stree.toJSONString();
    return stree.toString();
  }

  //find
  SharedPtr<Scene> find(String name)
  {
    ScopedReadLock read_lock(this->lock);
    auto it = this->map.find(name);
    return it != this->map.end() ? it->second : SharedPtr<Scene>();
  }

private:

  VISUS_NON_COPYABLE_CLASS(Scenes)

    typedef std::map<String, SharedPtr<Scene > > Map;

  RWLock            lock;
  Map               map;
  StringTree        stree;

  //addScene
  int addScene(StringTree& dst, String name,SharedPtr<Scene> scene)
  {
    this->map[name] = scene;

    StringTree* child = dst.addChild(StringTree("scene"));
    child->writeString("name", name);
    child->writeString("url", getUrl(name));

    return 1;
  }

  //addScenes (need write lock)
  int addScenes(StringTree& dst, const StringTree& src)
  {
    int ret = 0;

    //I want to maintain the group hierarchy!
    if (src.name == "group")
    {
      auto group = StringTree(src.name);
      group.attributes = src.attributes;
      for (auto child : src.getChilds())
        ret += addScenes(group, *child);
      if (ret)
        dst.addChild(group);
      return ret;
    }

    //flattening the hierarchy!
    if (src.name != "scene")
    {
      for (auto child : src.getChilds())
        ret += addScenes(dst, *child);
      return ret;
    }

    //just ignore those with empty names or not public
    String name = src.readString("name");
    bool is_public = StringUtils::contains(src.readString("permissions"), "public");
    if (name.empty() || !is_public)
      return 0;

    auto scene = Scene::loadScene(name);
    if (!scene) {
      VisusWarning() << "Scene::loadScene(" << name << ") failed, skipping it";
      return 0;
    }

    if (map.find(name) != map.end()) {
      VisusWarning() << "Scene name(" << name << ") already exists, skipping it";
      return 0;
    }

    return addScene(dst, name, scene);
  }

};


////////////////////////////////////////////////////////////////////////////////
ModVisus::ModVisus()
{
  datasets = std::make_shared<Datasets>();
  scenes = std::make_shared<Scenes>();
}

////////////////////////////////////////////////////////////////////////////////
ModVisus::~ModVisus()
{}

////////////////////////////////////////////////////////////////////////////////
bool ModVisus::configureDatasets()
{
  VisusInfo() << "ModVisus::configureDatasets()...";

  VisusConfig::getSingleton()->reload();

  datasets->load(*VisusConfig::getSingleton());
  scenes->load(*VisusConfig::getSingleton());

  VisusInfo() << "/mod_visus?action=list\n" << datasets->getBody();
  VisusInfo() << "/mod_visus?action=list_scenes\n" << scenes->getBody();

  return true;
}


///////////////////////////////////////////////////////////////////////////
NetResponse ModVisus::handleConfigureDatasets(const NetRequest& request)
{
  if (!configureDatasets())
    return NetResponseError(HttpStatus::STATUS_INTERNAL_SERVER_ERROR, "Cannot read the visus.config");

  return NetResponse(HttpStatus::STATUS_OK);
}


///////////////////////////////////////////////////////////////////////////
NetResponse ModVisus::handleAddDataset(const NetRequest& request)
{
  bool bOk = false;

  bool bPersistent = cbool(request.url.getParam("persistent", "true"));


  StringTree stree;
  if (request.url.hasParam("name"))
  {
    String name = request.url.getParam("name");
    String url = request.url.getParam("url");

    if (datasets->find(name))
      return NetResponseError(HttpStatus::STATUS_CONFLICT, "Cannot add dataset(" + name + ") because it already exists");

    stree = StringTree("dataset");
    stree.writeString("name", name);
    stree.writeString("url", url);
    stree.writeString("permissions", "public");
  }
  else if (request.url.hasParam("xml"))
  {
    String xml = request.url.getParam("xml");
    if (!stree.fromXmlString(xml))
      return NetResponseError(HttpStatus::STATUS_BAD_REQUEST, "Cannot decode xml");

    String name = stree.readString("name");
    if (datasets->find(name))
      return NetResponseError(HttpStatus::STATUS_CONFLICT, "Cannot add dataset(" + name + ") because it already exists");
  }

  //try to add
  {
    ScopedWriteLock write_lock(datasets->lock);

    //add src to visus.config in memory (this is necessary for Dataset::loadDataset(name))
    VisusConfig::getSingleton()->addChild(stree);

    //add src to visus.config on the file system (otherwise when mod_visus restarts it will be lost)
    if (bPersistent)
    {
      String     filename = VisusConfig::getSingleton()->filename;
      StringTree visus_config;
      if (!visus_config.fromXmlString(Utils::loadTextDocument(filename), /*bEnablePostProcessing*/false))
      {
        VisusWarning() << "Cannot load visus.config";
        VisusAssert(false);//TODO rollback
        return NetResponseError(HttpStatus::STATUS_BAD_REQUEST, "Add dataset failed");
      }

      visus_config.addChild(stree);

      if (!Utils::saveTextDocument(filename, visus_config.toString()))
      {
        VisusWarning() << "Cannot save new visus.config";
        VisusAssert(false);//TODO rollback
        return NetResponseError(HttpStatus::STATUS_BAD_REQUEST, "Add dataset failed");
      }
    }
  }

  if (!datasets->load(stree,/*bClear*/false))
    return NetResponseError(HttpStatus::STATUS_BAD_REQUEST, "Add dataset failed");

  return NetResponse(HttpStatus::STATUS_OK);
}


///////////////////////////////////////////////////////////////////////////
NetResponse ModVisus::handleReadDataset(const NetRequest& request)
{
  String dataset_name = request.url.getParam("dataset");

  auto dataset = datasets->find(dataset_name);
  if (!dataset)
    return NetResponseError(HttpStatus::STATUS_NOT_FOUND, "Cannot find dataset(" + dataset_name + ")");

  NetResponse response(HttpStatus::STATUS_OK);
  response.setHeader("visus-git-revision", ApplicationInfo::git_revision);
  response.setHeader("visus-typename", dataset->getTypeName());

  auto body = dataset->getDatasetBody();

  //fix urls (this is needed for midx where I need to remap urls)
  if (bool bIsXml = StringUtils::startsWith(body, "<"))
  {
    StringTree stree;
    if (stree.fromXmlString(body))
    {
      VisusReleaseAssert(stree.name=="dataset");
      stree.writeString("name", dataset_name);

      std::stack< std::pair<String,StringTree*>> stack;
      stack.push(std::make_pair("",&stree));
      while (!stack.empty())
      {
        auto prefix = stack.top().first;
        auto stree  = stack.top().second; 
        stack.pop();
        if (stree->name == "dataset" && stree->hasValue("name"))
        {
          prefix = prefix + (prefix.empty() ? "" : "/") + stree->readString("name");
          stree->writeString("url", datasets->getUrl(prefix));
        }
        for (auto child : stree->getChilds())
          stack.push(std::make_pair(prefix, child));
      }
      body = stree.toString();
    }
  }

  response.setTextBody(body,/*bHasBinary*/true);
  return response;
}

///////////////////////////////////////////////////////////////////////////
NetResponse ModVisus::handleReadScene(const NetRequest& request)
{
  String scene_name = request.url.getParam("scene");

  auto scene = scenes->find(scene_name);
  if (!scene)
    return NetResponseError(HttpStatus::STATUS_NOT_FOUND, "Cannot find scene(" + scene_name + ")");

  NetResponse response(HttpStatus::STATUS_OK);
  response.setHeader("visus-git-revision", ApplicationInfo::git_revision);
  //response.setHeader("visus-typename", scene->getTypeName());

  auto body = scene->getSceneBody();

  //scrgiorgio: do i need to fix url here?

  response.setTextBody(body,/*bHasBinary*/true);
  return response;
}


///////////////////////////////////////////////////////////////////////////
NetResponse ModVisus::handleGetListOfDatasets(const NetRequest& request)
{
  String format = request.url.getParam("format", "xml");
  String hostname = request.url.getParam("hostname"); //trick if you want $(localhost):$(port) to be replaced with what the client has
  String port = request.url.getParam("port");

  NetResponse response(HttpStatus::STATUS_OK);

  VisusConfig::getSingleton()->reload();

  if (format == "xml")
    response.setXmlBody(datasets->getBody(format));
  else if (format == "json")
    response.setJSONBody(datasets->getBody(format));
  else
    return NetResponseError(HttpStatus::STATUS_NOT_FOUND, "wrong format(" + format + ")");

  if (!hostname.empty())
    response.setTextBody(StringUtils::replaceAll(response.getTextBody(), "$(hostname)", hostname));

  if (!port.empty())
    response.setTextBody(StringUtils::replaceAll(response.getTextBody(), "$(port)", port));

  return response;
}

///////////////////////////////////////////////////////////////////////////
NetResponse ModVisus::handleGetListOfScenes(const NetRequest& request)
{
  String format = request.url.getParam("format", "xml");
  String hostname = request.url.getParam("hostname"); //trick if you want $(localhost):$(port) to be replaced with what the client has
  String port = request.url.getParam("port");

  NetResponse response(HttpStatus::STATUS_OK);

  VisusConfig::getSingleton()->reload();

  if (format == "xml")
    response.setXmlBody(scenes->getBody(format));
  else if (format == "json")
    response.setJSONBody(scenes->getBody(format));
  else
    return NetResponseError(HttpStatus::STATUS_NOT_FOUND, "wrong format(" + format + ")");

  if (!hostname.empty())
    response.setTextBody(StringUtils::replaceAll(response.getTextBody(), "$(hostname)", hostname));

  if (!port.empty())
    response.setTextBody(StringUtils::replaceAll(response.getTextBody(), "$(port)", port));

  return response;
}


///////////////////////////////////////////////////////////////////////////
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


///////////////////////////////////////////////////////////////////////////
NetResponse ModVisus::handleOpenSeaDragon(const NetRequest& request)
{
  String dataset_name = request.url.getParam("dataset");
  String compression = request.url.getParam("compression", "png");
  String debugMode = request.url.getParam("debugMode", "false");
  String showNavigator = request.url.getParam("showNavigator", "true");

  auto dataset = datasets->find(dataset_name);
  if (!dataset)
    return NetResponseError(HttpStatus::STATUS_NOT_FOUND, "Cannot find dataset(" + dataset_name + ")");

  if (dataset->getPointDim() != 2)
    return NetResponseError(HttpStatus::STATUS_BAD_REQUEST, "dataset(" + dataset_name + ") has dimension !=2");

  int w = (int)dataset->getBox().p2[0];
  int h = (int)dataset->getBox().p2[1];
  int maxh = dataset->getBitmask().getMaxResolution();
  int bitsperblock = dataset->getDefaultBitsPerBlock();

  std::ostringstream out;
  out
    << "<html>" << std::endl
    << "<head>" << std::endl
    << "<meta charset='utf-8'>" << std::endl
    << "<title>Visus OpenSeaDragon</title>" << std::endl
    << "<script src='https://openseadragon.github.io/openseadragon/openseadragon.min.js'></script>" << std::endl
    << "<style>.openseadragon {background-color: gray;}</style>" << std::endl
    << "</head>" << std::endl

    << "<body>" << std::endl
    << "<div id='osd1' class='openseadragon' style='width:100%; height:100%;'>" << std::endl

    << "<script type='text/javascript'>" << std::endl
    << "base_url = window.location.protocol + '//' + window.location.host + '/mod_visus?action=boxquery&dataset=" << dataset_name << "&compression=" << compression << "';" << std::endl
    << "w = " << w << ";" << std::endl
    << "h = " << h << ";" << std::endl
    << "maxh = " << maxh << ";" << std::endl
    << "bitsperblock = " << bitsperblock << ";" << std::endl

    //try to align as much as possible to the block shape: we know that a block
    //has 2^bitsperblock samples. Assuming the bitmask is balanced at the end (i.e. V.....01010101)
    //then we simply subdivide the domain in squares with tileSize^2=2^bitsperblock -> tileSize is about 2^(bitsperblock/2)

    << "tileSize = Math.pow(2,bitsperblock/2);" << std::endl
    << "minLevel=bitsperblock/2;" << std::endl
    << "maxLevel=maxh/2;" << std::endl
    << "OpenSeadragon({" << std::endl
    << "  id: 'osd1'," << std::endl
    << "  prefixUrl: 'https://raw.githubusercontent.com/openseadragon/svg-overlay/master/openseadragon/images/'," << std::endl
    << "  showNavigator: " << showNavigator << "," << std::endl
    << "  debugMode: " << debugMode << "," << std::endl
    << "  tileSources: {" << std::endl
    << "    height: h, width:  w, tileSize: tileSize, minLevel: minLevel, maxLevel: maxLevel," << std::endl
    << "    getTileUrl: function(level,x,y) {" << std::endl

    // trick to return an image with resolution (tileSize*tileSize) at a certain resolution
    // when level=maxLevel I just use the tileSize
    // when I go up one level I need to use 2*tileSize in order to have the same number of samples

    << "      lvlTileSize = tileSize*Math.pow(2, maxLevel-level);" << std::endl
    << "    	 x1 = Math.min(lvlTileSize*x,w); x2 = Math.min(x1 + lvlTileSize, w);" << std::endl
    << "    	 y1 = Math.min(lvlTileSize*y,h); y2 = Math.min(y1 + lvlTileSize, h);" << std::endl
    << "    	 return base_url" << std::endl
    << "    	   + '&box='+x1+'%20'+(x2-1)+'%20'+(h -y2)+'%20'+(h-y1-1)" << std::endl
    << "    		 + '&toh=' + level*2" << std::endl
    << "    		 + '&maxh=' + maxh ;" << std::endl
    << "}}});" << std::endl

    << "</script>" << std::endl
    << "</div>" << std::endl
    << "</body>" << std::endl
    << "</html>;" << std::endl;

  NetResponse response(HttpStatus::STATUS_OK);
  response.setHtmlBody(out.str());
  return response;
}


///////////////////////////////////////////////////////////////////////////
NetResponse ModVisus::handleBlockQuery(const NetRequest& request)
{
  String dataset_name = request.url.getParam("dataset");

  auto dataset = datasets->find(dataset_name);
  if (!dataset)
    return NetResponseError(HttpStatus::STATUS_NOT_FOUND, "Cannot find dataset(" + dataset_name + ")");

  String compression = request.url.getParam("compression");
  String fieldname = request.url.getParam("field", dataset->getDefaultField().name);
  double time = cdouble(request.url.getParam("time", cstring(dataset->getDefaultTime())));

  std::vector<BigInt> start_address; for (auto it : StringUtils::split(request.url.getParam("from", "0"))) start_address.push_back(cbigint(it));
  std::vector<BigInt> end_address; for (auto it : StringUtils::split(request.url.getParam("to", "0"))) end_address.push_back(cbigint(it));

  if (start_address.empty())
    return NetResponseError(HttpStatus::STATUS_NOT_FOUND, "start_address.empty()");

  if (start_address.size() != end_address.size())
    return NetResponseError(HttpStatus::STATUS_NOT_FOUND, "start_address.size()!=end_address.size()");

  Field field = fieldname.empty() ? dataset->getDefaultField() : dataset->getFieldByName(fieldname);
  if (!field.valid())
    return NetResponseError(HttpStatus::STATUS_NOT_FOUND, "Cannot find field(" + fieldname + ")");

  bool bHasFilter = !field.filter.empty();

  auto access = dataset->createAccessForBlockQuery();

  WaitAsync< Future<Void> > wait_async;
  access->beginRead();
  Aborted aborted;

  std::vector<NetResponse> responses;
  for (int I = 0; I < (int)start_address.size(); I++)
  {
    auto block_query = std::make_shared<BlockQuery>(field, time, start_address[I], end_address[I], aborted);
    wait_async.pushRunning(dataset->readBlock(access, block_query)).when_ready([block_query, &responses, dataset, compression](Void) {

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
NetResponse ModVisus::handleQuery(const NetRequest& request)
{
  String dataset_name = request.url.getParam("dataset");
  int maxh = cint(request.url.getParam("maxh"));
  int fromh = cint(request.url.getParam("fromh"));
  int endh = cint(request.url.getParam("toh"));
  String fieldname = request.url.getParam("field");
  double time = cdouble(request.url.getParam("time"));
  String compression = request.url.getParam("compression");
  bool   bDisableFilters = cbool(request.url.getParam("disable_filters"));
  bool   bKdBoxQuery = request.url.getParam("kdquery") == "box";
  String action = request.url.getParam("action");

  String palette = request.url.getParam("palette");
  double palette_min = cdouble(request.url.getParam("palette_min"));
  double palette_max = cdouble(request.url.getParam("palette_max"));
  String palette_interp = (request.url.getParam("palette_interp"));

  auto dataset = datasets->find(dataset_name);
  if (!dataset)
    return NetResponseError(HttpStatus::STATUS_NOT_FOUND, "Cannot find dataset(" + dataset_name + ")");

  int pdim = dataset->getPointDim();

  Field field = fieldname.empty() ? dataset->getDefaultField() : dataset->getFieldByName(fieldname);
  if (!field.valid())
    return NetResponseError(HttpStatus::STATUS_BAD_REQUEST, "Cannot find fieldname(" + fieldname + ")");

  auto query = std::make_shared<Query>(dataset.get(), 'r');
  query->time = time;
  query->field = field;
  query->start_resolution = fromh;
  query->end_resolutions = { endh };
  query->max_resolution = maxh;
  query->aborted = Aborted(); //TODO: how can I get the aborted from network?

                              //I apply the filter on server side only for the first coarse query (more data need to be processed on client side)
  if (fromh == 0 && !bDisableFilters)
  {
    query->filter.enabled = true;
    query->filter.domain = (bKdBoxQuery ? dataset->getBitmask().getPow2Box() : dataset->getBox());
  }
  else
  {
    query->filter.enabled = false;
  }

  //position
  if (action == "boxquery")
  {
    query->position = Position(NdBox::parseFromOldFormatString(pdim, request.url.getParam("box")));
  }
  else if (action == "pointquery")
  {
    Matrix  map = Matrix(request.url.getParam("matrix"));
    Box3d   box = Box3d::parseFromString(request.url.getParam("box"));
    query->position = (Position(map, box));
  }
  else
  {
    //TODO
    VisusAssert(false);
  }

  //query failed
  if (!dataset->beginQuery(query))
    return NetResponseError(HttpStatus::STATUS_BAD_REQUEST, "dataset->beginQuery() failed " + query->getLastErrorMsg());

  auto access = dataset->createAccess();
  if (!dataset->executeQuery(access, query))
    return NetResponseError(HttpStatus::STATUS_BAD_REQUEST, "dataset->executeQuery() failed " + query->getLastErrorMsg());

  auto buffer = query->buffer;

  //useful for kdquery=box (for example with discrete wavelets, don't want the extra channel)
  if (bKdBoxQuery)
  {
    if (auto filter = query->filter.value)
      buffer = filter->dropExtraComponentIfExists(buffer);
  }

  if (!palette.empty() && buffer.dtype.ncomponents() == 1)
  {
    TransferFunction tf;
    if (!tf.setDefault(palette))
    {
      VisusAssert(false);
      VisusInfo() << "invalid palette specified: " << palette;
      VisusInfo() << "use one of:";
      std::vector<String> tf_defaults = TransferFunction::getDefaults();
      for (int i = 0; i<tf_defaults.size(); i++)
        VisusInfo() << "\t" << tf_defaults[i];
    }
    else
    {
      if (palette_min != palette_max)
        tf.input_range = ComputeRange::createCustom(palette_min, palette_max);

      if (!palette_interp.empty())
        tf.interpolation.set(palette_interp);

      buffer = tf.applyToArray(buffer);
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
    bool bSpecifyScene = request.url.hasParam("scene");
    bool bCommercialBrower = !user_agent.empty() && !StringUtils::contains(user_agent, "visus");

    if (bCommercialBrower)
    {
      if (bSpecifyDataset)
      {
        request.url.setParam("action", "plugin");
      }
      else if (bSpecifyScene)
      {
        request.url.setParam("action", "readscene");
      }
      else
      {
        request.url.setParam("action", "list");
        request.url.setParam("format", "html");
      }
    }
    else
    {
      if (bSpecifyDataset)
      {
        request.url.setParam("action", "readdataset");
      }
      else if (bSpecifyScene)
      {
        request.url.setParam("action", "readscene");
      }
      else
      {
        request.url.setParam("action", "list");
        request.url.setParam("format", "xml");
      }
    }
  }

  String action = request.url.getParam("action");

  NetResponse response;

  if (action == "rangequery" || action == "blockquery")
    response = handleBlockQuery(request);

  else if (action == "query" || action == "boxquery" || action == "pointquery")
    response = handleQuery(request);

  else if (action == "readdataset" || action == "read_dataset")
    response = handleReadDataset(request);

  else if (action == "readscene" || action == "read_scene")
    response = handleReadScene(request);

  else if (action == "plugin")
    response = handleHtmlForPlugin(request);

  else if (action == "list")
    response = handleGetListOfDatasets(request);

  else if (action == "listscenes" || action == "list_scenes")
    response = handleGetListOfScenes(request);

  else if (action == "configure_datasets")
    response = handleConfigureDatasets(request);

  else if (action == "AddDataset" || action == "add_dataset")
    response = handleAddDataset(request);

  else if (action == "openseadragon")
    response = handleOpenSeaDragon(request);

  else if (action == "ping")
  {
    response = NetResponse(HttpStatus::STATUS_OK);
    response.setHeader("block-query-support-aggregation", "1");
  }

  else
    response = NetResponseError(HttpStatus::STATUS_NOT_FOUND, "unknown action(" + action + ")");

  VisusInfo()
    << " request(" << request.url.toString() << ") "
    << " status(" << response.getStatusDescription() << ") body(" << StringUtils::getStringFromByteSize(response.body ? response.body->c_size() : 0) << ") msec(" << t1.elapsedMsec() << ")";

  //add some standard header
  response.setHeader("git_revision", ApplicationInfo::git_revision);
  response.setHeader("version", cstring(ApplicationInfo::version));

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
