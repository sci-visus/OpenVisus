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

#include <Visus/IdxMultipleDataset.h>
#include <Visus/ModVisusAccess.h>
#include <Visus/Path.h>
#include <Visus/ThreadPool.h>
#include <Visus/ApplicationInfo.h>
#include <Visus/IdxMosaicAccess.h>
#include <Visus/Polygon.h>

#if VISUS_PYTHON
#include <Visus/Python.h>
#endif

#include <atomic>

namespace Visus {

///////////////////////////////////////////////////////////////////////////////////////
class IdxMultipleAccess : 
  public Access, 
  public std::enable_shared_from_this<IdxMultipleAccess>
{
public:

  VISUS_NON_COPYABLE_CLASS(IdxMultipleAccess)

  //constructor
  IdxMultipleAccess(IdxMultipleDataset* VF_, StringTree CONFIG_)
  : VF(VF_), CONFIG(CONFIG_)
  {
    VisusAssert(!VF->bMosaic);

    this->name = CONFIG.readString("name", "IdxMultipleAccess");
    this->can_read = true;
    this->can_write = false;
    this->bitsperblock = VF->getDefaultBitsPerBlock();

    for (auto child : VF->childs)
    {
      auto name = child.first;

      //see if the user specified how to access the data for each query dataset
      for (auto it : CONFIG.getChilds())
      {
        if (it->name == name || it->readString("name") == name)
          configs[std::make_pair(name, "")] = *it;
      }
    }

    bool disable_async = CONFIG.readBool("disable_async", VF->isServerMode());

    //TODO: special case when I can use the blocks
    //if (VF->childs.size() == 1 && VF->sameLogicSpace(VF->childs[0]))
    //  ;

    if (int nthreads= disable_async ? 0 : 3)
      this->thread_pool = std::make_shared<ThreadPool>("IdxMultipleAccess Worker",nthreads);
  }

  //destructor
  virtual ~IdxMultipleAccess()
  {
    thread_pool.reset();
  }

  //createDownAccess
  SharedPtr<Access> createDownAccess(String name, String fieldname) 
  {
    auto dataset = VF->getChild(name).dataset;
    VisusAssert(dataset);

    SharedPtr<Access> ret;

    StringTree config = dataset->getDefaultAccessConfig();

    auto it = this->configs.find(std::make_pair(name, fieldname));
    if (it==configs.end())
      it = configs.find(std::make_pair(name, ""));

    if (it != configs.end())
      config = it->second;

    config.inheritAttributeFrom(this->CONFIG);
    bool bForBlockQuery = VF->getKdQueryMode() & KdQueryMode::UseBlockQuery ? true : false;
    return dataset->createAccess(config, bForBlockQuery);
  }

  //writeBlock (not supported)
  virtual void writeBlock(SharedPtr<BlockQuery> BLOCKQUERY) override {
    //not supported
    VisusAssert(false);
    writeFailed(BLOCKQUERY);
  }

private:

  IdxMultipleDataset*                              VF = nullptr;
  StringTree                                       CONFIG;
  std::map< std::pair<String, String>, StringTree> configs;
  SharedPtr<ThreadPool>                            thread_pool;

  //readBlockInThread
  void readBlockInThread(SharedPtr<BlockQuery> BLOCKQUERY)
  {
    if (BLOCKQUERY->aborted())
      return readFailed(BLOCKQUERY);

    /*
    TODO: can be async block query be enabled for simple cases?
     (like: I want to cache blocks for dw datasets)
     if all the childs are bSameLogicSpace I can do the blending of the blocks
     instead of the blending of the buffer of regular queries.
    
    To tell the truth i'm not sure if this solution would be different from what
    I'm doing right now (except that I can go async)
    */
    auto QUERY = VF->createEquivalentQuery('r', BLOCKQUERY);

    if (!VF->beginQuery(QUERY))
      return readFailed(BLOCKQUERY);

    if (!VF->executeQuery(shared_from_this(), QUERY))
      return readFailed(BLOCKQUERY);

    BLOCKQUERY->buffer = QUERY->buffer;
    return readOk(BLOCKQUERY);
  };

  //readBlock 
  virtual void readBlock(SharedPtr<BlockQuery> BLOCKQUERY) override
  {
    ThreadPool::push(thread_pool, [this, BLOCKQUERY]() {
      readBlockInThread(BLOCKQUERY);
    });
  }

}; //end class


////////////////////////////////////////////////////////
#if VISUS_PYTHON
class QueryInputTerm
{
public:

  VISUS_NON_COPYABLE_CLASS(QueryInputTerm)

  IdxMultipleDataset*      VF;
  Query*                   QUERY;
  SharedPtr<Access>        ACCESS;

  SharedPtr<PythonEngine>  engine;
  Aborted                  aborted;

  struct
  {
    Int64 max_publish_msec = 1000;
    Int64 last_publish_time = -1;
  }
  incremental;

  //constructor
  QueryInputTerm(IdxMultipleDataset* VF_, Query* QUERY_, SharedPtr<Access> ACCESS_, Aborted aborted_)
    : VF(VF_), QUERY(QUERY_), ACCESS(ACCESS_), aborted(aborted_) {

    VisusAssert(!VF->bMosaic);

    this->engine = (!VF->isServerMode())? VF->python_engine_pool->createEngine() : std::make_shared<PythonEngine>();

    {
      ScopedAcquireGil acquire_gil;

      engine->execCode(
        "class DynamicObject:\n"
        "  def __getattr__(self, args) : return self.forwardGetAttr(args)\n"
        "  def __getitem__(self, args) : return self.forwardGetAttr(args)\n"
      );

      auto py_input = newDynamicObject([this](String expr1) {
        return getAttr1(expr1);
      });
      engine->setModuleAttr("input", py_input);
      Py_DECREF(py_input);

      //for fieldname=function_of(QUERY->time) 
      //NOTE: for getFieldByName(), I think I can use the default timestep since I just want to know the dtype
      engine->setModuleAttr("query_time", QUERY ? QUERY->time : VF->getTimesteps().getDefault());

      engine->addModuleFunction("doPublish", [this](PyObject *self, PyObject *args) {
        return doPublish(self, args);
      });

      engine->addModuleFunction("voronoi",      [this](PyObject *self, PyObject *args) {return blendBuffers(self, args, BlendBuffers::VororoiBlend); });
      engine->addModuleFunction("averageBlend", [this](PyObject *self, PyObject *args) {return blendBuffers(self, args, BlendBuffers::AverageBlend); });
      engine->addModuleFunction("noBlend"     , [this](PyObject *self, PyObject *args) {return blendBuffers(self, args, BlendBuffers::NoBlend); });
    }
  }

  //destructor
  virtual ~QueryInputTerm()
  {
    {
      ScopedAcquireGil acquire_gil;
      engine->delModuleAttr("query_time");
      engine->delModuleAttr("doPublish");
      engine->delModuleAttr("voronoiBlend");
      engine->delModuleAttr("averageBlend");
      engine->delModuleAttr("noBlend");
      engine->delModuleAttr("input");
    }

    if (!VF->isServerMode())
      VF->python_engine_pool->releaseEngine(this->engine);
  }

  //computeOutput
  Array computeOutput(String code)
  {
    ScopedAcquireGil acquire_gil;
    engine->execCode(code);

    auto ret = engine->getModuleArrayAttr("output");
    if (!ret && !aborted())
      ThrowException("script does not assign 'output' value");

    if (VF->debug_mode & IdxMultipleDataset::DebugSaveImages)
    {
      static int cont = 0;
      ArrayUtils::saveImage(StringUtils::format() << "temp/" << cont++ << ".up.result.png", ret);
    }

    return ret;
  }

  //newDynamicObject
  PyObject* newDynamicObject(std::function<PyObject*(String)> getattr)
  {
    auto ret = engine->evalCode("DynamicObject()");  //new reference
    VisusAssert(ret);
    engine->addObjectMethod(ret, "forwardGetAttr", [getattr](PyObject*, PyObject* args) {

      VisusAssert(PyTuple_Check(args));
      VisusAssert(PyTuple_Size(args) == 1);
      auto arg0 = PyTuple_GetItem(args, 0); VisusAssert(arg0);//borrowed
      auto expr = PythonEngine::convertToString(arg0); VisusAssert(!expr.empty());
      if (!getattr) {
        PythonEngine::setError("getattr is null");
        return (PyObject*)nullptr;
      }
      return getattr(expr);
    });
    return ret;
  }

  //getAttr1
  PyObject* getAttr1(String expr1)
  {
    //example: input.timesteps
    if (expr1 == "timesteps")
      return engine->newPyObject(VF->getTimesteps().asVector());

    auto vf = VF->getChild(expr1).dataset;
    if (!vf)
      ThrowException(StringUtils::format() << "input['" << expr1 << "'] not found");

    auto ret = newDynamicObject([this, expr1](String expr2) {
      return getAttr2(expr1, expr2);
    });
    return ret;
  }

  //getAttr2
  PyObject* getAttr2(String expr1, String expr2)
  {
    auto child_dataset = VF->getChild(expr1);
    auto vf = child_dataset.dataset.get();
    VisusAssert(vf);

    auto M = child_dataset.M;

    //example: input.datasetname.timesteps
    if (expr2 == "timesteps")
      return engine->newPyObject(vf->getTimesteps().asVector());

    //see https://github.com/sci-visus/visus-issues/issues/367 
    //specify a vf  (see midxofmidx.midx)
    //EXAMPLE: output = input.first   ['output=input.A.temperature'];
    //EXAMPLE: output = input.first.                 A.temperature 
    if (auto midx = dynamic_cast<IdxMultipleDataset*>(vf))
    {
      if (midx->getChild(expr2).dataset)
      {
        auto ret = newDynamicObject([this, expr1, expr2](String expr3) {
          return getAttr2(expr1, StringUtils::join({ expr2 }, ".", "output=input.", "." + expr3 + ";"));
        });
        return ret;
      }
    }

    Array ret;

    //execute a query (expr2 is the fieldname)
    Field field = vf->getFieldByName(expr2);

    if (!field.valid())
      ThrowException(StringUtils::format() << "input['" << expr1 << "']['" << expr2 << "'] not found");

    int pdim = VF->getPointDim();

    //only getting dtype for field name
    if (!QUERY)
      return engine->newPyObject(Array(PointNi(pdim), field.dtype));

    {
      ScopedReleaseGil release_gil;
      auto down_query = createDownQuery(expr1, expr2);
      ret = executeDownQuery(down_query);
    }

    return engine->newPyObject(ret);
  }

  //createDownQuery
  SharedPtr<Query> createDownQuery(String name, String fieldname)
  {
    auto key = name +"/"+fieldname;

    //already created?
    auto it = QUERY->down_queries.find(key);
    if (it != QUERY->down_queries.end())
      return it->second;

    auto child = VF->childs[name];
    auto M     = child.M;
    auto vf    = child.dataset; VisusAssert(vf);
    auto field = vf->getFieldByName(fieldname); VisusAssert(field.valid());

    auto BOX = Position(M, vf->getLogicBox()).toAxisAlignedBox();

    //no intersection? just skip this down query
    if (!QUERY->position.toAxisAlignedBox().intersect(BOX))
      return SharedPtr<Query>();

    auto query = std::make_shared<Query>(vf.get(), 'r');
    QUERY->down_queries[key] = query;
    query->down_info.name = name;
    query->aborted = QUERY->aborted;
    query->time    = QUERY->time;
    query->field   = field;

    //euristic to find delta in the hzcurve
    //TODO!!!! this euristic produces too many samples
    auto VOLUME = Position(   VF->getLogicBox()).computeVolume();
    auto volume = Position(M, vf->getLogicBox()).computeVolume();
    int delta_h = -(int)log2(VOLUME / volume);

    //resolutions
    if (!QUERY->start_resolution)
      query->start_resolution = 0;
    else
      query->start_resolution = Utils::clamp(QUERY->start_resolution + delta_h, 0, vf->getMaxResolution()); //probably a block query

    std::set<int> end_resolutions;
    for (auto END_RESOLUTION : QUERY->end_resolutions)
    {
      auto end_resolution = Utils::clamp(END_RESOLUTION + delta_h, 0, vf->getMaxResolution());
      end_resolutions.insert(end_resolution);
    }
    query->end_resolutions = std::vector<int>(end_resolutions.begin(), end_resolutions.end());

    auto QUERY_T   = QUERY->position.getTransformation();
    auto QUERY_BOX = QUERY->position.getBoxNd();

    // WRONG, consider that M could have mat(3,0) | mat(3,1) | mat(3,2) !=0 and so I can have non-parallel axis
    // i.e. computing the bounding box in position very far from the mapped region are wrong because some axis of the quads can interect in some points
    // and I have an "inversion of axis" 
    // if you use this wrong version, for voronoi in 2d you will see some missing pieces around
    // solution is to limit the QUERY_BOX into a more "local" one
#if 1
    QUERY_BOX = Position(QUERY_T.invert(), M, vf->getLogicBox()).toAxisAlignedBox().getIntersection(QUERY_BOX);
#endif

    query->position = Position(M.invert(), QUERY_T, QUERY_BOX);

    //skip this argument since returns empty array
    if (!vf->beginQuery(query))
    {
      query->setFailed("cannot begin the query");
      return query;
    }

    //ignore missing timesteps
    if (!vf->getTimesteps().containsTimestep(query->time))
    {
      VisusInfo() << "Missing timestep(" << query->time << ") for input['" << name << "." << field.name << "']...ignoring it";
      query->setFailed("missing timestep");
      return query;
    }

    VisusAssert(!query->down_info.BUFFER);

    //if not multiple access i think it will be a pure remote query
    if (auto multiple_access = std::dynamic_pointer_cast<IdxMultipleAccess>(ACCESS))
      query->down_info.access = multiple_access->createDownAccess(name, field.name);

    return query;
  }

  //executeDownQuery
  Array executeDownQuery(SharedPtr<Query> query)
  {
    //VisusInfo() << Thread::getThreadId();

    //already failed
    if (!query || query->failed())
      return Array();

    auto name = query->down_info.name;

    auto child = VF->childs[name];
    auto M = child.M;
    auto vf = child.dataset; VisusAssert(vf);

    //NOTE if I cannot execute it probably reached max resolution for query, in that case I recycle old 'BUFFER'
    if (query->canExecute())
    {
      if (VF->debug_mode & IdxMultipleDataset::DebugSkipReading)
      {
        query->allocateBufferIfNeeded();
        ArrayUtils::setBufferColor(query->buffer, VF->childs[name].color);
        query->buffer.layout = ""; //row major
        query->currentLevelReady();
      }
      else
      {
        if (!vf->executeQuery(query->down_info.access, query))
        {
          query->setFailed("cannot execute the query");
          return Array();
        }
      }

      //VisusInfo() << "MIDX up nsamples(" << QUERY->nsamples.toString() << ") dw(" << name << "." << field.name << ") nsamples(" << query->buffer.dims.toString() << ")";

      //force resampling
      query->down_info.BUFFER = Array();
    }

    //already resampled
    if (query->down_info.BUFFER && query->down_info.BUFFER.dims == QUERY->nsamples)
      return query->down_info.BUFFER;

    //create a brand new BUFFER for doing the warpPerspective
    query->down_info.BUFFER = Array(QUERY->nsamples, query->buffer.dtype);
    query->down_info.BUFFER.fillWithValue(query->field.default_value);
    query->down_info.BUFFER.layout = ""; //row major
    query->down_info.BUFFER.bounds = Position(M, query->buffer.bounds);
    query->down_info.BUFFER.clipping = Position(M, query->buffer.clipping);

    query->down_info.BUFFER.alpha = std::make_shared<Array>(QUERY->nsamples, DTypes::UINT8);
    query->down_info.BUFFER.alpha->fillWithValue(0);

    auto PIXEL_TO_LOGIC = Position::computeTransformation(Position(QUERY->position), query->down_info.BUFFER.dims);
    auto pixel_to_logic = Position::computeTransformation(Position(query->position), query->buffer.dims);

    // Tperspective := PIXEL <- pixel
    auto LOGIC_TO_PIXEL = PIXEL_TO_LOGIC.invert();
    Matrix Tperspective = LOGIC_TO_PIXEL * M * pixel_to_logic;

    //this will help to find voronoi seams betweeen images
    query->down_info.LOGIC_TO_PIXEL = LOGIC_TO_PIXEL;
    query->down_info.PIXEL_TO_LOGIC = PIXEL_TO_LOGIC;
    query->down_info.logic_centroid = M * vf->getLogicBox().center();

    //limit the samples to good logic domain
    //explanation: for each pixel in dims, tranform it to the logic dataset box, if inside set the pixel to 1 otherwise set the pixel to 0
    if (!query->buffer.alpha)
      query->buffer.alpha = std::make_shared<Array>(ArrayUtils::createTransformedAlpha(vf->getLogicBox(), pixel_to_logic, query->buffer.dims, QUERY->aborted));
    else
      VisusReleaseAssert(query->buffer.alpha->dims==query->buffer.dims);

    if (!QUERY->aborted())
    {
      ArrayUtils::warpPerspective(query->down_info.BUFFER, Tperspective, query->buffer, QUERY->aborted);

      if (VF->debug_mode & IdxMultipleDataset::DebugSaveImages)
      {
        static int cont = 0;
        ArrayUtils::saveImage(StringUtils::format() << "temp/" << cont << ".dw." + name << "." << query->field.name << ".buffer.png", query->buffer);
        ArrayUtils::saveImage(StringUtils::format() << "temp/" << cont << ".dw." + name << "." << query->field.name << ".alpha_.png", *query->buffer.alpha );
        ArrayUtils::saveImage(StringUtils::format() << "temp/" << cont << ".up." + name << "." << query->field.name << ".buffer.png", query->down_info.BUFFER);
        ArrayUtils::saveImage(StringUtils::format() << "temp/" << cont << ".up." + name << "." << query->field.name << ".alpha_.png", *query->down_info.BUFFER.alpha);
        //ArrayUtils::setBufferColor(query->BUFFER, VF->childs[name].color);
        cont++;
      }
    }

    return query->down_info.BUFFER;
  }

  //blendBuffers
  PyObject* blendBuffers(PyObject *self, PyObject *args, BlendBuffers::Type type)
  {
    int argc = args ? (int)PyObject_Length(args) : 0;

    int pdim = VF->getPointDim();

    BlendBuffers blend(type, aborted);

    //preview only
    if (!QUERY)
    {
      if (!argc)
      {
        for (auto it : VF->childs)
        {
          auto arg = Array(PointNi(pdim), it.second.dataset->getDefaultField().dtype);
          blend.addBlendArg(arg);
        }
      }
      else
      {
        PyObject * arg0 = nullptr;
        if (!PyArg_ParseTuple(args, "O:blendBuffers", &arg0) )
        {
          PythonEngine::setError("invalid argument");
          return (PyObject*)nullptr;
        }

        if (!PyList_Check(arg0))
        {
          PythonEngine::setError("invalid argument");
          return (PyObject*)nullptr;
        }

        for (int I = 0, N = (int)PyList_Size(arg0); I < N; I++)
        {
          auto arg = engine->pythonObjectToArray(PyList_GetItem(arg0, I));
          blend.addBlendArg(arg);
        }
      }

      return engine->newPyObject(blend.result);
    }

    int num_args = 0;

    //special case: empty argument means all default fields of midx
    if (!argc)
    {
      ScopedReleaseGil release_gil;

      //this can run in parallel
      CriticalSection blendlock;

      std::vector< SharedPtr<Query> > queries;
      queries.reserve(VF->childs.size());
      for (auto it : VF->childs)
      {
        auto name      = it.first;
        auto fieldname = it.second.dataset->getDefaultField().name;
        auto query=createDownQuery(name,fieldname);
        if (query && !query->failed())
          queries.push_back(query);
      }

      //I don't see any advantage using OpenMP here
      //bool bRunInParallel = false;// VF->bServerMode ? false : true;
      //#pragma omp parallel for if(bRunInParallel) 
      for (int I = 0; I<(int)queries.size(); I++)
      {
        auto query = queries[I];
        executeDownQuery(query);

        if (query->down_info.BUFFER && !query->aborted()) 
        {
          ScopedLock scopedlock(blendlock);
          blend.addBlendArg(query->down_info.BUFFER, query->down_info.PIXEL_TO_LOGIC, query->down_info.logic_centroid);
          ++num_args;
        }
      }
    }
    else
    {
      //I need also the alpha so I'm using queries
      //parseArrayList(self, args);
      ScopedReleaseGil release_gil;

      for (auto it : QUERY->down_queries)
      {
        auto query = it.second;

        //failed/empty buffer
        if (query && query->down_info.BUFFER && query->aborted())
        {
          blend.addBlendArg(query->down_info.BUFFER, query->down_info.PIXEL_TO_LOGIC, query->down_info.logic_centroid);
          ++num_args;
        }
      }
    }
     
    if (!num_args) {
      PythonEngine::setError("empty argument");
      return (PyObject*)nullptr;
    }

    return engine->newPyObject(blend.result);
  }

  //doPublish
  PyObject* doPublish(PyObject *self, PyObject *args)
  {
    auto output = engine->getModuleArrayAttr("output");

    if (!output || !QUERY || !QUERY->incrementalPublish)
      return (PyObject*)nullptr;

    //postpost a little?
    auto current_time = Time::now().getUTCMilliseconds();
    if (incremental.last_publish_time > 0)
    {
      auto enlapsed_msec = current_time - incremental.last_publish_time;
      if (enlapsed_msec < incremental.max_publish_msec)
        return (PyObject*)nullptr;
    }

    Array copy;
    if (!ArrayUtils::deepCopy(copy, output)) {
      VisusAssert(false);
      return (PyObject*)nullptr;
    }
    output = copy;

    //equivalent of: output->shareProperties(*input); (when no input array is available, as in the query)
    // this->layout   = other.layout;
    output.bounds = QUERY->position;
    output.clipping = QUERY->clipping;

    //a projection happened?
    int pdim = QUERY->nsamples.getPointDim();
    for (int D = 0; D < pdim; D++)
    {
      if (output.dims[D] == 1 && QUERY->nsamples[D] > 1)
      {
        auto box = output.bounds.getBoxNd();
        box.p2[D] = box.p1[D];
        output.bounds = Position(output.bounds.getTransformation(), box);
        output.clipping = Position::invalid(); //disable clipping
      }
    }

    incremental.last_publish_time = current_time;
    QUERY->incrementalPublish(output);

    return (PyObject*)nullptr;
  }

};
#endif //VISUS_PYTHON



///////////////////////////////////////////////////////////////////////////////////
IdxMultipleDataset::IdxMultipleDataset() {

  this->debug_mode = 0;

#if VISUS_PYTHON
  python_engine_pool = std::make_shared<PythonEnginePool>();
#endif //VISUS_PYTHON
}


///////////////////////////////////////////////////////////////////////////////////
IdxMultipleDataset::~IdxMultipleDataset() {
}

///////////////////////////////////////////////////////////////////////////////////
SharedPtr<Access> IdxMultipleDataset::createAccess(StringTree config, bool bForBlockQuery)
{
  VisusAssert(this->valid());

  if (config.empty())
    config = getDefaultAccessConfig();

  //consider I can have thousands of childs (NOTE: this attribute should be "inherited" from child)
  config.writeBool("disable_async", true); 

  String type = StringUtils::toLower(config.readString("type"));

  if (type.empty())
  {
    Url url = config.readString("url",this->getUrl().toString());

    //local disk access
    if (url.isFile())
    {
      if (bMosaic)
        return std::make_shared<IdxMosaicAccess>(this,config);
      else
        return std::make_shared<IdxMultipleAccess>(this, config);
    }
    else
    {
      VisusAssert(url.isRemote());

      if (bForBlockQuery)
        return std::make_shared<ModVisusAccess>(this, config);
      else
        //hopefully I can execute box/point queries on the server
        return SharedPtr<Access>();
    }
  }

  //IdxMosaicAccess
  if (type == "idxmosaicaccess" || (bMosaic && (config.empty() || type.empty())))
  {
    VisusReleaseAssert(bMosaic);
    return std::make_shared<IdxMosaicAccess>(this, config);
  }
    

  //IdxMultipleAccess
  if (type == "idxmultipleaccess" || type == "midx" || type == "multipleaccess")
    return std::make_shared<IdxMultipleAccess>(this, config);

  return IdxDataset::createAccess(config, bForBlockQuery);
}

////////////////////////////////////////////////////////////////////////////////////
Field IdxMultipleDataset::getFieldByNameThrowEx(String FIELDNAME) const
{
  if (bMosaic)
    return IdxDataset::getFieldByNameThrowEx(FIELDNAME);

  auto existing=IdxDataset::getFieldByNameThrowEx(FIELDNAME);

  if (existing.valid())
    return existing;

#if VISUS_PYTHON
  auto output = QueryInputTerm(const_cast<IdxMultipleDataset*>(this), nullptr, SharedPtr<Access>(), Aborted()).computeOutput(FIELDNAME);
  return Field(FIELDNAME, output.dtype);
#else
  return Field(); //invalid
#endif


}

////////////////////////////////////////////////////////////////////////////////////
void IdxMultipleDataset::addChild(IdxMultipleDataset::Child value)
{
  VisusAssert(!childs.count(value.name));
  childs[value.name] = value;
}


////////////////////////////////////////////////////////////////////////////////////
String IdxMultipleDataset::getInputName(String dataset_name, String fieldname)
{
#if VISUS_PYTHON

  std::ostringstream out;
  out << "input";

  if (PythonEngine::isGoodVariableName(dataset_name))
  {
    out << "." << dataset_name;
  }
  else
  {
    out << "['" << dataset_name << "']";
  }

  if (PythonEngine::isGoodVariableName(fieldname))
  {
    out << "." << fieldname;
  }
  else
  {
    if (StringUtils::contains(fieldname, "\n"))
    {
      const String triple = "\"\"\"";
      out << "[" + triple + "\n" + fieldname + triple + "]";
    }
    else
    {
      //fieldname = StringUtils::replaceAll(fieldname, "\"", "\\\"");
      fieldname = StringUtils::replaceAll(fieldname, "'", "\\'");
      out << "['" << fieldname << "']";
    }
  }

  return out.str();
#else
  return FormatString() << "input." << dataset_name << "." << fieldname;
#endif
};

////////////////////////////////////////////////////////////////////////////////////
Field IdxMultipleDataset::createField(String operation_name)
{
  std::ostringstream out;

  std::vector<String> args;
  for (auto it : childs)
  {
    String arg = "f" + cstring((int)args.size());
    args.push_back(arg);
    out << arg << "=" <<getInputName(it.first, it.second.dataset->getDefaultField().name)<< std::endl;
  }
  out << "output=" << operation_name << "([" << StringUtils::join(args,",") << "])" << std::endl;

  String fieldname = out.str();
  Field ret = getFieldByName(fieldname);
  ret.setDescription(operation_name);
  VisusAssert(ret.valid());
  return ret;
};

////////////////////////////////////////////////////////////////////////////////////
String IdxMultipleDataset::removeAliases(String url)
{
  //replace some alias
  auto URL = this->getUrl();

  String cfd = URL.isFile() ? Path(URL.getPath()).getParent().toString() : "";

  if (URL.isFile() && !cfd.empty())
  {
    if (Url(url).isFile() && StringUtils::startsWith(Url(url).getPath(), "./"))
      url = cfd + Url(url).getPath().substr(1);

    if (StringUtils::contains(url, "$(CurrentFileDirectory)"))
      url = StringUtils::replaceAll(url, "$(CurrentFileDirectory)", cfd);
  }
  else if (URL.isRemote())
  {
    if (StringUtils::contains(url, "$(protocol)"))
      url = StringUtils::replaceAll(url, "$(protocol)", URL.getProtocol());

    if (StringUtils::contains(url, "$(hostname)"))
      url = StringUtils::replaceAll(url, "$(hostname)", URL.getHostname());

    if (StringUtils::contains(url, "$(port)"))
      url = StringUtils::replaceAll(url, "$(port)", cstring(URL.getPort()));
  }

  return url;
};


///////////////////////////////////////////////////////////
void IdxMultipleDataset::parseDataset(ObjectStream& istream, Matrix M)
{
  VisusAssert(istream.getCurrentContext()->name == "dataset");
  String url = istream.readInline("url");
  VisusAssert(!url.empty());

  String default_name = StringUtils::format() << "child_" << std::setw(4) << std::setfill('0') << cstring((int)this->childs.size());

  Child child;
  child.name = StringUtils::trim(istream.readInline("name", istream.readInline("id"))); 
  child.color = Color::parseFromString(istream.readInline("color"));
  child.mosaic_filename_template = istream.readInline("filename_template");

  //override name if exist
  if (child.name.empty() || this->childs.find(child.name) != this->childs.end())
    child.name = default_name;

  url= removeAliases(url);

  //if mosaic all datasets are the same, I just need to know the IDX filename template
  if (this->bMosaic && !childs.empty() && !child.mosaic_filename_template.empty())
  {
    auto first = childs.begin()->second.dataset;
    auto other = std::dynamic_pointer_cast<IdxDataset>(first->clone());
    VisusReleaseAssert(first);
    VisusReleaseAssert(other);

    //all the idx files are the same except for the IDX path
    child.mosaic_filename_template =istream.readInline("filename_template");

    VisusReleaseAssert(!child.mosaic_filename_template.empty());
    child.mosaic_filename_template = removeAliases(child.mosaic_filename_template);

    other->setUrl(url);
    other->idxfile.filename_template = child.mosaic_filename_template;
    other->idxfile.validate(url); VisusAssert(other->idxfile.valid());
    child.dataset = other;
  }
  else
  {
    child.dataset = LoadDatasetEx(url,this->getConfig());
  }

  if (!child.dataset) {
    VisusReleaseAssert(false);
    return;
  }

  auto pdim = child.dataset->getPointDim();
  auto sdim = pdim + 1;
  M.setSpaceDim(sdim);
  child.M = M;

  auto s_offset = istream.readInline("offset");
  if (!s_offset.empty())
  {
    auto M = Matrix::translate(PointNd::parseFromString(s_offset));
    M.setSpaceDim(sdim);
    child.M *= M;
  }

  if (istream.pushContext("M"))
  {
    auto value = istream.readInline("value");
    if (!value.empty())
    {
      auto M =Matrix::parseFromString(value);
      M.setSpaceDim(sdim);
      child.M *= M;
    }

    for (auto it : istream.getCurrentContext()->getChilds())
    {
      if (it->name == "translate")
      {
        double x = cdouble(it->readString("x"));
        double y = cdouble(it->readString("y"));
        double z = cdouble(it->readString("z"));
        auto M = Matrix::translate(PointNd(x,y,z));
        M.setSpaceDim(sdim);
        child.M *= M;
      }
      else if (it->name == "rotate")
      {
        double x = Utils::degreeToRadiant(cdouble(it->readString("x")));
        double y = Utils::degreeToRadiant(cdouble(it->readString("y")));
        double z = Utils::degreeToRadiant(cdouble(it->readString("z")));
        auto M = Matrix::rotate(Quaternion::fromEulerAngles(x, y, z));
        M.setSpaceDim(sdim);
        child.M *= M;
      }
      else if (it->name == "scale")
      {
        double x = cdouble(it->readString("x"));
        double y = cdouble(it->readString("y"));
        double z = cdouble(it->readString("z"));
        auto M = Matrix::nonZeroScale(PointNd(x, y, z));
        M.setSpaceDim(sdim);
        child.M *= M;
      }

      else if (it->name == "M")
      {
        auto M = Matrix::parseFromString(it->readString("value"));
        M.setSpaceDim(sdim);
        child.M *= M;
      }
    }

    istream.popContext("M");
  }

  //if mosaic then only an offset
  if (bMosaic)
    VisusAssert(child.M.submatrix(child.M.getSpaceDim()-1,child.M.getSpaceDim()-1).isIdentity());

  //VisusInfo() << "midx single M("<<child.M.toString()<<") box("<<child.dataset->box.toString(false)<<")";
  //VisusInfo() << " bounds("<<Position(child.M,child.dataset->box).toAxisAlignedBox().toString(false)<<")";

  addChild(child);

}

///////////////////////////////////////////////////////////
void IdxMultipleDataset::parseDatasets(ObjectStream& istream, Matrix T)
{
  auto context = istream.getCurrentContext();
  

  for (int I = 0; I < (int)context->getNumberOfChilds(); I++)
  {
    auto name = context->getChild(I).name;
    
    if (name == "dataset")
    {
      istream.pushContext("dataset");

      if (cbool(istream.readInline("enabled", "1"))) {
        parseDataset(istream, T);
      }

      istream.popContext("dataset");
      continue;
    }

    if (name == "translate")
    {
      istream.pushContext("translate");

      if (cbool(istream.readInline("enabled", "1")))
      {
        double x = cdouble(istream.readInline("x"));
        double y = cdouble(istream.readInline("y"));
        double z = cdouble(istream.readInline("z"));
        parseDatasets(istream, T * Matrix::translate(Point3d(x, y, z)));
      }
      istream.popContext("translate");
      continue;
    }

    if (name == "rotate")
    {
      istream.pushContext("rotate");

      if (cbool(istream.readInline("enabled", "1")))
      {
        double x = Utils::degreeToRadiant(cdouble(istream.readInline("x")));
        double y = Utils::degreeToRadiant(cdouble(istream.readInline("y")));
        double z = Utils::degreeToRadiant(cdouble(istream.readInline("z")));
        parseDatasets(istream, T * Matrix::rotate(Quaternion::fromEulerAngles(x, y, z)));
      }
      istream.popContext("rotate");
      continue;
    }

    if (name == "scale")
    {
      istream.pushContext("scale");

      if (cbool(istream.readInline("enabled", "1")))
      {
        double x = cdouble(istream.readInline("x"));
        double y = cdouble(istream.readInline("y"));
        double z = cdouble(istream.readInline("z"));
        parseDatasets(istream, T * Matrix::nonZeroScale(PointNd(x, y, z)));
      }

      istream.popContext("scale");
      continue;
    }

    //pass throught
    if (name != "field")
    {
      istream.pushContext(name);
      if (cbool(istream.readInline("enabled", "1")))
        parseDatasets(istream, T);

      istream.popContext(name);
    }
  }
}

///////////////////////////////////////////////////////////
void IdxMultipleDataset::computeDefaultFields()
{
  clearFields();

  addField(createField("ArrayUtils.average"));
  addField(createField("ArrayUtils.add"));
  addField(createField("ArrayUtils.sub"));
  addField(createField("ArrayUtils.mul"));
  addField(createField("ArrayUtils.div"));
  addField(createField("ArrayUtils.min"));
  addField(createField("ArrayUtils.max"));
  addField(createField("ArrayUtils.standardDeviation"));
  addField(createField("ArrayUtils.median"));

  //note: this wont' work on old servers
  addField(Field("output=voronoi()"));
  addField(Field("output=noBlend()"));
  addField(Field("output=averageBlend()"));

  for (auto it : childs)
  {
    for (auto field : it.second.dataset->getFields())
    {
      auto arg = getInputName(it.first, field.name);
      Field FIELD = getFieldByName("output=" + arg  + ";");
      VisusAssert(FIELD.valid());
      FIELD.setDescription(it.first + "/" + field.getDescription());
      addField(FIELD);
    }
  }
}

///////////////////////////////////////////////////////////
bool IdxMultipleDataset::openFromUrl(Url URL)
{
  StringTree BODY;
  if (!BODY.fromXmlString(Utils::loadTextDocument(URL.toString())))
    return false;

  BODY.writeString("url", URL.toString());

  setUrl(URL);
  setDatasetBody(BODY.toString());

  ObjectStream istream(BODY, 'r');

  this->bMosaic = cbool(istream.readInline("mosaic"));

  parseDatasets(istream, Matrix::identity(4));

  if (childs.empty())
  {
    VisusAssert(false);
    this->invalidate();
    return false;
  }

  auto first = childs.begin()->second.dataset;
  int pdim = first->getPointDim();

  IdxFile& IDXFILE=this->idxfile;

  //the user specified the box?
  auto logic_box = istream.readInline("box");
  if (!logic_box.empty())
  {
    IDXFILE.logic_box = BoxNi::parseFromOldFormatString(pdim, logic_box);
  }
  else
  {
    if (bMosaic)
    {
      IDXFILE.logic_box = BoxNi::invalid();
      for (auto it : childs)
      {
        auto M = it.second.M;
        auto dataset = it.second.dataset;
        auto logic_box = Position(M, dataset->getLogicBox()).toAxisAlignedBox().castTo<BoxNi>();
        IDXFILE.logic_box = IDXFILE.logic_box.getUnion(logic_box);
      }
    }
    //try not to loose pixels
    else
    {
      //union of boxes
      auto PHYSICAL_BOX = BoxNd::invalid();
      for (auto it : childs)
      {
        auto M = it.second.M;
        auto dataset = it.second.dataset;
        auto physical_position = Position(M, dataset->getPhysicPosition()).toAxisAlignedBox();
        PHYSICAL_BOX = PHYSICAL_BOX.getUnion(physical_position);
      }

      std::vector<double> DENSITY;
      for (auto it : childs)
      {
        auto M = it.second.M;
        auto dataset = it.second.dataset;
        auto pixels = dataset->getLogicBox().size().innerProduct();
        auto volume = Position(M, dataset->getPhysicPosition()).computeVolume();
        auto density = pixels / volume;
        DENSITY.push_back(density);
      }

      // try to keep the same number of pixels
      // TOT_PIXELS[max_density] = PHYSICAL_VOLUMES[max_density] * pow(scale,pdim)
      // DENSITY[max_density] = pow(scale,pdim)
      auto max_density = std::distance(DENSITY.begin(), std::max_element(DENSITY.begin(), DENSITY.end()));
      auto scale = pow(DENSITY[max_density], 1.0 / pdim);
      VisusInfo() << "Scale to not loose pixels " << scale << " #max_density("<<max_density<<")";

      auto not_loose_pixels = Matrix::scale(pdim, scale) * Matrix::translate(-PHYSICAL_BOX.p1);

      //todo, right now physic bounds is == logic box
#if 1

      auto LOGIC_BOX = BoxNi::invalid();
      for (auto it : childs)
      {
        auto M = it.second.M;
        auto dataset = it.second.dataset;
        M = not_loose_pixels * M;
        it.second.M = M;
        auto logic_box = Position(M, dataset->getPhysicPosition()).toAxisAlignedBox().castTo<BoxNi>();
        LOGIC_BOX = LOGIC_BOX.getUnion(logic_box);
      }

      IDXFILE.logic_box = LOGIC_BOX;
      IDXFILE.logic_to_physic = Matrix::identity(pdim+1);

#else

      //union of boxes
      auto LOGIC_BOX = BoxNi::invalid();
      for (auto it : childs)
      {
        auto M = it.second.M;
        auto dataset = it.second.dataset;
        auto logic_box = Position(not_loose_pixels, M,  dataset->getPhysicPosition()).toAxisAlignedBox().castTo<BoxNi>();
        LOGIC_BOX = LOGIC_BOX.getUnion(logic_box);
      }

      IDXFILE.logic_box = LOGIC_BOX;
      IDXFILE.logic_to_physic = Position::computeTransformation(Position(PHYSICAL_BOX),LOGIC_BOX);
#endif
    }
  }

  //VisusInfo() << "midx box is " << IDXFILE.box.toString(false);
  if (bMosaic)
  {
    //i need the final right part to be as the child
    auto BITMASK = DatasetBitmask::guess(IDXFILE.logic_box.p2);
    auto bitmask = first->getBitmask();

    PointNi DIMS = BITMASK.getPow2Dims();
    PointNi dims = first->getBitmask().getPow2Dims();
    
    auto left  = DatasetBitmask::guess(DIMS.innerDiv(dims)).toString();
    auto right = bitmask.toString().substr(1);
    BITMASK = DatasetBitmask(left + right);
    VisusReleaseAssert(BITMASK.getPow2Dims()==DIMS);
    VisusReleaseAssert(StringUtils::endsWith(BITMASK.toString(), right));

    IDXFILE.bitmask = BITMASK;
    IDXFILE.timesteps = first->getTimesteps();
    IDXFILE.fields = first->getFields();
    IDXFILE.bitsperblock = first->getDefaultBitsPerBlock();

    IDXFILE.validate(this->getUrl());
    VisusReleaseAssert(IDXFILE.valid());

    //VisusInfo() << "MIDX idxfile is the following" << std::endl << IDXFILE.toString();
    setIdxFile(IDXFILE);

    return true;
  }
  
  if (childs.size() == 1)
  {
    if (auto dataset=std::dynamic_pointer_cast<IdxDataset>(first))
      IDXFILE.time_template = dataset->idxfile.time_template;
  }
 
  //union of all timesteps
  for (auto it : childs)
    IDXFILE.timesteps.addTimesteps(it.second.dataset->getTimesteps());

  //this is to pass the validation, an midx has infinite run-time fields 
  IDXFILE.fields.push_back(Field("DATA", DTypes::UINT8));

  IDXFILE.validate(URL);
  VisusReleaseAssert(IDXFILE.valid());

  //VisusInfo() << "MIDX idxfile is the following" << std::endl << IDXFILE.toString();
  setIdxFile(IDXFILE);

  //for non-mosaic I cannot use block query
  //if (pdim==2)
  //  this->kdquery_mode = KdQueryMode::UseQuery;

  if (istream.getCurrentContext()->findChildWithName("field"))
  {
    clearFields();

    int generate_name = 0;

    while (istream.pushContext("field"))
    {
      String name = istream.readInline("name"); 
      if (name.empty())
        name = StringUtils::format() << "field_" + generate_name++;

      //I expect to find here CData node or Text node...
      istream.pushContext("code");
      String code = istream.readText(); VisusAssert(!code.empty());
      istream.popContext("code");

      Field FIELD = getFieldByName(code); 
      if (FIELD.valid())
      {
        ParseStringParams parse(name);
        FIELD.params = parse.params; //important for example in case I want to override the time
        FIELD.setDescription(parse.without_params);
        addField(FIELD);
      }

      istream.popContext("field");
    }
  }
  else
  {
    computeDefaultFields();
  }

  return true;
}

////////////////////////////////////////////////////////////////////////
std::map<String, SharedPtr<Dataset>> IdxMultipleDataset::getInnerDatasets() const
{
  std::map<String, SharedPtr<Dataset> > ret;
  for (auto it : childs)
    ret[it.first] = it.second.dataset;
  return ret;
}

////////////////////////////////////////////////////////////////////////
bool IdxMultipleDataset::executeQuery(SharedPtr<Access> access,SharedPtr<Query> QUERY)
{
  //execute N-Query (independentely) and blend them
  if (!bMosaic)
  {
    if (auto multiple_access = std::dynamic_pointer_cast<IdxMultipleAccess>(access))
    {
      if (!this->Dataset::executeQuery(access, QUERY))
        return false;

      String error_msg;
      Array  OUTPUT;

#if VISUS_PYTHON
      try
      {
        OUTPUT = QueryInputTerm(this, QUERY.get(), multiple_access, QUERY->aborted).computeOutput(QUERY->field.name);
      }
      catch (std::exception ex)
      {
        error_msg = ex.what();
      }
#else
      error_msg = "Python disabled";
#endif

      if (QUERY->aborted())
        OUTPUT = Array();

      if (!OUTPUT || !OUTPUT.getTotalNumberOfSamples())
      {
        if (QUERY->aborted())
        {
          QUERY->setFailed("query aborted");
          return false;
        }
        else
        {
          //VisusInfo() << "Failed to execute box query: " << error_msg;
          QUERY->setFailed(error_msg);
          return false;
        }
      }

      //can happen that I change the output dimensions from scripting
      //NOTE: it will be unmergeable!
      if (OUTPUT.dims != QUERY->nsamples)
      {
        QUERY->nsamples = OUTPUT.dims;
        QUERY->logic_box = LogicBox();
      }

      QUERY->buffer = OUTPUT;
      QUERY->currentLevelReady();
      return true;
    }
  }

  // example: since I can handle blocks, I can even enable caching adding for example a RamAccess/DiskAcces to the top
  if (access)
    return IdxDataset::executeQuery(access, QUERY);

  // blending happens on the server
  else
    return IdxDataset::executePureRemoteQuery(QUERY);
}

////////////////////////////////////////////////////////////////////////
bool IdxMultipleDataset::nextQuery(SharedPtr<Query> QUERY)
{
  if (!IdxDataset::nextQuery(QUERY))
    return false;

  for (auto it : QUERY->down_queries)
  {
    auto  query   = it.second;
    auto  vf      = this->childs[query->down_info.name].dataset; VisusAssert(vf);

    if (query && query->canNext())
      vf->nextQuery(query);
  }

  return true;
}

////////////////////////////////////////////////////////////////////////
bool IdxMultipleDataset::createIdxFile(String idx_filename, Field idx_field) const
{
  auto idxfile = this->idxfile;

  idxfile.filename_template = "";
  idxfile.time_template = "";
  idxfile.fields.clear();
  idxfile.fields.push_back(idx_field);
  idxfile.validate(Url(idx_filename));

  if (!idxfile.valid())
    return false;

  if (!idxfile.save(idx_filename))
    return false;

  return true;
}

} //namespace Visus
