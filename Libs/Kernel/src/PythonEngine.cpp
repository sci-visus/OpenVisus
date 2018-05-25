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

#include <Visus/PythonEngine.h>
#include <Visus/Thread.h>
#include <Visus/Log.h>
#include <Visus/ApplicationInfo.h>
#include <Visus/Path.h>

#include <cctype>

#include <pydebug.h>

namespace Visus {

PyThreadState* PythonEngine::mainThreadState=nullptr;

static std::set<String> ReservedWords =
{
  "and", "del","from","not","while","as","elif","global","or","with","assert", "else","if",
  "pass","yield","break","except","import","print", "class","exec""in","raise","continue", 
  "finally","is","return","def","for","lambda","try"
};


///////////////////////////////////////////////////////////////////////////
ScopedAcquireGil::ScopedAcquireGil() 
{
  state = PyGILState_Ensure();
}


///////////////////////////////////////////////////////////////////////////
ScopedAcquireGil::~ScopedAcquireGil()
{
  PyGILState_Release(state);
}

///////////////////////////////////////////////////////////////////////////
ScopedReleaseGil::ScopedReleaseGil()
  : state(PyEval_SaveThread()) {
}


///////////////////////////////////////////////////////////////////////////
ScopedReleaseGil::~ScopedReleaseGil() {
  PyEval_RestoreThread(state);
}


////////////////////////////////////////////////////////////////////////////////////
bool PythonEngine::isGoodVariableName(String name)
{
  if (name.empty() || ReservedWords.count(name))
    return false;

  if (!std::isalpha(name[0]))
    return false;

  for (int I = 1; I<(int)name.length(); I++)
  {
    if (!(std::isalnum(name[I]) || name[I] == '_'))
      return false;
  }

  return true;
};

///////////////////////////////////////////////////////////////////////////
static bool runningInsidePyMain() {
  const auto& args = ApplicationInfo::args;
  return args.empty() || args[0].empty() || args[0] == "__main__";
}


///////////////////////////////////////////////////////////////////////////
void InitPython()
{
  if (runningInsidePyMain())
    return;

    VisusInfo() << "Initializing python...";

  Py_VerboseFlag = 0;
  auto& args = ApplicationInfo::args;
  std::vector<String> new_args;
  for (int I = 0; I < args.size(); I++)
  {
    if (args[I] == "-v") {
      Py_VerboseFlag = 1;
      continue;
    }
    if (args[I] == "-vv") {
      Py_VerboseFlag = 2;
      continue;
    }
    else if (args[I] == "-vvv") {
      Py_VerboseFlag = 3;
      continue;
    }
    new_args.push_back(args[I]);
  }
  args = new_args;

  const char* arg0 = ApplicationInfo::args[0].c_str();

#if PY_MAJOR_VERSION >= 3
  #if PY_MINOR_VERSION<=4
  #define Py_DecodeLocale _Py_char2wchar
  #endif
  Py_SetProgramName(Py_DecodeLocale(arg0, NULL));
#else
  Py_SetProgramName((char*)arg0);
#endif

  //skips initialization registration of signal handlers
  Py_InitializeEx(0);

  // acquire the gil
  PyEval_InitThreads();

  //NOTE if you try to have multiple interpreters (Py_NewInterpreter) I get deadlock
  //see https://issues.apache.org/jira/browse/MODPYTHON-217
  //see https://trac.xapian.org/ticket/185
  PythonEngine::mainThreadState = PyEval_SaveThread();

  VisusInfo() << "Python initialization done";
}


///////////////////////////////////////////////////////////////////////////
void ShutdownPython()
{
  if (runningInsidePyMain())
    return;

  VisusInfo() << "Shutting down python...";
  PyEval_RestoreThread(PythonEngine::mainThreadState);
  Py_Finalize();

  VisusInfo() << "Python shutting down done";
}

////////////////////////////////////////////////////////////////////////////////////
void PythonEngine::incrRef(PyObject* value) {
  Py_INCREF(value);
}

////////////////////////////////////////////////////////////////////////////////////
void PythonEngine::decrRef(PyObject* value) {
  Py_DECREF(value);
}

////////////////////////////////////////////////////////////////////////////////////
String PythonEngine::fixPath(String value) {
#if WIN32
  return StringUtils::replaceAll(value, "/", "\\\\");
#else
  return StringUtils::replaceAll(value, "\\\\", "/");
#endif
}

////////////////////////////////////////////////////////////////////////////////////
void PythonEngine::addSysPath(String value) {
  value = fixPath(value);
  auto cmd = "import sys; sys.path.append('" + value + "')";
  VisusInfo() << cmd;
  execCode(cmd);
}

  ///////////////////////////////////////////////////////////////////////////
PythonEngine::PythonEngine(bool bVerbose)
{
  ScopedAcquireGil acquire_gil;

  module_name = StringUtils::format() << "__PythonEngine__" << ++ModuleId();

  this->module  = PyImport_AddModule(module_name.c_str()); 
  if (!module) {

    if (bVerbose)
      VisusInfo()<<"PyImport_AddModule(\""<< module_name<<"\") failed";
    VisusAssert(false);
  }

  this->globals = PyModule_GetDict(module); //borrowed

  auto builtins = PyEval_GetBuiltins(); VisusAssert(builtins);
  PyDict_SetItemString(this->globals, "__builtins__", builtins);

  auto addSysPath=[&](String value) {
    value=fixPath(value);
    auto cmd="import sys; sys.path.append('" + value+ "')";

    if (bVerbose)
      VisusInfo()<<cmd;
    execCode(cmd);
  };

  //add the directory of the executable
  if (runningInsidePyMain())
  {
    if (bVerbose)
      VisusInfo() << "Visus is running inside PyMain";
  }
  else
  {
    if (bVerbose)
      VisusInfo() << "Visus is NOT running inside PyMain";

    //try to find visus.py in the same directory where the app is 
    //example: <name>.app/Contents/MacOS/<name>
    #if __APPLE__
      addSysPath(KnownPaths::CurrentApplicationFile.getParent().getParent().getParent().getParent().toString());
    #else
      addSysPath(KnownPaths::CurrentApplicationFile.getParent().toString());
    #endif
  }

  #if defined(VISUS_PYTHON_SYS_PATH)
    addSysPath(VISUS_PYTHON_SYS_PATH);
  #endif

    if (bVerbose)
    VisusInfo() << "Trying to import visuspy...";

  execCode("from visuspy import *");

  if (bVerbose)
    VisusInfo() << "...imported visuspy";

  swig_type_aborted = SWIG_TypeQuery("Visus::Aborted *");
  VisusAssert(swig_type_aborted);

  swig_type_array = SWIG_TypeQuery("Visus::Array *");
  VisusAssert(swig_type_array);

  //swig_type_object_type_info = SWIG_TypeQuery("Visus::SharedPtr< Visus::Object > *");
  //VisusAssert(object_type_info);
}


///////////////////////////////////////////////////////////////////////////
PythonEngine::~PythonEngine()
{
  ScopedAcquireGil acquire_gil;

  if (__redirect_output__) {
    execCode("__redirect_output__.restoreOldIO()");
    delModuleAttr("__redirect_output__");
    __redirect_output__ = nullptr;
  }

  // Delete the module from sys.modules
  PyObject* modules = PyImport_GetModuleDict();
  PyDict_DelItemString(modules, module_name.c_str());
}

///////////////////////////////////////////////////////////////////////////
void PythonEngine::setModuleAttr(String name, PyObject* value) {
  PyDict_SetItemString(globals, name.c_str(), value);
}


///////////////////////////////////////////////////////////////////////////
PyObject* PythonEngine::getModuleAttr(String name) {
  return PyDict_GetItemString(globals, name.c_str()); //borrowed
}

///////////////////////////////////////////////////////////////////////////
bool PythonEngine::hasModuleAttr(String name) {
  return getModuleAttr(name) ? true : false;
}

///////////////////////////////////////////////////////////////////////////
void PythonEngine::delModuleAttr(String name) {
  if (hasModuleAttr(name))
    PyDict_DelItemString(globals, name.c_str());
}

///////////////////////////////////////////////////////////////////////////
PyObject* PythonEngine::newPyFunction(PyObject* self, String name, Function fn)
{
  //see http://code.activestate.com/recipes/54352-defining-python-class-methods-in-c/
  //see http://bannalia.blogspot.it/2016/07/passing-capturing-c-lambda-functions-as.html
  //see https://stackoverflow.com/questions/26716711/documentation-for-pycfunction-new-pycfunction-newex

  class PyCapsuleInfo
  {
  public:
    UniquePtr<PyMethodDef> mdef;
    Function               fn;
    PyObject*              self = nullptr;
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

///////////////////////////////////////////////////////////////////////////
void PythonEngine::addModuleFunction(String name, Function fn)
{
  auto py_fn = newPyFunction(/*self*/nullptr, name, fn);
  setModuleAttr(name, py_fn);
  Py_DECREF(py_fn);
}

///////////////////////////////////////////////////////////////////////////
void PythonEngine::addObjectMethod(PyObject* self, String name, Function fn)
{
  auto py_fn = newPyFunction(self, name, fn);
  auto py_name = PyString_FromString(name.c_str());
  PyObject_SetAttr(self, py_name, py_fn);
  Py_DECREF(py_fn);
  Py_XDECREF(py_name);
}

///////////////////////////////////////////////////////////////////////////
PyObject* PythonEngine::newPyObject(double value) {
  return PyFloat_FromDouble(value);;
}

///////////////////////////////////////////////////////////////////////////
PyObject* PythonEngine::newPyObject(int value) {
  return PyLong_FromLong(value);
}

///////////////////////////////////////////////////////////////////////////
PyObject* PythonEngine::newPyObject(String s) {
  return PyString_FromString(s.c_str());
}

///////////////////////////////////////////////////////////////////////////
PyObject* PythonEngine::newPyObject(Aborted value) {
  return newPyObject<Aborted>(value, swig_type_aborted);
}

///////////////////////////////////////////////////////////////////////////
PyObject* PythonEngine::newPyObject(Array value) {
  return newPyObject<Array>(value, swig_type_array);
}

///////////////////////////////////////////////////////////////////////////
Aborted PythonEngine::getModuleAbortedAttr(String name) {
  return getSwigModuleAttr<Aborted>(name, swig_type_aborted);
}

///////////////////////////////////////////////////////////////////////////
Array PythonEngine::getModuleArrayAttr(String name) {
  return getSwigModuleAttr<Array>(name, swig_type_array);
}

///////////////////////////////////////////////////////////////////////////
Array PythonEngine::toArray(PyObject* py_object) {

  Array* ptr = nullptr;
  int res = SWIG_ConvertPtr(py_object, (void**)&ptr, swig_type_array, 0);

  if (!SWIG_IsOK(res) || !ptr)
    ThrowException(StringUtils::format() << "cannot convert to array");

  Array ret = (*ptr);

  if (SWIG_IsNewObj(res))
    delete ptr;

  return ret;
}


///////////////////////////////////////////////////////////////////////////
void PythonEngine::redirectOutputTo(std::function<void(String)> value)
{
  ScopedAcquireGil acquire_gil;

  execCode(
    "import sys\n"
    "class RedirectOutput :\n"
    "  def __init__(self) :\n"
    "    self.stdout,self.stderr = (sys.stdout,sys.stderr)\n"
    "    sys.stdout,sys.stderr   = (self,self)\n"
    "  def __del__(self) :\n"
    "    self.restoreOldIO()\n"
    "  def restoreOldIO(self): \n"
    "    sys.stdout,sys.stderr = (self.stdout,self.stderr)\n"
    "  def write(self, msg) :\n"
    "   self.internalWrite(msg)\n"
    "  def flush(self) :\n"
    "    pass\n"
    "__redirect_output__=RedirectOutput()\n"
  );

  __redirect_output__ = getModuleAttr("__redirect_output__");
  VisusAssert(__redirect_output__);

  addObjectMethod(__redirect_output__, "internalWrite", [this, value](PyObject*, PyObject* args)
  {
    VisusAssert(PyTuple_Check(args));
    for (int I = 0, N = (int)PyTuple_Size(args); I < N; I++) {
      auto obj = PyTuple_GetItem(args, I);
      auto s = PythonEngine::convertToString(obj);
      value(s);
    }
    auto ret = Py_None;
    incrRef(ret);
    return ret;
  });

}

///////////////////////////////////////////////////////////////////////////
String PythonEngine::convertToString(PyObject* value)
{
  if (!value) 
    return "";
  
  PyObject *py_str = PyObject_Str(value);
  auto tmp = SWIG_Python_str_AsChar(py_str);
  String ret = tmp ? tmp : "";
  SWIG_Python_str_DelForPy3(tmp);
  Py_DECREF(py_str);
  return ret;
}

///////////////////////////////////////////////////////////////////////////
String PythonEngine::getLastErrorMessage()
{
  PyObject *type = nullptr, *value = nullptr, *traceback = nullptr;
  PyErr_Fetch(&type, &value, &traceback);
  PyErr_Clear();

  std::ostringstream out;
  out<<"Python error: "<< convertToString(type)<<" "<<convertToString(value);
  return out.str();
}

///////////////////////////////////////////////////////////////////////////
void PythonEngine::execCode(String s)
{
  auto obj = PyRun_StringFlags(s.c_str(), Py_file_input, globals, globals, nullptr);

  if (bool bMaybeError=(obj == nullptr))
  {
    if (PyErr_Occurred())
    {
      auto  error_msg = getLastErrorMessage();
      ThrowException(error_msg);
    }
  }

  Py_DECREF(obj);

#if PY_MAJOR_VERSION < 3
  if (Py_FlushLine())
    PyErr_Clear();
#endif
};




///////////////////////////////////////////////////////////////////////////
PyObject* PythonEngine::evalCode(String s)
{
  //see https://bugs.python.org/issue405837
  //Return value: New reference.
  auto obj = PyRun_StringFlags(s.c_str(), Py_eval_input, globals, globals, nullptr);

  if (bool bMaybeError = (obj == nullptr))
  {
    if (PyErr_Occurred())
    {
      auto  error_msg = getLastErrorMessage();
      ThrowException(error_msg);
    }
  }

#if PY_MAJOR_VERSION < 3
  if (Py_FlushLine())
    PyErr_Clear();
#endif

  return obj;
};


} //namespace Visus

