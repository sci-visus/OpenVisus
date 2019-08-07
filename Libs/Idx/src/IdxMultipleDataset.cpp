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

  IdxMultipleDataset* DATASET = nullptr;
  StringTree                                       CONFIG;
  std::map< std::pair<String, String>, StringTree> configs;
  SharedPtr<ThreadPool>                            thread_pool;

  //constructor
  IdxMultipleAccess(IdxMultipleDataset* VF_, StringTree CONFIG_)
  : DATASET(VF_), CONFIG(CONFIG_)
  {
    VisusAssert(!DATASET->bMosaic);

    this->name = CONFIG.readString("name", "IdxMultipleAccess");
    this->can_read = true;
    this->can_write = false;
    this->bitsperblock = DATASET->getDefaultBitsPerBlock();

    for (auto child : DATASET->down_datasets)
    {
      auto name = child.first;

      //see if the user specified how to access the data for each query dataset
      for (auto it : CONFIG.getChilds())
      {
        if (it->name == name || it->readString("name") == name)
          configs[std::make_pair(name, "")] = *it;
      }
    }

    bool disable_async = CONFIG.readBool("disable_async", DATASET->isServerMode());

    //TODO: special case when I can use the blocks
    //if (DATASET->childs.size() == 1 && DATASET->sameLogicSpace(DATASET->childs[0]))
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
    auto dataset = DATASET->getChild(name);
    VisusAssert(dataset);

    SharedPtr<Access> ret;

    StringTree config = dataset->getDefaultAccessConfig();

    auto it = this->configs.find(std::make_pair(name, fieldname));
    if (it==configs.end())
      it = configs.find(std::make_pair(name, ""));

    if (it != configs.end())
      config = it->second;

    config.inheritAttributeFrom(this->CONFIG);
    bool bForBlockQuery = DATASET->getKdQueryMode() & KdQueryMode::UseBlockQuery ? true : false;
    return dataset->createAccess(config, bForBlockQuery);
  }

  //readBlock 
  virtual void readBlock(SharedPtr<BlockQuery> BLOCKQUERY) override
  {
    ThreadPool::push(thread_pool, [this, BLOCKQUERY]() 
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
      auto QUERY = DATASET->createEquivalentBoxQuery('r', BLOCKQUERY);

      if (!DATASET->nextQuery(QUERY))
        return readFailed(BLOCKQUERY);

      if (!DATASET->executeQuery(shared_from_this(), QUERY))
        return readFailed(BLOCKQUERY);

      BLOCKQUERY->buffer = QUERY->buffer;
      return readOk(BLOCKQUERY);
    });
  }

  //writeBlock (not supported)
  virtual void writeBlock(SharedPtr<BlockQuery> BLOCKQUERY) override {
    //not supported
    VisusAssert(false);
    writeFailed(BLOCKQUERY);
  }

}; //end class


//////////////////////////////////////////////////////
class IdxMosaicAccess : public Access
{
public:

  VISUS_NON_COPYABLE_CLASS(IdxMosaicAccess)

  //_____________________________________________________
  class Child
  {
  public:
    SharedPtr<IdxDataset> dataset;
    SharedPtr<Access>     access;

    struct Compare
    {
      bool operator()(const PointNi& a, const  PointNi& b) const {
        return a.toVector() < b.toVector();
      }
    };

  };

  IdxMultipleDataset* DATASET;
  StringTree CONFIG;
  std::map<PointNi, Child, Child::Compare > childs;


  //constructor
  IdxMosaicAccess(IdxMultipleDataset* VF_, StringTree CONFIG = StringTree())
    : DATASET(VF_)
  {
    VisusReleaseAssert(DATASET->bMosaic);

    if (!DATASET->valid())
      ThrowException("IdxDataset not valid");

    this->name = CONFIG.readString("name", "IdxMosaicAccess");
    this->CONFIG = CONFIG;
    this->can_read = StringUtils::find(CONFIG.readString("chmod", "rw"), "r") >= 0;
    this->can_write = StringUtils::find(CONFIG.readString("chmod", "rw"), "w") >= 0;
    this->bitsperblock = DATASET->getDefaultBitsPerBlock();

    auto first = DATASET->getFirstChild();
    auto dims = first->getLogicBox().p2;
    int  pdim = first->getPointDim();
    int  sdim = pdim + 1;

    for (auto it : DATASET->down_datasets)
    {
      auto dataset = std::dynamic_pointer_cast<IdxDataset>(it.second); VisusAssert(dataset);
      VisusAssert(false);// need to check if this is correct
      VisusAssert(dataset->logic_to_LOGIC.submatrix(sdim-1,sdim-1).isIdentity());
      auto offset = dataset->logic_to_LOGIC.getCol(pdim).castTo<PointNi>();
      auto index = offset.innerDiv(dims);
      VisusAssert(!this->childs.count(index));
      this->childs[index].dataset = dataset;
    }
  }

  //destructor
  virtual ~IdxMosaicAccess() {
  }

  //getChildAccess
  SharedPtr<Access> getChildAccess(const Child& child) const
  {
    if (child.access)
      return child.access;

    //with thousansands of childs I don't want to create ThreadPool or NetService
    auto config = StringTree();
    config.writeBool("disable_async", true);
    auto ret = child.dataset->createAccess(config,/*bForBlockQuery*/true);
    const_cast<Child&>(child).access = ret;
    return ret;
  }

  //beginIO
  virtual void beginIO(String mode) override {
    Access::beginIO(mode);
  }

  //endIO
  virtual void endIO() override
  {
    for (const auto& it : childs)
    {
      auto access = it.second.access;
      if (access && (access->isReading() || access->isWriting()))
        access->endIO();
    }

    Access::endIO();
  }

  //readBlock
  virtual void readBlock(SharedPtr<BlockQuery> QUERY) override
  {
    VisusAssert(isReading());

    auto pdim = DATASET->getPointDim();
    auto BLOCK = QUERY->start_address >> bitsperblock;
    auto first = childs.begin()->second.dataset;
    auto NBITS = DATASET->getMaxResolution() - first->getMaxResolution();
    PointNi dims = first->getLogicBox().p2;

    bool bBlockTotallyInsideSingle = (BLOCK >= ((BigInt)1 << NBITS));

    if (bBlockTotallyInsideSingle)
    {
      //forward the block read to a single child
      auto P1    = QUERY->getLogicBox().p1;
      auto index = P1.innerDiv(dims);
      auto p1    = P1.innerMod(dims);

      auto it = childs.find(index);
      if (it == childs.end())
        return readFailed(QUERY);

      auto dataset = it->second.dataset;
      VisusAssert(dataset);

      auto hzfrom = HzOrder(dataset->idxfile.bitmask).getAddress(p1);

      auto block_query = std::make_shared<BlockQuery>(dataset.get(), QUERY->field, QUERY->time, hzfrom, hzfrom + ((BigInt)1 << bitsperblock), 'r', QUERY->aborted);

      auto access = getChildAccess(it->second);

      if (!access->isReading())
        access->beginIO(this->getMode());

      //TODO: should I keep track of running queries in order to wait for them in the destructor?
      dataset->executeBlockQuery(access, block_query).when_ready([this, QUERY, block_query](Void) {

        if (block_query->failed())
          return readFailed(QUERY); //failed

        QUERY->buffer = block_query->buffer;
        return readOk(QUERY);
        });
    }
    else
    {
      //THIS IS GOING TO BE SLOW: i need to compose coarse blocks by executing "normal" query and merging them
      auto t1 = Time::now();

      VisusInfo() << "IdxMosaicAccess is composing block " << BLOCK << " (slow)";

      //row major
      QUERY->buffer.layout = "";

      DatasetBitmask BITMASK = DATASET->idxfile.bitmask;
      HzOrder HZORDER(BITMASK);

      for (const auto& it : childs)
      {
        auto dataset = it.second.dataset;
        auto offset = it.first.innerMultiply(dims);
        auto access = getChildAccess(it.second);

        auto query = std::make_shared<BoxQuery>(dataset.get(), QUERY->field, QUERY->time, 'r');
        query->logic_box = QUERY->getLogicBox().translate(-offset);
        query->setResolutionRange(BLOCK ? query->end_resolutions[0] : 0, HZORDER.getAddressResolution(BITMASK, QUERY->end_address - 1) - NBITS );

        if (access->isReading() || access->isWriting())
          access->endIO();

        if (!dataset->nextQuery(query))
          continue;

        if (!query->allocateBufferIfNeeded())
          continue;

        if (!dataset->executeQuery(access, query))
          continue;

        auto pixel_p1 =      PointNi(pdim); auto logic_p1 = query->logic_samples.pixelToLogic(pixel_p1); auto LOGIC_P1 = logic_p1 + offset; auto PIXEL_P1 = QUERY->logic_samples.logicToPixel(LOGIC_P1);
        auto pixel_p2 = query->buffer.dims; auto logic_p2 = query->logic_samples.pixelToLogic(pixel_p2); auto LOGIC_P2 = logic_p2 + offset; auto PIXEL_p2 = QUERY->logic_samples.logicToPixel(LOGIC_P2);

        ArrayUtils::insert(
          QUERY->buffer, PIXEL_P1, PIXEL_p2, PointNi::one(pdim),
          query->buffer, pixel_p1, pixel_p2, PointNi::one(pdim),
          QUERY->aborted);
      }

      if (bool bPrintStats = false)
      {
        auto stats = ApplicationStats::io.readValues(true);
        VisusInfo() << "!!! BLOCK " << BLOCK << " inside " << (bBlockTotallyInsideSingle ? "yes" : "no")
          << " nopen(" << stats.nopen << ") rbytes(" << StringUtils::getStringFromByteSize(stats.rbytes) << ")  wbytes(" << StringUtils::getStringFromByteSize(stats.wbytes) << ")"
          << " msec(" << t1.elapsedMsec() << ")";
      }

      return QUERY->aborted() ? readFailed(QUERY) : readOk(QUERY);
    }
  }

  //writeBlock
  virtual void writeBlock(SharedPtr<BlockQuery> QUERY) override
  {
    //not supported!
    VisusAssert(isWriting());
    VisusAssert(false);
    return writeFailed(QUERY);
  }

};


////////////////////////////////////////////////////////
#if VISUS_PYTHON
class QueryInputTerm
{
public:

  VISUS_NON_COPYABLE_CLASS(QueryInputTerm)

  IdxMultipleDataset*      DATASET;
  BoxQuery*                QUERY;
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
  QueryInputTerm(IdxMultipleDataset* VF_, BoxQuery* QUERY_, SharedPtr<Access> ACCESS_, Aborted aborted_)
    : DATASET(VF_), QUERY(QUERY_), ACCESS(ACCESS_), aborted(aborted_) {

    VisusAssert(!DATASET->bMosaic);

    this->engine = (!DATASET->isServerMode())? DATASET->python_engine_pool->createEngine() : std::make_shared<PythonEngine>();

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
      engine->setModuleAttr("query_time", QUERY ? QUERY->time : DATASET->getTimesteps().getDefault());

      engine->addModuleFunction("doPublish", [this](PyObject *self, PyObject *args) {
        return doIncrementalPublish(self, args);
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

    if (!DATASET->isServerMode())
      DATASET->python_engine_pool->releaseEngine(this->engine);
  }

  //computeOutput
  Array computeOutput(String code)
  {
    ScopedAcquireGil acquire_gil;
    engine->execCode(code);

    auto ret = engine->getModuleArrayAttr("output");
    if (!ret && !aborted())
      ThrowException("script does not assign 'output' value");

    if (DATASET->debug_mode & IdxMultipleDataset::DebugSaveImages)
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
      return engine->newPyObject(DATASET->getTimesteps().asVector());

    auto dataset = DATASET->getChild(expr1);
    if (!dataset)
      ThrowException(StringUtils::format() << "input['" << expr1 << "'] not found");

    auto ret = newDynamicObject([this, expr1](String expr2) {
      return getAttr2(expr1, expr2);
    });
    return ret;
  }

  //getAttr2
  PyObject* getAttr2(String expr1, String expr2)
  {
    auto dataset = DATASET->getChild(expr1);
    VisusAssert(dataset);

    //example: input.datasetname.timesteps
    if (expr2 == "timesteps")
      return engine->newPyObject(dataset->getTimesteps().asVector());

    //see https://github.com/sci-visus/visus-issues/issues/367 
    //specify a dataset  (see midxofmidx.midx)
    //EXAMPLE: output = input.first   ['output=input.A.temperature'];
    //EXAMPLE: output = input.first.                 A.temperature 
    if (auto midx = std::dynamic_pointer_cast<IdxMultipleDataset>(dataset))
    {
      if (midx->getChild(expr2))
      {
        auto ret = newDynamicObject([this, expr1, expr2](String expr3) {
          return getAttr2(expr1, StringUtils::join({ expr2 }, ".", "output=input.", "." + expr3 + ";"));
        });
        return ret;
      }
    }

    Array ret;

    //execute a query (expr2 is the fieldname)
    Field field = dataset->getFieldByName(expr2);

    if (!field.valid())
      ThrowException(StringUtils::format() << "input['" << expr1 << "']['" << expr2 << "'] not found");

    int pdim = DATASET->getPointDim();

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
  SharedPtr<BoxQuery> createDownQuery(String name, String fieldname)
  {
    auto key = name +"/"+fieldname;

    //already created?
    auto it = QUERY->down_queries.find(key);
    if (it != QUERY->down_queries.end())
      return it->second;

    auto dataset = DATASET->down_datasets[name]; VisusAssert(dataset);
    auto field = dataset->getFieldByName(fieldname); VisusAssert(field.valid());
    auto logic_to_LOGIC = dataset->logic_to_LOGIC;

    auto LOGIC_MAPPED_REGION = Position(logic_to_LOGIC, dataset->getLogicBox()).toAxisAlignedBox();

    //no intersection? just skip this down query
    if (!QUERY->logic_box.castTo<BoxNd>().intersect(LOGIC_MAPPED_REGION))
      return SharedPtr<BoxQuery>();

    auto query = std::make_shared<BoxQuery>(dataset.get(), field, QUERY->time, 'r', QUERY->aborted);
    QUERY->down_queries[key] = query;
    query->down_info.name = name;

    auto QUERY_BOX = QUERY->logic_box.castTo<BoxNd>();

    // consider that logic_to_logic could have mat(3,0) | mat(3,1) | mat(3,2) !=0 and so I can have non-parallel axis
    // using directly the QUERY_BOX could result in missing pieces for example in voronoi
    // solution is to limit the QUERY_BOX to the valid mapped region
    QUERY_BOX = QUERY_BOX.getIntersection(LOGIC_MAPPED_REGION);

    query->logic_box = Position(logic_to_LOGIC.invert(), QUERY_BOX).toDiscreteAxisAlignedBox();

    //euristic to find delta in the hzcurve
    //TODO!!!! this euristic produces too many samples
    auto VOLUME = Position(                DATASET->getLogicBox()).computeVolume();
    auto volume = Position(logic_to_LOGIC, dataset->getLogicBox()).computeVolume();
    int delta_h = -(int)log2(VOLUME / volume);

    //resolutions
    if (!QUERY->start_resolution)
      query->start_resolution = 0;
    else
      query->start_resolution = Utils::clamp(QUERY->start_resolution + delta_h, 0, dataset->getMaxResolution()); //probably a block query

    std::set<int> resolutions;
    for (auto END_RESOLUTION : QUERY->end_resolutions)
    {
      auto end_resolution = Utils::clamp(END_RESOLUTION + delta_h, 0, dataset->getMaxResolution());
      resolutions.insert(end_resolution);
    }
    query->end_resolutions = std::vector<int>(resolutions.begin(), resolutions.end());

    //skip this argument since returns empty array
    if (!dataset->nextQuery(query))
    {
      query->setFailed("cannot begin the query");
      return query;
    }

    //ignore missing timesteps
    if (!dataset->getTimesteps().containsTimestep(query->time))
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
  Array executeDownQuery(SharedPtr<BoxQuery> query)
  {
    //VisusInfo() << Thread::getThreadId();

    //already failed
    if (!query || query->failed())
      return Array();

    auto name = query->down_info.name;

    auto dataset = DATASET->down_datasets[name]; VisusAssert(dataset);

    //NOTE if I cannot execute it probably reached max resolution for query, in that case I recycle old 'BUFFER'
    if (query->canExecute())
    {
      if (DATASET->debug_mode & IdxMultipleDataset::DebugSkipReading)
      {
        query->allocateBufferIfNeeded();
        ArrayUtils::setBufferColor(query->buffer, DATASET->down_datasets[name]->color);
        query->buffer.layout = ""; //row major
        VisusAssert(query->buffer.dims == query->getNumberOfSamples());
        query->setCurrentResolution(query->end_resolution);

      }
      else
      {
        if (!dataset->executeQuery(query->down_info.access, query))
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
    auto NSAMPLES = QUERY->getNumberOfSamples();
    if (query->down_info.BUFFER && query->down_info.BUFFER.dims == NSAMPLES)
      return query->down_info.BUFFER;

    //create a brand new BUFFER for doing the warpPerspective
    query->down_info.BUFFER = Array(NSAMPLES, query->buffer.dtype);
    query->down_info.BUFFER.fillWithValue(query->field.default_value);

    query->down_info.BUFFER.alpha = std::make_shared<Array>(NSAMPLES, DTypes::UINT8);
    query->down_info.BUFFER.alpha->fillWithValue(0);

    auto PIXEL_TO_LOGIC = Position::computeTransformation(Position(QUERY->logic_box), query->down_info.BUFFER.dims);
    auto pixel_to_logic = Position::computeTransformation(Position(query->logic_box), query->buffer.dims);

    // Tperspective := PIXEL <- pixel
    auto logic_to_LOGIC = dataset->logic_to_LOGIC;
    auto LOGIC_TO_PIXEL = PIXEL_TO_LOGIC.invert();
    Matrix Tperspective = LOGIC_TO_PIXEL * logic_to_LOGIC * pixel_to_logic;

    //this will help to find voronoi seams betweeen images
    query->down_info.LOGIC_TO_PIXEL = LOGIC_TO_PIXEL;
    query->down_info.PIXEL_TO_LOGIC = PIXEL_TO_LOGIC;
    query->down_info.LOGIC_CENTROID = logic_to_LOGIC * dataset->getLogicBox().center();

    //limit the samples to good logic domain
    //explanation: for each pixel in dims, tranform it to the logic dataset box, if inside set the pixel to 1 otherwise set the pixel to 0
    if (!query->buffer.alpha)
      query->buffer.alpha = std::make_shared<Array>(ArrayUtils::createTransformedAlpha(dataset->getLogicBox(), pixel_to_logic, query->buffer.dims, QUERY->aborted));
    else
      VisusReleaseAssert(query->buffer.alpha->dims==query->buffer.dims);

    if (!QUERY->aborted())
    {
      ArrayUtils::warpPerspective(query->down_info.BUFFER, Tperspective, query->buffer, QUERY->aborted);

      if (DATASET->debug_mode & IdxMultipleDataset::DebugSaveImages)
      {
        static int cont = 0;
        ArrayUtils::saveImage(StringUtils::format() << "temp/" << cont << ".dw." + name << "." << query->field.name << ".buffer.png", query->buffer);
        ArrayUtils::saveImage(StringUtils::format() << "temp/" << cont << ".dw." + name << "." << query->field.name << ".alpha_.png", *query->buffer.alpha );
        ArrayUtils::saveImage(StringUtils::format() << "temp/" << cont << ".up." + name << "." << query->field.name << ".buffer.png", query->down_info.BUFFER);
        ArrayUtils::saveImage(StringUtils::format() << "temp/" << cont << ".up." + name << "." << query->field.name << ".alpha_.png", *query->down_info.BUFFER.alpha);
        //ArrayUtils::setBufferColor(query->BUFFER, DATASET->childs[name].color);
        cont++;
      }
    }

    return query->down_info.BUFFER;
  }

  //blendBuffers
  PyObject* blendBuffers(PyObject *self, PyObject *args, BlendBuffers::Type type)
  {
    int argc = args ? (int)PyObject_Length(args) : 0;

    int pdim = DATASET->getPointDim();

    BlendBuffers blend(type, aborted);

    //preview only
    if (!QUERY)
    {
      if (!argc)
      {
        for (auto it : DATASET->down_datasets)
        {
          auto arg = Array(PointNi(pdim), it.second->getDefaultField().dtype);
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

      std::vector< SharedPtr<BoxQuery> > queries;
      queries.reserve(DATASET->down_datasets.size());
      for (auto it : DATASET->down_datasets)
      {
        auto name      = it.first;
        auto fieldname = it.second->getDefaultField().name;
        auto query=createDownQuery(name,fieldname);
        if (query && !query->failed())
          queries.push_back(query);
      }

      //I don't see any advantage using OpenMP here
      //bool bRunInParallel = false;// DATASET->bServerMode ? false : true;
      //#pragma omp parallel for if(bRunInParallel) 
      for (int I = 0; I<(int)queries.size(); I++)
      {
        auto query = queries[I];
        executeDownQuery(query);

        if (query->down_info.BUFFER && !query->aborted()) 
        {
          ScopedLock scopedlock(blendlock);
          blend.addBlendArg(query->down_info.BUFFER, query->down_info.PIXEL_TO_LOGIC, query->down_info.LOGIC_CENTROID);
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
          blend.addBlendArg(query->down_info.BUFFER, query->down_info.PIXEL_TO_LOGIC, query->down_info.LOGIC_CENTROID);
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

  //doIncrementalPublish
  PyObject* doIncrementalPublish(PyObject *self, PyObject *args)
  {
    auto output = engine->getModuleArrayAttr("output");

    if (!output || !QUERY || !QUERY->incrementalPublish)
      return nullptr;

    //postpost a little?
    auto current_time = Time::now().getUTCMilliseconds();
    if (incremental.last_publish_time > 0)
    {
      auto enlapsed_msec = current_time - incremental.last_publish_time;
      if (enlapsed_msec < incremental.max_publish_msec)
        return nullptr;
    }

    QUERY->incrementalPublish(output);
    return nullptr;
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
  for (auto it : down_datasets)
  {
    String arg = "f" + cstring((int)args.size());
    args.push_back(arg);
    out << arg << "=" <<getInputName(it.first, it.second->getDefaultField().name)<< std::endl;
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
void IdxMultipleDataset::parseDataset(ObjectStream& istream, Matrix modelview)
{
  VisusAssert(istream.getCurrentContext()->name == "dataset");
  String url = istream.readInline("url");
  VisusAssert(!url.empty());

  String default_name = StringUtils::format() << "child_" << std::setw(4) << std::setfill('0') << cstring((int)this->down_datasets.size());

  String name = StringUtils::trim(istream.readInline("name", istream.readInline("id"))); 
  
  //override name if exist
  if (name.empty() || this->down_datasets.find(name) != this->down_datasets.end())
    name = default_name;

  url= removeAliases(url);

  //if mosaic all datasets are the same, I just need to know the IDX filename template
  SharedPtr<Dataset> child;
  if (this->bMosaic && !down_datasets.empty() && istream.hasAttribute("filename_template"))
  {
    auto first = getFirstChild();
    auto other = std::dynamic_pointer_cast<IdxDataset>(first->clone());
    VisusReleaseAssert(first);
    VisusReleaseAssert(other);

    //all the idx files are the same except for the IDX path
    String mosaic_filename_template =istream.readInline("filename_template");

    VisusReleaseAssert(!mosaic_filename_template.empty());
    mosaic_filename_template = removeAliases(mosaic_filename_template);

    other->setUrl(url);
    other->idxfile.filename_template = mosaic_filename_template;
    other->idxfile.validate(url); VisusAssert(other->idxfile.valid());
    child = other;
  }
  else
  {
    child = LoadDatasetEx(url,this->getConfig());
  }

  if (!child) {
    VisusReleaseAssert(false);
    return;
  }

  child->color = Color::parseFromString(istream.readInline("color"));;
  auto sdim = child->getPointDim() + 1;
  modelview.setSpaceDim(sdim);

  //override physic_box 
  if (istream.hasAttribute("physic_box"))
  {
    auto physic_box = BoxNd::parseFromString(istream.readInline("physic_box"));
    child->setPhysicPosition(physic_box);
  }
  
  // transform physic box
  if (istream.hasAttribute("offset"))
    modelview *= Matrix::translate(PointNd::parseFromString(istream.readInline("offset"))).withSpaceDim(sdim);

  if (istream.pushContext("M"))
  {
    if (istream.hasAttribute("value"))
      modelview *= Matrix::parseFromString(istream.readInline("value")).withSpaceDim(sdim);

    for (auto it : istream.getCurrentContext()->getChilds())
    {
      if (it->name == "translate")
        modelview *= Matrix::translate(PointNd(cdouble(it->readString("x")), cdouble(it->readString("y")), cdouble(it->readString("z")))).withSpaceDim(sdim);

      else if (it->name == "rotate")
        modelview *= Matrix::rotate(Quaternion::fromEulerAngles(Utils::degreeToRadiant(cdouble(it->readString("x"))), Utils::degreeToRadiant(cdouble(it->readString("y"))), Utils::degreeToRadiant(cdouble(it->readString("z"))))).withSpaceDim(sdim);

      else if (it->name == "scale")
        modelview *= Matrix::nonZeroScale(PointNd(cdouble(it->readString("x")), cdouble(it->readString("y")), cdouble(it->readString("z")))).withSpaceDim(sdim);

      else if (it->name == "M")
        modelview *= Matrix::parseFromString(it->readString("value")).withSpaceDim(sdim);
    }

    istream.popContext("M");
  }

  VisusAssert(modelview.getSpaceDim() == sdim);
  child->setPhysicPosition(Position(modelview, child->getPhysicPosition()));
  addChild(name, child);

}

///////////////////////////////////////////////////////////
void IdxMultipleDataset::parseDatasets(ObjectStream& istream, Matrix modelview)
{
  auto context = istream.getCurrentContext();
  for (int I = 0; I < (int)context->getNumberOfChilds(); I++)
  {
    auto name = context->getChild(I).name;
    
    if (name=="dataset")
    {
      istream.pushContext("dataset");
      if (cbool(istream.readInline("enabled", "1"))) 
        parseDataset(istream, modelview);
      istream.popContext("dataset");
      continue;
    }

    if (name=="translate")
    {
      istream.pushContext("translate");
      if (cbool(istream.readInline("enabled", "1")))
        parseDatasets(istream, modelview * Matrix::translate(Point3d(cdouble(istream.readInline("x")), cdouble(istream.readInline("y")), cdouble(istream.readInline("z")))));
      istream.popContext("translate");
      continue;
    }

    if (name=="rotate")
    {
      istream.pushContext("rotate");
      if (cbool(istream.readInline("enabled", "1")))
        parseDatasets(istream, modelview * Matrix::rotate(Quaternion::fromEulerAngles(Utils::degreeToRadiant(cdouble(istream.readInline("x"))), Utils::degreeToRadiant(cdouble(istream.readInline("y"))), Utils::degreeToRadiant(cdouble(istream.readInline("z"))))));
      istream.popContext("rotate");
      continue;
    }

    if (name=="scale")
    {
      istream.pushContext("scale");
      if (cbool(istream.readInline("enabled", "1")))
        parseDatasets(istream, modelview * Matrix::nonZeroScale(PointNd(cdouble(istream.readInline("x")), cdouble(istream.readInline("y")), cdouble(istream.readInline("z")))));
      istream.popContext("scale");
      continue;
    }

    //pass throught
    if (name != "field")
    {
      istream.pushContext(name);
      if (cbool(istream.readInline("enabled", "1")))
        parseDatasets(istream, modelview);

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

  for (auto it : down_datasets)
  {
    for (auto field : it.second->getFields())
    {
      auto arg = getInputName(it.first, field.name);
      Field FIELD = getFieldByName("output=" + arg  + ";");
      VisusAssert(FIELD.valid());
      FIELD.setDescription(it.first + "/" + field.getDescription());
      addField(FIELD);
    }
  }
}

typedef std::vector< std::pair<int, Point2i> > Edges;

Edges GetEdges(int pdim)
{
  if (pdim == 2)
  {
    return Edges({
      std::make_pair(0,Point2i(0,1)),
      std::make_pair(1,Point2i(1,2)),
      std::make_pair(0,Point2i(2,3)),
      std::make_pair(1,Point2i(3,0)),
      });
  }

  if (pdim == 3)
  {
    return Edges({
        std::make_pair(0,Point2i(0,1)),
        std::make_pair(1,Point2i(1,2)),
        std::make_pair(0,Point2i(2,3)),
        std::make_pair(1,Point2i(3,0)),
        std::make_pair(0,Point2i(4,5)),
        std::make_pair(1,Point2i(5,6)),
        std::make_pair(0,Point2i(6,7)),
        std::make_pair(1,Point2i(7,4)),
        std::make_pair(2,Point2i(0,4)),
        std::make_pair(2,Point2i(1,5)),
        std::make_pair(2,Point2i(2,6)),
        std::make_pair(2,Point2i(3,7))
      });
  }

  ThrowException("internal error");
  return Edges();
};

///////////////////////////////////////////////////////////
bool IdxMultipleDataset::openFromUrl(Url URL)
{
  auto DATASET = this;

  StringTree BODY;
  if (!BODY.fromXmlString(Utils::loadTextDocument(URL.toString())))
    return false;

  BODY.writeString("url", URL.toString());

  setUrl(URL);
  setDatasetBody(BODY.toString());

  ObjectStream istream(BODY, 'r');

  this->bMosaic = cbool(istream.readInline("mosaic"));

  parseDatasets(istream, Matrix::identity(4));

  if (down_datasets.empty())
  {
    VisusAssert(false);
    this->invalidate();
    return false;
  }

  auto first = getFirstChild();
  int pdim = first->getPointDim();
  int sdim = pdim + 1;

  IdxFile& IDXFILE = DATASET->idxfile;

  //for mosaic physic and logic are the same
  if (bMosaic)
  {
    auto LOGIC_BOX = BoxNd::invalid();
    for (auto it : down_datasets)
    {
      auto dataset = it.second;
      VisusAssert(dataset->getPhysicPosition().getBoxNi() == dataset->getLogicBox());
      dataset->logic_to_LOGIC = dataset->getPhysicPosition().getTransformation();
      VisusAssert(dataset->logic_to_LOGIC.submatrix(sdim - 1, sdim - 1).isIdentity()); // only offset
      LOGIC_BOX = LOGIC_BOX.getUnion(Position(dataset->logic_to_LOGIC, dataset->getLogicBox()).toAxisAlignedBox());
    }

    IDXFILE.logic_box = LOGIC_BOX.castTo<BoxNi>();
    IDXFILE.bounds = LOGIC_BOX;

    VisusAssert(IDXFILE.logic_box.p1 == PointNi(pdim));

    //i need the final right part to be as the child
    auto BITMASK = DatasetBitmask::guess(IDXFILE.logic_box.p2);
    auto bitmask = first->getBitmask();

    PointNi DIMS = BITMASK.getPow2Dims();
    PointNi dims = first->getBitmask().getPow2Dims();

    auto left = DatasetBitmask::guess(DIMS.innerDiv(dims)).toString();
    auto right = bitmask.toString().substr(1);
    BITMASK = DatasetBitmask(left + right);
    VisusReleaseAssert(BITMASK.getPow2Dims() == DIMS);
    VisusReleaseAssert(StringUtils::endsWith(BITMASK.toString(), right));

    IDXFILE.bitmask = BITMASK;
    IDXFILE.timesteps = first->getTimesteps();
    IDXFILE.fields = first->getFields();
    IDXFILE.bitsperblock = first->getDefaultBitsPerBlock();

    IDXFILE.validate(DATASET->getUrl());
    VisusReleaseAssert(IDXFILE.valid());

    //VisusInfo() << "MIDX idxfile is the following" << std::endl << IDXFILE.toString();
    setIdxFile(IDXFILE);

    return true;
  }

  //set PHYSIC_BOX (union of physic boxes)
  auto PHYSIC_BOX = BoxNd::invalid();
  for (auto it : down_datasets)
  {
    auto dataset = it.second;
    PHYSIC_BOX = PHYSIC_BOX.getUnion(dataset->getPhysicPosition().toAxisAlignedBox());
  }
  IDXFILE.bounds = PHYSIC_BOX;

  //set LOGIC_BOX (at the beginning is the PHYSIC_BOX) and logic_to_LOGIC
  //project into DATASET AXIS and try not to loose pixels
  //this is just an euristic
  auto VS = PointNd::zero(pdim);
  for (auto it : down_datasets)
  {
    auto dataset = it.second;
    auto logic_box = dataset->getLogicBox();
    auto bounds = dataset->getPhysicPosition();

    VisusAssert(pdim == 2 || pdim == 3);
    auto PHYSIC_POINTS = bounds.getPoints();
    for (auto edge : GetEdges(pdim))
    {
      int  axis = edge.first;
      auto npixels = logic_box.size()[axis];

      int AXIS = -1;
      double PROJECTION = NumericLimits<double>::lowest();
      for (int I = 0; I < pdim; I++)
      {
        auto projection = fabs(PHYSIC_POINTS[edge.second.y][I] - PHYSIC_POINTS[edge.second.x][I]);
        if (projection > PROJECTION)
        {
          AXIS = I;
          PROJECTION = projection;
        }
      }

      //PROJECTION * vs = npixels
      auto vs = npixels / PROJECTION;
      VS[AXIS] = std::max(VS[AXIS], vs);
    }
  }
  IDXFILE.logic_box = BoxNd(PointNd::zero(pdim), VS.innerMultiply(PHYSIC_BOX.size())).castTo<BoxNi>();

  //set logic_to_LOGIC
  for (auto it : down_datasets)
  {
    auto dataset = it.second;
    auto physic_to_PHYSIC = Matrix::identity(sdim);
    auto PHYSIC_to_LOGIC = Position::computeTransformation(IDXFILE.logic_box, PHYSIC_BOX);
    dataset->logic_to_LOGIC = PHYSIC_to_LOGIC * physic_to_PHYSIC * dataset->logicToPhysic();
  }

  //time
  {
    if (down_datasets.size() == 1)
    {
      if (auto dataset = std::dynamic_pointer_cast<IdxDataset>(first))
        IDXFILE.time_template = dataset->idxfile.time_template;
    }

    //union of all timesteps
    for (auto it : down_datasets)
      IDXFILE.timesteps.addTimesteps(it.second->getTimesteps());
  }

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
bool IdxMultipleDataset::executeQuery(SharedPtr<Access> access,SharedPtr<BoxQuery> QUERY)
{
  if (!QUERY || !QUERY->canExecute())
    return false;

  if (QUERY->aborted())
    return QUERY->setFailed("QUERY aboted");

  //for 'r' queries I can postpone the allocation
  if (QUERY->mode == 'w' && !QUERY->buffer)
    return QUERY->setFailed("write buffer not set");

  //execute N-Query (independentely) and blend them
  if (!bMosaic)
  {
    if (auto multiple_access = std::dynamic_pointer_cast<IdxMultipleAccess>(access))
    {
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

      //a projection happened? results will be unmergeable!
      if (OUTPUT.dims != QUERY->logic_samples.nsamples)
        QUERY->merge_mode = DoNotMergeSamples;
      QUERY->buffer = OUTPUT;
      QUERY->setCurrentResolution(QUERY->end_resolution);
      return true;
    }
  }

  // blending happens on the server
  if (!access)
    return IdxDataset::executePureRemoteQuery(QUERY);

  // example: since I can handle blocks, I can even enable caching adding for example a RamAccess/DiskAcces to the top
  return IdxDataset::executeQuery(access, QUERY);
    
}

////////////////////////////////////////////////////////////////////////
bool IdxMultipleDataset::nextQuery(SharedPtr<BoxQuery> QUERY)
{
  if (!IdxDataset::nextQuery(QUERY))
    return false;

  for (auto it : QUERY->down_queries)
  {
    auto  query   = it.second;
    auto  dataset = this->down_datasets[query->down_info.name]; VisusAssert(dataset);

    if (query)
      dataset->nextQuery(query);
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
