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

#ifndef VISUS_PY_MULTIPLE_DATASET_H__
#define VISUS_PY_MULTIPLE_DATASET_H__

#include <Visus/Db.h>

#if WIN32 && defined(_DEBUG) 
#   undef _DEBUG
#   include <Python.h>
#   define _DEBUG 1
#else
#   include <Python.h>
#endif

namespace Visus {


inline String cstring(PyObject* value)
{
  if (!value) return "";
  PyObject* py_str = PyObject_Str(value);
  const char* tmp = py_str ? PyUnicode_AsUTF8(py_str) : nullptr;
  String ret = tmp ? tmp : "";
  SWIG_Python_str_DelForPy3(tmp);
  Py_DECREF(py_str);
  return ret;
}

///////////////////////////////////////////////////////////////////////////////////////
class ScopedAcquireGil
{
public:
  PyGILState_STATE state = PyGILState_Ensure();
  ScopedAcquireGil() {}
  ~ScopedAcquireGil() { PyGILState_Release(state); }
};

///////////////////////////////////////////////////////////////////////////////////////
class ScopedReleaseGil
{
public:
  PyThreadState* state = PyEval_SaveThread();
  ScopedReleaseGil() {}
  ~ScopedReleaseGil() { PyEval_RestoreThread(state); }
};

///////////////////////////////////////////////////////////////////////////////////////
class ComputeOutput 
{
public:

  typedef std::function<PyObject* (PyObject*, PyObject*)> Function;

  IdxMultipleDataset*      DATASET;
  BoxQuery*                QUERY;
  SharedPtr<Access>        ACCESS;
  Aborted                  aborted;

  String    module_name;
  PyObject* module = nullptr;
  PyObject* globals = nullptr;

  //constructor
  ComputeOutput(const IdxMultipleDataset* DATASET_, BoxQuery* QUERY_, SharedPtr<Access> ACCESS_, Aborted aborted_)
    : DATASET(const_cast<IdxMultipleDataset*>(DATASET_)), QUERY(QUERY_), ACCESS(ACCESS_), aborted(aborted_) {
    {
      ScopedAcquireGil acquire_gil;
      static std::atomic<int> module_id(0);
      this->module_name = concatenate("__PythonEngine__", ++module_id);
      this->module = PyImport_AddModule(module_name.c_str());
      VisusReleaseAssert(this->module);
      this->globals = PyModule_GetDict(module); //borrowed
      auto builtins = PyEval_GetBuiltins(); VisusAssert(builtins);
      PyDict_SetItemString(this->globals, "__builtins__", builtins);

      execCode(
        "from OpenVisus import *\n"
        "class DynamicObject:\n"
        "  def __getattr__(self, args) : return self.forwardGetAttr(args)\n"
        "  def __getitem__(self, args) : return self.forwardGetAttr(args)\n"
      );

      auto py_input = newDynamicObject([this](String expr1) {
        return getAttr1(expr1);
      });
      setModuleAttr("input", py_input);
      Py_DECREF(py_input);

      //for fieldname=function_of(QUERY->time) 
      //NOTE: for getField(), I think I can use the default timestep since I just want to know the dtype
      setModuleAttr("query_time", QUERY ? QUERY->time : DATASET->getTimesteps().getDefault());

      addModuleFunction("doPublish", [this](PyObject* self, PyObject* args) {

        auto output = getModuleAttr("output");

        if (!output)
          ThrowException("C++ ThrowException, ouput not set");

        auto array = pythonObjectToArray(output);

        if (array.valid() && QUERY && QUERY->incrementalPublish)
          QUERY->incrementalPublish(array);

        return nullptr;
      });

      addModuleFunction("voronoi", [this](PyObject* self, PyObject* args) {
        return blendBuffers(BlendBuffers::VororoiBlend, args); 
       });

      addModuleFunction("averageBlend", [this](PyObject* self, PyObject* args) {
        return blendBuffers(BlendBuffers::AverageBlend, args); 
        });

      addModuleFunction("noBlend", [this](PyObject* self, PyObject* args) {
        return blendBuffers(BlendBuffers::NoBlend, args); 
        });
    }
  }

  //destructor
  virtual ~ComputeOutput()
  {
    ScopedAcquireGil acquire_gil;
    delModuleAttr("query_time");
    delModuleAttr("doPublish");
    delModuleAttr("voronoiBlend");
    delModuleAttr("averageBlend");
    delModuleAttr("noBlend");
    delModuleAttr("input");
    PyDict_DelItemString(PyImport_GetModuleDict(), module_name.c_str());
  }

  //doCompute
  Array doCompute(String code)
  {
    ScopedAcquireGil acquire_gil;
    execCode(code);

    auto output = getModuleAttr("output");

    if (!output)
      ThrowException("C++ ThrowException, output not set");

    auto ret = pythonObjectToArray(output);

    if (!ret.valid())
    {
      if (aborted())
        return ret;
      else
        ThrowException("C++ ThrowException, output not valid");
    }

    if (DATASET->debug_mode & IdxMultipleDataset::DebugSaveImages)
    {
      static int cont = 0;
      ArrayUtils::saveImage(concatenate("tmp/debug_pymultipledataset/", cont++, ".up.result.png"), ret);
    }

    return ret;
  }

private:

  //getPythonErrorMessage
  static String getPythonErrorMessage()
  {
    //see http://www.solutionscan.org/154789-python
    auto err = PyErr_Occurred();
    if (!err)
      return "";

    PyObject* type, * value, * traceback;
    PyErr_Fetch(&type, &value, &traceback);

    std::ostringstream out;

    out << "Python error: " << cstring(type) << " " << cstring(value) << " ";

    auto module_name = PyString_FromString("traceback");
    auto module = PyImport_Import(module_name);
    Py_DECREF(module_name);

    auto fn = module ? PyObject_GetAttrString(module, "format_exception") : nullptr;
    if (fn && PyCallable_Check(fn))
    {
      if (auto descr = PyObject_CallFunctionObjArgs(fn, type, value, traceback, NULL))
      {
        out << cstring(descr);
        Py_DECREF(descr);
      }
    }

    return out.str();
  }

  //execCode
  void execCode(String s)
  {
    auto obj = PyRun_StringFlags(s.c_str(), Py_file_input, globals, globals, nullptr);
    bool bError = (obj == nullptr);
    if (bError)
    {
      if (PyErr_Occurred())
      {
        String error_msg = cstring("C++ catched Python error. Source code :\n", s, "\nPython Error Message:\n", getPythonErrorMessage());
        PyErr_Clear();
        PrintInfo(error_msg);
        ThrowException(error_msg);
      }
    }
    Py_DECREF(obj);
  }

  //evalCode
  PyObject* evalCode(String s)
  {
    //see https://bugs.python.org/issue405837
    //Return value: New reference.
    auto obj = PyRun_StringFlags(s.c_str(), Py_eval_input, globals, globals, nullptr);

    if (bool bMaybeError = (obj == nullptr))
    {
      if (PyErr_Occurred())
      {
        String error_msg = cstring("C++ catched Python error. Source code :\n", s, "\nPython Error Message:\n", getPythonErrorMessage());
        PyErr_Clear();
        PrintInfo(error_msg);
        ThrowException(error_msg);
      }
    }

    return obj;
  }

  //newPyObject
  PyObject* newPyObject(double value) {
    return PyFloat_FromDouble(value);;
  }

  //newPyObject
  PyObject* newPyObject(int value) {
    return PyLong_FromLong(value);
  }

  //newPyObject
  PyObject* newPyObject(String s) {
    return PyString_FromString(s.c_str());
  }

  //newPyObject
  PyObject* newPyObject(Aborted value)
  {
    auto typeinfo = SWIG_TypeQuery("Visus::Aborted *");
    VisusAssert(typeinfo);
    Aborted* ptr = new Aborted(value);
    return SWIG_NewPointerObj(ptr, typeinfo, SWIG_POINTER_OWN);
  }

  //newPyObject
  PyObject* newPyObject(Array value)
  {
    auto typeinfo = SWIG_TypeQuery("Visus::Array *");
    VisusAssert(typeinfo);
    auto ptr = new Array(value);
    return SWIG_NewPointerObj(ptr, typeinfo, SWIG_POINTER_OWN);
  }

  //newPyObject
  template <typename T>
  PyObject* newPyObject(const std::vector<T>& values)
  {
    auto ret = PyTuple_New(values.size());
    if (!ret)
      return nullptr;

    for (int i = 0; i < values.size(); i++) {
      auto value = newPyObject(values[i]);
      if (!value) {
        Py_DECREF(ret);
        return nullptr;
      }
      PyTuple_SetItem(ret, i, value);
    }
    return ret;
  }

  //setModuleAttr
  void setModuleAttr(String name, PyObject* value) {
    PyDict_SetItemString(globals, name.c_str(), value);
  }

  //setModuleAttr
  template <typename Value>
  void setModuleAttr(String name, Value value) {
    auto obj = newPyObject(value);
    setModuleAttr(name, obj);
    Py_DECREF(obj);
  }

  //getModuleAttr
  PyObject* getModuleAttr(String name) {
    return PyDict_GetItemString(globals, name.c_str()); //borrowed
  }

  //hasModuleAttr
  bool hasModuleAttr(String name) {
    return getModuleAttr(name) ? true : false;
  }

  //delModuleAttr
  void delModuleAttr(String name) {
    if (hasModuleAttr(name))
      PyDict_DelItemString(globals, name.c_str());
  }

  //internalNewPyFunction
  PyObject* internalNewPyFunction(PyObject* self, String name, Function fn)
  {
    //see http://code.activestate.com/recipes/54352-defining-python-class-methods-in-c/
    //see http://bannalia.blogspot.it/2016/07/passing-capturing-c-lambda-functions-as.html
    //see https://stackoverflow.com/questions/26716711/documentation-for-pycfunction-new-pycfunction-newex

    class PyCapsuleInfo
    {
    public:
      UniquePtr<PyMethodDef> mdef;
      Function               fn;
      PyObject* self = nullptr;
    };

    //callFunction
    auto callFunction = [](PyObject* py_capsule, PyObject* args)->PyObject*
    {
      auto info = static_cast<PyCapsuleInfo*>(PyCapsule_GetPointer(py_capsule, nullptr));
      return info->fn(info->self, args);
    };

    auto info = new PyCapsuleInfo();
    info->mdef.reset(new PyMethodDef({ name.c_str(), callFunction, METH_VARARGS,nullptr }));
    info->fn = fn;
    info->self = self;

    auto py_capsule = PyCapsule_New(info, nullptr, /*destrutor*/[](PyObject* py_capsule) {
      auto info = static_cast<PyCapsuleInfo*>(PyCapsule_GetPointer(py_capsule, nullptr));
      delete info;
    });

    auto ret = PyCFunction_NewEx(/*method definition*/info->mdef.get(), /*self*/py_capsule,/*module*/self ? nullptr : this->module);
    Py_DECREF(py_capsule);
    return ret;
  }

  //addModuleFunction
  void addModuleFunction(String name, Function fn)
  {
    auto py_fn = internalNewPyFunction(/*self*/nullptr, name, fn);
    setModuleAttr(name, py_fn);
    Py_DECREF(py_fn);
  }

  //addObjectMethod
  void addObjectMethod(PyObject* self, String name, Function fn)
  {
    auto py_fn = internalNewPyFunction(self, name, fn);
    auto py_name = PyString_FromString(name.c_str());
    PyObject_SetAttr(self, py_name, py_fn);
    Py_DECREF(py_fn);
    Py_XDECREF(py_name);
  }

  //pythonObjectToArray
  Array pythonObjectToArray(PyObject* py_object)
  {
    Array* ptr = nullptr;
    int res = SWIG_ConvertPtr(py_object, (void**)&ptr, SWIG_TypeQuery("Visus::Array *"), 0);

    if (!SWIG_IsOK(res) || !ptr)
      ThrowException("C++ cannot convert to array");

    Array ret = *ptr;

    if (SWIG_IsNewObj(res))
      delete ptr;

    return ret;
  }

  //newDynamicObject
  PyObject* newDynamicObject(std::function<PyObject* (String)> getattr)
  {
    auto ret = evalCode("DynamicObject()");  //new reference
    VisusAssert(ret);
    addObjectMethod(ret, "forwardGetAttr", [getattr](PyObject*, PyObject* args) {

      VisusAssert(PyTuple_Check(args));
      VisusAssert(PyTuple_Size(args) == 1);
      auto arg0 = PyTuple_GetItem(args, 0); VisusAssert(arg0);//borrowed
      auto expr = cstring(arg0); VisusAssert(!expr.empty());
      if (!getattr) {
        PyErr_SetString(PyExc_SystemError, "getattr is null");
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
      return newPyObject(DATASET->getTimesteps().asVector());

    auto dataset = DATASET->getChild(expr1);
    if (!dataset)
      ThrowException("C++ error. input['", expr1, "'] not found");

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
      return newPyObject(dataset->getTimesteps().asVector());

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

    //execute a query (expr2 is the fieldname)
    Field field = dataset->getField(expr2);

    if (!field.valid())
      ThrowException("C++ error. input['", expr1, "']['", expr2, "'] not found");

    //only getting dtype for field name
    Array array;
    if (bool bPreview=!QUERY)
    {
      array = Array(PointNi(DATASET->getPointDim()), field.dtype); 
    }
    else
    {
      ScopedReleaseGil release_gil;
      array = DATASET->executeDownQuery(QUERY, this->ACCESS, expr1, expr2);
    }

    return newPyObject(array);
  }

  //blendBuffers
  PyObject* blendBuffers(BlendBuffers::Type type, PyObject* args)
  {
    int nargs = args ? (int)PyObject_Length(args) : 0;
    BlendBuffers blend(type, aborted);

    //arguments are arrays, can we extend to be BoxQuery for 'deferred' evaluation?
    if (nargs)
    {
      PyObject* arg0 = nullptr;
      if (!PyArg_ParseTuple(args, "O:blendBuffers", &arg0))
      {
        PyErr_SetString(PyExc_SystemError, "invalid argument");
        return (PyObject*)nullptr;
      }

      if (!PyList_Check(arg0))
      {
        PyErr_SetString(PyExc_SystemError, "invalid argument");
        return (PyObject*)nullptr;
      }

      std::vector<Array> buffers;
      for (int I = 0; I < nargs; I++)
        buffers.push_back(pythonObjectToArray(PyList_GetItem(arg0, I)));

      {
        ScopedReleaseGil release_gil;
        for (auto buffer : buffers)
        {
          if (!buffer.valid() || (QUERY && QUERY->aborted()))
            continue;
          blend.addBlendArg(buffer);
        }
      }
    }
    else
    {
      ScopedReleaseGil release_gil;

      for (auto it : DATASET->down_datasets)
      {
        auto dataset_name = it.first;
        auto field = it.second->getField();

        Array buffer = QUERY?
          DATASET->executeDownQuery(QUERY, this->ACCESS, dataset_name, field.name) :
          Array(PointNi(DATASET->getPointDim()), field.dtype);

        if (!buffer.valid() || (QUERY && QUERY->aborted()))
          continue;

        blend.addBlendArg(buffer);
      }
    }

    return newPyObject(blend.result);
  }

};

//////////////////////////////////////////////////////////////////////////////////
class PyMultipleDataset : public IdxMultipleDataset
{
public:

  //constructor
  PyMultipleDataset() {
  }

  //destructor
  virtual ~PyMultipleDataset() {
  }

  //computeOuput
  virtual Array computeOuput(BoxQuery* QUERY, SharedPtr<Access> ACCESS, Aborted aborted, String CODE) const override {
    return ComputeOutput(this, QUERY, ACCESS, aborted).doCompute(CODE);
  }

};

} //namespace Visus

#endif //VISUS_PY_MULTIPLE_DATASET_H__




