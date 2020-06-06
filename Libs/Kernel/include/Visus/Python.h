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

#ifndef _VISUS_PYTHON_ENGINE_H__
#define _VISUS_PYTHON_ENGINE_H__

#include <Visus/Array.h>
#include <Visus/StringUtils.h>
#include <Visus/ApplicationInfo.h>
#include <Visus/Path.h>

#include <queue>

#ifdef WIN32
#pragma warning( push )
#pragma warning (disable:4996)
#endif

#include <functional>

#ifdef WIN32
#pragma warning( pop )
#endif

#pragma push_macro("slots")
#undef slots

#if defined(_DEBUG) && defined(SWIG_PYTHON_INTERPRETER_NO_DEBUG)
/* Use debug wrappers with the Python release dll */
# undef _DEBUG
# include <Python.h>
# define _DEBUG
#else
# include <Python.h>
#endif

#pragma pop_macro("slots")

#include <pydebug.h>

//see SWIG_TYPE_TABLE necessary to share type info
#include <Visus/swigpyrun.h>

namespace Visus {

///////////////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API PythonEngine
{
public:

  VISUS_CLASS(PythonEngine)

  //____________________________________________
  class VISUS_KERNEL_API ScopedAcquireGil
  {
  public:

    VISUS_CLASS(ScopedAcquireGil)

    PyGILState_STATE* state = nullptr;

    //constructor
    ScopedAcquireGil()
    {
      this->state = new PyGILState_STATE();
      *this->state = PyGILState_Ensure();
    }

    //destructor
    ~ScopedAcquireGil()
    {
      if (this->state)
      {
        PyGILState_Release(*state);
        delete state;
      }
    }
  };

  //____________________________________________
  class VISUS_KERNEL_API ScopedReleaseGil 
  {
  public:

    PyThreadState* state = nullptr;

    //constructor
    ScopedReleaseGil()
      : state(PyEval_SaveThread()) {
    }

    //destructor
    ~ScopedReleaseGil() {
      PyEval_RestoreThread(state);
    }
  };

  typedef std::function<PyObject*(PyObject*, PyObject*)> Function;

#if PY_MAJOR_VERSION <3
#define char2wchar(arg) ((char*)arg)
#else

  static wchar_t* char2wchar(const char* value)
  {
#if PY_MINOR_VERSION<=4
    return _Py_char2wchar((char*)value, NULL);
#else
    return Py_DecodeLocale((char*)value, NULL);
#endif
  }

#endif

  //constructor
  PythonEngine(bool bVerbose = false)
  {
    static std::atomic<int> module_id(0);
    this->module_name = concatenate("__PythonEngine__", ++module_id);
    //PrintInfo("Creating PythonEngine",module_name,"...");

    ScopedAcquireGil acquire_gil;
    this->module = PyImport_AddModule(module_name.c_str());
    VisusReleaseAssert(this->module);

    this->globals = PyModule_GetDict(module); //borrowed

    auto builtins = PyEval_GetBuiltins(); VisusAssert(builtins);
    PyDict_SetItemString(this->globals, "__builtins__", builtins);

    if (bVerbose)
      PrintInfo("Trying to import OpenVisus...");

    execCode("from OpenVisus import *");

    if (bVerbose)
      PrintInfo("...imported OpenVisus");
  }

  //destructor
  virtual ~PythonEngine()
  {
    //PrintInfo("Destroying PythonEngine",this->module_name,"...");

    // Delete the module from sys.modules
    {
      ScopedAcquireGil acquire_gil;
      PyObject* modules = PyImport_GetModuleDict();
      PyDict_DelItemString(modules, module_name.c_str());
    }
  }

  //incrRef
  static void incrRef(PyObject* value) {
    Py_INCREF(value);
  }

  //decrRef
  static void decrRef(PyObject* value) {
    Py_DECREF(value);
  }

  //GetPythonErrorMessage
  static String GetPythonErrorMessage()
  {
    //see http://www.solutionscan.org/154789-python
    auto err = PyErr_Occurred();
    if (!err)
      return "";

    PyObject* type, * value, * traceback;
    PyErr_Fetch(&type, &value, &traceback);

    std::ostringstream out;

    out << "Python error: "
      << PythonEngine::convertToString(type) << " "
      << PythonEngine::convertToString(value) << " ";

    auto module_name = PyString_FromString("traceback");
    auto module = PyImport_Import(module_name);
    Py_DECREF(module_name);

    auto fn = module ? PyObject_GetAttrString(module, "format_exception") : nullptr;
    if (fn && PyCallable_Check(fn))
    {
      if (auto descr = PyObject_CallFunctionObjArgs(fn, type, value, traceback, NULL))
      {
        out << PythonEngine::convertToString(descr);
        Py_DECREF(descr);
      }
    }

    return out.str();
  }

  //PythonPrintCrLfIfNeeded
  static void PythonPrintCrLfIfNeeded()
  {
#if PY_MAJOR_VERSION < 3
    if (Py_FlushLine())
      PyErr_Clear();
#endif
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
        String error_msg = cstring("Python error code:\n", s, "\nError:\n", GetPythonErrorMessage());
        PyErr_Clear();
        PrintInfo(error_msg);
        ThrowException(error_msg);
      }
    }

    Py_DECREF(obj);
    PythonPrintCrLfIfNeeded();
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
        String error_msg = cstring("Python error code:\n", s, "\nError:\n", GetPythonErrorMessage());
        PyErr_Clear();
        PrintInfo(error_msg);
        ThrowException(error_msg);
      }
    }

    PythonPrintCrLfIfNeeded();
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
        decrRef(ret);
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
    decrRef(obj);
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

  //setError (to call when you return nullpptr in Function)
  static void setError(String explanation, PyObject* err = nullptr)
  {
    if (!err)
      err = PyExc_SystemError;

    PyErr_SetString(err, explanation.c_str());
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

  //getModuleAbortedAttr
  Aborted getModuleAbortedAttr(String name)
  {
    auto typeinfo = SWIG_TypeQuery("Visus::Aborted *");
    VisusAssert(typeinfo);

    auto py_object = getModuleAttr(name);
    if (!py_object)
      ThrowException("cannot find", name, "in module");

    Aborted* ptr = nullptr;
    int res = SWIG_ConvertPtr(py_object, (void**)&ptr, typeinfo, 0);

    if (!SWIG_IsOK(res) || !ptr)
      ThrowException("cannot cast", name, "to", typeinfo->name);

    Aborted ret = *ptr;

    if (SWIG_IsNewObj(res))
      delete ptr;

    return ret;
  }

  //getModuleArrayAttr
  Array getModuleArrayAttr(String name)
  {
    auto py_object = getModuleAttr(name);
    if (!py_object)
      ThrowException("cannot find", name, "in module");
    return pythonObjectToArray(py_object);
  }

  //pythonObjectToArray
  Array pythonObjectToArray(PyObject* py_object)
  {
    auto typeinfo = SWIG_TypeQuery("Visus::Array *");
    VisusAssert(typeinfo);

    Array* ptr = nullptr;
    int res = SWIG_ConvertPtr(py_object, (void**)&ptr, typeinfo, 0);

    if (!SWIG_IsOK(res) || !ptr)
      ThrowException("cannot convert to array");

    Array ret = *ptr;

    if (SWIG_IsNewObj(res))
      delete ptr;

    return ret;
  }

  //convertToString
  static String convertToString(PyObject* value)
  {
    if (!value)
      return "";

    PyObject* py_str = PyObject_Str(value);
    auto tmp = SWIG_Python_str_AsChar(py_str);
    String ret = tmp ? tmp : "";
    SWIG_Python_str_DelForPy3(tmp);
    Py_DECREF(py_str);
    return ret;
  }

  //printMessage (must have the GIL)
  void printMessage(String message)
  {
    PySys_WriteStdout("%s", message.c_str());
  }

public:

  //MainThreadState
  static PyThreadState*&  MainThreadState()
  {
    static PyThreadState* ret = nullptr;
    return ret;
  }

  //attach
  static void attach()
  {
    if (Py_IsInitialized())
    {
      PrintInfo("Visus is running (i.e. extending) python");
    }
    else
    {
      PrintInfo("Initializing embedded python...");

      Py_VerboseFlag = 0;
      auto& args = ApplicationInfo::args;
      std::vector<String> new_args;
      for (int I = 0; I < args.size(); I++)
      {
        if (StringUtils::startsWith(args[I], "-v")) {
          Py_VerboseFlag = (int)args[I].size() - 1;
          continue;
        }
        new_args.push_back(args[I]);
      }
      args = new_args;

      const char* arg0 = ApplicationInfo::args[0].c_str();
      Py_SetProgramName(char2wchar(arg0));

      //IMPORTANT: if you want to avoid the usual sys.path initialization
      //you can copy the python shared library (example: python36.dll) and create a file with the same name and _pth extension
      //(example python36_d._pth). in that you specify the directories to include. you can also for example a python36.zip file
      //or maybe you can set PYTHONHOME

      //skips initialization registration of signal handlers
      Py_InitializeEx(0);

      // acquire the gil
      PyEval_InitThreads();

      //add value PYTHONPATH in order to find the OpenVisus directory
      {
        auto path = KnownPaths::BinaryDirectory.toString() + "/../..";
#if WIN32
        path = StringUtils::replaceAll(path, "/", "\\\\");
#else
        path = StringUtils::replaceAll(path, "\\\\", "/");
#endif
        auto cmd = StringUtils::join({
          "import os,sys",
          "value=os.path.realpath('" + path + "')",
          "if not value in sys.path: sys.path.append(value)"
          }, "\r\n");

        PyRun_SimpleString(cmd.c_str());
      }

      //NOTE if you try to have multiple interpreters (Py_NewInterpreter) I get deadlock
      //see https://issues.apache.org/jira/browse/MODPYTHON-217
      //see https://trac.xapian.org/ticket/185
      MainThreadState() = PyEval_SaveThread();
    }

    PrintInfo("Python initialization done");

    //this is important to import swig libraries
    if (auto engine = std::make_shared<PythonEngine>(true))
    {
      ScopedAcquireGil acquire_gil;
      engine->execCode("print('PythonEngine is working fine')");
    }
  }

  //detach
  static void detach()
  {
    if (auto state=MainThreadState()) {
      PyEval_RestoreThread(state);
      Py_Finalize();
    }
  }

private:

  String  module_name;

  PyObject* module = nullptr;
  PyObject* globals = nullptr;

  //fixPath
  static String fixPath(String value) {
#if WIN32
    return StringUtils::replaceAll(value, "/", "\\\\");
#else
    return StringUtils::replaceAll(value, "\\\\", "/");
#endif
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


};



/////////////////////////////////////////////////////
class VISUS_KERNEL_API PythonEnginePool
{
public:

  //createEngine
  SharedPtr<PythonEngine> createEngine() {
    ScopedLock lock(this->lock);
    if (freelist.empty()) 
      freelist.push(std::make_shared<PythonEngine>());
    auto ret = freelist.front();
    freelist.pop();
    return ret;
  }

  //releaseEngine
  void releaseEngine(SharedPtr<PythonEngine> value) {
    ScopedLock lock(this->lock);
    freelist.push(value);
  }

private:

  CriticalSection lock;
  std::queue< SharedPtr<PythonEngine> > freelist;

};

} //namespace Visus

#endif //_VISUS_PYTHON_ENGINE_H__
