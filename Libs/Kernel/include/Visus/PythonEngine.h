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

#include <Visus/Object.h>
#include <Visus/Array.h>
#include <Visus/Log.h>
#include <Visus/StringUtils.h>

#ifdef WIN32
#pragma warning( push )
#pragma warning (disable:4996)
#endif

#pragma push_macro("slots")
#undef slots

#ifndef SWIG_FILE_WITH_INIT

  #if defined(_DEBUG) && defined(SWIG_PYTHON_INTERPRETER_NO_DEBUG)
  /* Use debug wrappers with the Python release dll */
  # undef _DEBUG
  # include <Python.h>
  # define _DEBUG
  #else
  # include <Python.h>
  #endif

  #include <Visus/swigpyrun.h>
#endif

#pragma pop_macro("slots")

#include <functional>

#ifdef WIN32
#pragma warning( pop )
#endif


namespace Visus {


///////////////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API PythonEngine
{
public:

  VISUS_CLASS(PythonEngine)

public:

  static PyThreadState* mainThreadState;

  typedef std::function<PyObject*(PyObject*, PyObject*)> Function;

  //constructor
  PythonEngine(bool bVerbose=false);

  //destructor
  virtual ~PythonEngine();

  //redirectOutputTo
  void redirectOutputTo(std::function<void(String)> value);

  //isGoodVariableName
  static bool isGoodVariableName(String name);

  //incrRef
  static void incrRef(PyObject* value);

  //decrRef
  static void decrRef(PyObject* value);

  //execCode
  void execCode(String s);

  //evalCode
  PyObject* evalCode(String s);

  //newPyObject
  PyObject* newPyObject(double value);

  //newPyObject
  PyObject* newPyObject(int value);

  //newPyObject
  PyObject* newPyObject(String s);

  //newPyObject
  PyObject* newPyObject(Aborted value);

  //newPyObject
  PyObject* newPyObject(Array value);

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
  void setModuleAttr(String name, PyObject* value);

  //setModuleAttr
  template <typename Value>
  void setModuleAttr(String name, Value value) {
    auto obj = newPyObject(value);
    setModuleAttr(name, obj);
    decrRef(obj);
  }

  //getModuleAttr
  PyObject* getModuleAttr(String name);

  //hasModuleAttr
  bool hasModuleAttr(String name);

  //delModuleAttr
  void delModuleAttr(String name);

  //newPyFunction
  PyObject* newPyFunction(PyObject* self, String name, Function fn);

  //addModuleFunction
  void addModuleFunction(String name,Function fn);

  //addObjectMethod
  void addObjectMethod(PyObject* self, String name, Function fn);

  //getModuleAbortedAttr
  Aborted getModuleAbortedAttr(String name);

  //getModuleArrayAttr
  Array getModuleArrayAttr(String name);

  //toArray
  Array toArray(PyObject* py_object);

  //getLastErrorMessage
  static String getLastErrorMessage();

  //convertToString
  static String convertToString(PyObject* value);

  //addSysPath
  void addSysPath(String value);

  //fixPath
  static String fixPath(String value);

private:

  String  module_name;

  PyObject* module = nullptr;
  PyObject* globals = nullptr;

  swig_type_info* swig_type_aborted = nullptr;
  swig_type_info* swig_type_array = nullptr;

  PyObject* __redirect_output__ = nullptr;

  //generateModuleName
  static std::atomic<int>& ModuleId() {
    static std::atomic<int> ret;
    return ret;
  }

  //newPyObject
  template <typename ValueClass>
  PyObject* newPyObject(ValueClass value, swig_type_info* type_info) {
    return SWIG_NewPointerObj(new ValueClass(value), type_info, SWIG_POINTER_OWN);
  }

  //getSwigModuleAttr
  template <typename ReturnClass>
  ReturnClass getSwigModuleAttr(String name, swig_type_info* type_info)
  {
    auto py_object = getModuleAttr(name);
    if (!py_object)
      ThrowException(StringUtils::format() << "cannot find '" << name << "' in module");

    ReturnClass* ptr = nullptr;
    int res = SWIG_ConvertPtr(py_object, (void**)&ptr, type_info, 0);

    if (!SWIG_IsOK(res) || !ptr)
      ThrowException(StringUtils::format() << "cannot case '" << name << "' to " << type_info->name);

    ReturnClass ret = (*ptr);

    if (SWIG_IsNewObj(res))
      delete ptr;

    return ret;
  }

};


/////////////////////////////////////////////////////
class VISUS_KERNEL_API ScopedAcquireGil
{
public:

  VISUS_CLASS(ScopedAcquireGil)

  PyGILState_STATE state;

  //constructor
  ScopedAcquireGil();

  //destructor
  ~ScopedAcquireGil();
};

typedef ScopedAcquireGil PythonThreadBlock;

/////////////////////////////////////////////////////
class VISUS_KERNEL_API ScopedReleaseGil {
  
public:

  PyThreadState* state=nullptr;

  //constructor
  ScopedReleaseGil();
  
  //destructor
  ~ScopedReleaseGil();
};

typedef ScopedReleaseGil PythonThreadAllow;

} //namespace Visus

#endif //_VISUS_PYTHON_ENGINE_H__

