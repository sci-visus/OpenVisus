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

#if VISUS_PYTHON

#include <Visus/Python.h>
#include <Visus/Thread.h>
#include <Visus/Path.h>
#include <Visus/File.h>

#include <cctype>

#include <pydebug.h>
#include <frameobject.h>


//see SWIG_TYPE_TABLE necessary to share type info
#include <Visus/swigpyrun.h>

#if PY_MAJOR_VERSION <3
  #define CharToWideChar(arg) ((char*)arg)
#else

  static wchar_t* CharToWideChar(const char* value) 
  {
  #if PY_MINOR_VERSION<=4
    return _Py_char2wchar((char*)value, NULL);
  #else
    return Py_DecodeLocale((char*)value, NULL);
  #endif
  }

#endif

namespace Visus {



///////////////////////////////////////////////////////////////////////////
ScopedAcquireGil::ScopedAcquireGil() 
{
  this->state = new PyGILState_STATE();
  *this->state = PyGILState_Ensure();
}


///////////////////////////////////////////////////////////////////////////
ScopedAcquireGil::~ScopedAcquireGil()
{
  if (this->state)
  {
    PyGILState_Release(*state);
    delete state;
  }
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
void PythonEngine::setVerboseFlag(int value)
{
  Py_VerboseFlag = value;
}

static std::atomic<int> module_id(0);

  ///////////////////////////////////////////////////////////////////////////
PythonEngine::PythonEngine(bool bVerbose) 
{
  this->module_name = concatenate("__PythonEngine__",++module_id);
  //PrintInfo("Creating PythonEngine",module_name,"...");

  ScopedAcquireGil acquire_gil;
  this->module  = PyImport_AddModule(module_name.c_str()); 
  VisusReleaseAssert(this->module);

  this->globals = PyModule_GetDict(module); //borrowed

  auto builtins = PyEval_GetBuiltins(); VisusAssert(builtins);
  PyDict_SetItemString(getGlobals(), "__builtins__", builtins);

	if (bVerbose)
    PrintInfo("Trying to import OpenVisus...");

  execCode("from OpenVisus import *");

  if (bVerbose)
    PrintInfo("...imported OpenVisus");
}


///////////////////////////////////////////////////////////////////////////
PythonEngine::~PythonEngine()
{
  //PrintInfo("Destroying PythonEngine",this->module_name,"...");

  // Delete the module from sys.modules
  {
    ScopedAcquireGil acquire_gil;
    PyObject* modules = PyImport_GetModuleDict();
    PyDict_DelItemString(modules, module_name.c_str());
  }
}

///////////////////////////////////////////////////////////////////////////
void PythonEngine::setProgramName(String value)
{
  Py_SetProgramName(CharToWideChar(value.c_str()));
}

///////////////////////////////////////////////////////////////////////////
PyObject* PythonEngine::getAttr(PyObject* self, String name) {
  return PyDict_GetItemString(self, name.c_str()); //borrowed
}

///////////////////////////////////////////////////////////////////////////
void PythonEngine::setAttr(PyObject* self, String name, PyObject* value) {
  PyDict_SetItemString(self, name.c_str(), value);
}

///////////////////////////////////////////////////////////////////////////
bool PythonEngine::hasAttr(PyObject* self, String name) {
  return getAttr(self, name) ? true : false;
}

///////////////////////////////////////////////////////////////////////////
void PythonEngine::delAttr(PyObject* self, String name) {

  if (!hasAttr(self, name)) return;
  PyDict_DelItemString(self, name.c_str());
}

///////////////////////////////////////////////////////////////////////////
void PythonEngine::setError(String explanation)
{
  PyErr_SetString(PyExc_SystemError, explanation.c_str());
}

///////////////////////////////////////////////////////////////////////////
PyObject* PythonEngine::wrapFunction(PyObject* self, String name, Function fn, PyObject* module)
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

  auto ret = PyCFunction_NewEx(/*method definition*/info->mdef.get(), /*self*/py_capsule, module);
  Py_DECREF(py_capsule);
  return ret;
}

///////////////////////////////////////////////////////////////////////////
void PythonEngine::addModuleFunction(String name, Function fn)
{
  auto wrapped = wrapFunction(nullptr, name, fn,this->module);
  setGlobalAttr(name, wrapped);
  Py_DECREF(wrapped);
}

///////////////////////////////////////////////////////////////////////////
void PythonEngine::addObjectMethod(PyObject* self, String name, Function fn)
{
  auto py_fn = wrapFunction(self, name, fn, nullptr);
  auto py_name = PyString_FromString(name.c_str());
  PyObject_SetAttr(self, py_name, py_fn);
  Py_DECREF(py_fn);
  Py_XDECREF(py_name);
}

///////////////////////////////////////////////////////////////////////////
PyObject* PythonEngine::wrapDouble(double value) {
  return PyFloat_FromDouble(value);;
}

///////////////////////////////////////////////////////////////////////////
double PythonEngine::unwrapDouble(PyObject* obj) {
  return PyFloat_AsDouble(obj);;
}

///////////////////////////////////////////////////////////////////////////
PyObject* PythonEngine::wrapInt(int value) {
  return PyLong_FromLong(value);
}

///////////////////////////////////////////////////////////////////////////
int PythonEngine::unwrapInt(PyObject* obj) {
  return PyLong_AsLong(obj);;
}

///////////////////////////////////////////////////////////////////////////
PyObject* PythonEngine::wrapString(String s) {
  return PyString_FromString(s.c_str());
}

///////////////////////////////////////////////////////////////////////////
String PythonEngine::unwrapString(PyObject* value)
{
  if (!value) return "";
  PyObject* py_str = PyObject_Str(value);
  auto tmp = SWIG_Python_str_AsChar(py_str);
  String ret = tmp ? tmp : "";
  SWIG_Python_str_DelForPy3(tmp);
  Py_DECREF(py_str);
  return ret;
}

///////////////////////////////////////////////////////////////////////////
PyObject* PythonEngine::wrapAborted(Aborted value) 
{
  auto typeinfo = SWIG_TypeQuery("Visus::Aborted *");
  VisusAssert(typeinfo);
  Aborted* ptr = new Aborted(value);
  return SWIG_NewPointerObj(ptr, typeinfo, SWIG_POINTER_OWN);
}


///////////////////////////////////////////////////////////////////////////
Aborted PythonEngine::unwrapAborted(PyObject* py_object) 
{
  auto typeinfo = SWIG_TypeQuery("Visus::Aborted *");
  VisusAssert(typeinfo);

  Aborted* ptr = nullptr;
  int res = SWIG_ConvertPtr(py_object, (void**)&ptr, typeinfo, 0);

  if (!SWIG_IsOK(res) || !ptr)
    ThrowException("cannot convert to aborted");

  Aborted ret = *ptr;

  if (SWIG_IsNewObj(res))
    delete ptr;

  return ret;
}

///////////////////////////////////////////////////////////////////////////
PyObject* PythonEngine::wrapArray(Array value) 
{
  auto typeinfo = SWIG_TypeQuery("Visus::Array *");
  VisusAssert(typeinfo);
  auto ptr = new Array(value);
  return SWIG_NewPointerObj(ptr, typeinfo, SWIG_POINTER_OWN);
}


///////////////////////////////////////////////////////////////////////////
Array PythonEngine::unwrapArray(PyObject* py_object) 
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

static int tb_displayline(PyObject* f, PyObject* filename, int lineno, PyObject* name)
{
  int err;
  PyObject* line;

  if (filename == NULL || name == NULL)
    return -1;

  line = PyUnicode_FromFormat("  File \"%U\", line %d, in %U\n",
    filename, lineno, name);

  if (line == NULL)
    return -1;

  err = PyFile_WriteObject(line, f, Py_PRINT_RAW);
  Py_DECREF(line);

  if (err != 0)
    return err;

  /* ignore errors since we can't report them, can we? */
  if (_Py_DisplaySourceLine(f, filename, lineno, 4))
    PyErr_Clear();

  return err;
}

static int tb_printinternal(PyTracebackObject* tb, PyObject* f, long limit)
{
  int err = 0;
  long depth = 0;
  PyTracebackObject* tb1 = tb;
  while (tb1 != NULL) {
    depth++;
    tb1 = tb1->tb_next;
  }
  while (tb != NULL && err == 0) 
  {
    if (depth <= limit) {
      err = tb_displayline(f, tb->tb_frame->f_code->co_filename, tb->tb_lineno, tb->tb_frame->f_code->co_name);
    }
    depth--;
    tb = tb->tb_next;
    if (err == 0)
      err = PyErr_CheckSignals();
  }
  return err;
}


///////////////////////////////////////////////////////////////////////////
String GetPythonErrorMessage()
{
  //see http://www.solutionscan.org/154789-python
  auto err = PyErr_Occurred();
  if (!err)
    return "";

  //PyErr_Print();
  
  //TODO: the following does not provice the same infos as PyErr_Print
#if 1
  PyObject *type, *value, *traceback;
  PyErr_Fetch(&type, &value, &traceback);

  std::ostringstream out;

  out << "Python error: " 
    << PythonEngine::unwrapString(type) << " "
    << PythonEngine::unwrapString(value) << " "<<std::endl;

  auto module_name = PyString_FromString("traceback");
  auto module = PyImport_Import(module_name);
  Py_DECREF(module_name);

  auto fn = module? PyObject_GetAttrString(module, "format_exception") : nullptr;
  if (fn && PyCallable_Check(fn))
  {
    if (auto lines = PyObject_CallFunctionObjArgs(fn, type, value, traceback, NULL))
    {
      for (int i=0; i < PyList_GET_SIZE(lines); i++) 
      {
        auto line = PyList_GET_ITEM(lines, i);
        if (line != NULL)
          out<<PythonEngine::unwrapString(line);
      }

      Py_DECREF(lines);
    }
  }
#endif

  return out.str();
}

///////////////////////////////////////////////////////////////////////////
void PythonEngine::printMessage(String message)
{
  PySys_WriteStdout("%s", message.c_str());
}

///////////////////////////////////////////////////////////////////////////
void PythonEngine::execCode(String s)
{
  auto obj = PyRun_StringFlags(s.c_str(), Py_file_input, getGlobals(), getGlobals(), nullptr);
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

#if PY_MAJOR_VERSION < 3
  if (Py_FlushLine())  PyErr_Clear();
#endif
};


///////////////////////////////////////////////////////////////////////////
PyObject* PythonEngine::evalCode(String s)
{
  //see https://bugs.python.org/issue405837
  //Return value: New reference.
  auto obj = PyRun_StringFlags(s.c_str(), Py_eval_input, getGlobals(), getGlobals(), nullptr);

  if (bool bMaybeError = (obj == nullptr))
  {
    if (PyErr_Occurred())
    {
      String error_msg = cstring("Python error code:\n", s,"\nError:\n", GetPythonErrorMessage());
      PyErr_Clear();
      PrintInfo(error_msg);
      ThrowException(error_msg);
    }
  }

#if PY_MAJOR_VERSION < 3
  if (Py_FlushLine())  PyErr_Clear();
#endif

  return obj;
};


} //namespace Visus

#endif //VISUS_PYTHON
