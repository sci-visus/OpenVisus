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

#include <Visus/Db.h>
#include <Visus/Array.h>

#include <Visus/DatasetBitmask.h>
#include <Visus/GoogleMapsDataset.h>
#include <Visus/DatasetArrayPlugin.h>
#include <Visus/OnDemandAccess.h>
#include <Visus/StringTree.h>
#include <Visus/IdxDataset.h>
#include <Visus/IdxMultipleDataset.h>

#if VISUS_PYTHON

#include <Visus/Python.h>

namespace Visus {

class InputTerm
{
public:

  VISUS_NON_COPYABLE_CLASS(InputTerm)

  IdxMultipleDataset*      DATASET;
  BoxQuery*                QUERY;
  SharedPtr<Access>        ACCESS;

  SharedPtr<PythonEngine>  engine;
  Aborted                  aborted;

  //constructor
  InputTerm(SharedPtr<PythonEngine> engine_, IdxMultipleDataset* DATASET_, BoxQuery* QUERY_, SharedPtr<Access> ACCESS_, Aborted aborted_)
    : engine(engine_), DATASET(DATASET_), QUERY(QUERY_), ACCESS(ACCESS_), aborted(aborted_) {

    VisusAssert(!DATASET->is_mosaic);

    {
      PythonEngine::ScopedAcquireGil acquire_gil;

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

      engine->addModuleFunction("doPublish", [this](PyObject* self, PyObject* args) {
        auto output = engine->getModuleArrayAttr("output");
        if (output && QUERY && QUERY->incrementalPublish)
          QUERY->incrementalPublish(output);
        return nullptr;
      });

      engine->addModuleFunction("voronoi", [this](PyObject* self, PyObject* args) {return blendBuffers(BlendBuffers::VororoiBlend, args); });
      engine->addModuleFunction("averageBlend", [this](PyObject* self, PyObject* args) {return blendBuffers(BlendBuffers::AverageBlend, args); });
      engine->addModuleFunction("noBlend", [this](PyObject* self, PyObject* args) {return blendBuffers(BlendBuffers::NoBlend, args); });
    }
  }

  //destructor
  virtual ~InputTerm()
  {
    PythonEngine::ScopedAcquireGil acquire_gil;
    engine->delModuleAttr("query_time");
    engine->delModuleAttr("doPublish");
    engine->delModuleAttr("voronoiBlend");
    engine->delModuleAttr("averageBlend");
    engine->delModuleAttr("noBlend");
    engine->delModuleAttr("input");
  }

  //computeOutput
  Array computeOutput(String code)
  {
    PythonEngine::ScopedAcquireGil acquire_gil;
    engine->execCode(code);

    auto ret = engine->getModuleArrayAttr("output");
    if (!ret && !aborted())
      ThrowException("empty 'output' value");

    if (DATASET->debug_mode & IdxMultipleDataset::DebugSaveImages)
    {
      static int cont = 0;
      ArrayUtils::saveImage(concatenate("temp/", cont++, ".up.result.png"), ret);
    }

    return ret;
  }

  //newDynamicObject
  PyObject* newDynamicObject(std::function<PyObject* (String)> getattr)
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
      ThrowException("input['", expr1, "'] not found");

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
      ThrowException("input['", expr1, "']['", expr2, "'] not found");

    int pdim = DATASET->getPointDim();

    //only getting dtype for field name
    if (!QUERY)
      return engine->newPyObject(Array(PointNi(pdim), field.dtype));

    {
      PythonEngine::ScopedReleaseGil release_gil;
      auto down_query = DATASET->createDownQuery(this->ACCESS, this->QUERY, expr1, expr2);
      ret = DATASET->executeDownQuery(QUERY, down_query);
    }

    return engine->newPyObject(ret);
  }

  //blendBuffers
  PyObject* blendBuffers(BlendBuffers::Type type, PyObject* args)
  {
    int N = args ? (int)PyObject_Length(args) : 0;
    BlendBuffers blend(type, aborted);

    //preview only
    if (!QUERY)
    {
      if (!N)
      {
        for (auto it : DATASET->down_datasets)
          blend.addBlendArg(Array(PointNi(DATASET->getPointDim()), it.second->getDefaultField().dtype));
      }
      else
      {
        //arguments are arrays
        PyObject* arg0 = nullptr;
        if (!PyArg_ParseTuple(args, "O:blendBuffers", &arg0))
        {
          PythonEngine::setError("invalid argument");
          return (PyObject*)nullptr;
        }

        if (!PyList_Check(arg0))
        {
          PythonEngine::setError("invalid argument");
          return (PyObject*)nullptr;
        }

        for (int I = 0; I < N; I++)
          blend.addBlendArg(engine->pythonObjectToArray(PyList_GetItem(arg0, I)));
      }
    }
    else
    {
      PythonEngine::ScopedReleaseGil release_gil;

      //special case: empty argument means all down dataset default fields
      if (!N)
      {
        for (auto it : DATASET->down_datasets)
        {
          auto dataset_name = it.first;
          auto fieldname = it.second->getDefaultField().name;

          auto query = DATASET->createDownQuery(this->ACCESS, this->QUERY, dataset_name, fieldname);
          if (!query || query->failed() || query->aborted())
            continue;

          DATASET->executeDownQuery(QUERY, query);

          if (!query->down_info.BUFFER || query->aborted())
            continue;

          blend.addBlendArg(query->down_info.BUFFER, query->down_info.PIXEL_TO_LOGIC, query->down_info.LOGIC_CENTROID);
        }
      }
      else
      {
        for (auto it : QUERY->down_queries)
        {
          auto query = it.second;
          if (!query || !query->down_info.BUFFER || query->aborted())
            continue;

          blend.addBlendArg(query->down_info.BUFFER, query->down_info.PIXEL_TO_LOGIC, query->down_info.LOGIC_CENTROID);
        }
      }
    }

    return engine->newPyObject(blend.result);
  }

};

class PyIdxMultipleDataset : public IdxMultipleDataset
{
public:

  SharedPtr<PythonEnginePool> pool = std::make_shared<PythonEnginePool>();

  //constructor
  PyIdxMultipleDataset() {
  }

  //destructor
  virtual ~PyIdxMultipleDataset() {
  }

  //getFieldByNameThrowEx
  virtual Field getFieldByNameThrowEx(String FIELDNAME) const override
  {
    if (is_mosaic)
      return this->IdxDataset::getFieldByNameThrowEx(FIELDNAME);

    String CODE;
    if (find_field.count(FIELDNAME))
      CODE = find_field.find(FIELDNAME)->second.name;  //existing field (it's a symbolic name)
    else
      CODE = FIELDNAME; //the fieldname itself is the expression

    auto engine = isServerMode() ? std::make_shared<PythonEngine>() : pool->createEngine();
    auto OUTPUT = InputTerm(engine, const_cast<PyIdxMultipleDataset*>(this), nullptr, SharedPtr<Access>(), Aborted()).computeOutput(CODE);
    if (!isServerMode()) pool->releaseEngine(engine);
    return Field(CODE, OUTPUT.dtype);
  }

  //executeQuery
  virtual bool executeQuery(SharedPtr<Access> ACCESS, SharedPtr<BoxQuery> QUERY) override
  {
    if (is_mosaic)
      return IdxDataset::executeQuery(ACCESS, QUERY);

    auto MULTIPLE_ACCESS = std::dynamic_pointer_cast<IdxMultipleAccess>(ACCESS);
    if (!MULTIPLE_ACCESS)
      return IdxDataset::executeQuery(ACCESS, QUERY);

    if (!QUERY)
      return false;

    if (!(QUERY->isRunning() && QUERY->getCurrentResolution() < QUERY->getEndResolution()))
      return false;

    if (QUERY->aborted())
    {
      QUERY->setFailed("QUERY aboted");
      return false;
    }

    if (QUERY->mode == 'w')
    {
      QUERY->setFailed("not supported");
      return false;
    }

    //execute N-Query (independentely) and blend them
    auto engine = isServerMode() ? std::make_shared<PythonEngine>() : pool->createEngine();

    Array  OUTPUT;
    try
    {
      OUTPUT = InputTerm(engine, this, QUERY.get(), ACCESS, QUERY->aborted).computeOutput(QUERY->field.name);
    }
    catch (std::exception ex)
    {
      QUERY->setFailed(QUERY->aborted() ? "query aborted" : ex.what());
    }

    if (!isServerMode())
      pool->releaseEngine(engine);

    if (QUERY->failed())
      return false;

    //a projection happened? results will be unmergeable!
    if (OUTPUT.dims != QUERY->logic_samples.nsamples)
      QUERY->merge_mode = DoNotMergeSamples;

    QUERY->buffer = OUTPUT;
    QUERY->setCurrentResolution(QUERY->end_resolution);
    return true;
  }

};

} //namespace Visus

#endif

namespace Visus {

bool DbModule::bAttached = false;

///////////////////////////////////////////////////////////////////////////////////////////
void DbModule::attach()
{
  if (bAttached)  
    return;
  
  PrintInfo("Attaching DbModule...");
  
  bAttached = true;

  KernelModule::attach();

  DatasetFactory::allocSingleton();
  DatasetFactory::getSingleton()->registerDatasetType("GoogleMapsDataset",  []() {return std::make_shared<GoogleMapsDataset>(); });
  DatasetFactory::getSingleton()->registerDatasetType("IdxDataset",         []() {return std::make_shared<IdxDataset>(); });
  DatasetFactory::getSingleton()->registerDatasetType("IdxMultipleDataset", []() {return std::make_shared<IdxMultipleDataset>(); });

#if VISUS_PYTHON
  DatasetFactory::getSingleton()->registerDatasetType("IdxMultipleDataset", []() {return std::make_shared<PyIdxMultipleDataset>(); });
#endif 

  ArrayPlugins::getSingleton()->values.push_back(std::make_shared<DatasetArrayPlugin>());

  auto config = getModuleConfig();

  if (auto value = config->readInt("Configuration/OnDemandAccess/External/nconnections", 8))
    OnDemandAccess::Defaults::nconnections = value;


  //remove broken files
  {
    auto config = getModuleConfig();
    {
      auto directory = config->readString("Configuration/IdxDataset/RemoveLockFiles/directory");
      if (!directory.empty())
        IdxDataset::tryRemoveLockAndCorruptedBinaryFiles(directory);
    }
  }

  PrintInfo("Attached DbModule");
}

//////////////////////////////////////////////
void DbModule::detach()
{
  if (!bAttached)  
    return;
  
  PrintInfo("Detaching DbModule...");
  
  bAttached = false;

  DatasetFactory::releaseSingleton();

  KernelModule::detach();

  PrintInfo("Detached DbModule.");
}



} //namespace Visus 

