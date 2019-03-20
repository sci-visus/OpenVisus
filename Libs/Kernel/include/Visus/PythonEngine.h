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

# if VISUS_PYTHON

#include <Visus/Array.h>
#include <Visus/Log.h>
#include <Visus/StringUtils.h>

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
//for windows using Release anyway (otherwise most site-packages don't work)
# undef _DEBUG
# include <Python.h>
# define _DEBUG
#else
# include <Python.h>
#endif

#pragma pop_macro("slots")

namespace Visus {

///////////////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API PythonEngine
{
public:

  VISUS_CLASS(PythonEngine)

public:

  typedef std::function<PyObject*(PyObject*, PyObject*)> Function;

  //constructor
  PythonEngine(bool bVerbose=false);

  //destructor
  virtual ~PythonEngine();

  //setMainThread
  static void setMainThread();

  //main
  static int main(std::vector<String> args);

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

  //setError (to call when you return nullpptr in Function)
  static void setError(String explanation, PyObject* err= nullptr);

  //addModuleFunction
  void addModuleFunction(String name,Function fn);

  //addObjectMethod
  void addObjectMethod(PyObject* self, String name, Function fn);

  //getModuleAbortedAttr
  Aborted getModuleAbortedAttr(String name);

  //getModuleArrayAttr
  Array getModuleArrayAttr(String name);

  //pythonObjectToArray
  Array pythonObjectToArray(PyObject* py_object);

  //getLastErrorMessage
  static String getLastErrorMessage();

  //convertToString
  static String convertToString(PyObject* value);

  //addSysPath
  void addSysPath(String value,bool bVerbose=true);

  //printMessage (must have the GIL)
  void printMessage(String message);

private:

  String  module_name;

  PyObject* module = nullptr;
  PyObject* globals = nullptr;

  //fixPath
  static String fixPath(String value);

  //internalNewPyFunction
  PyObject* internalNewPyFunction(PyObject* self, String name, Function fn);

};


/////////////////////////////////////////////////////
class VISUS_KERNEL_API ScopedAcquireGil
{
  PyGILState_STATE* state = nullptr;

public:

  VISUS_CLASS(ScopedAcquireGil)

  //constructor
  ScopedAcquireGil();

  //destructor
  ~ScopedAcquireGil();
};

typedef ScopedAcquireGil PythonThreadBlock;

/////////////////////////////////////////////////////
class VISUS_KERNEL_API ScopedReleaseGil {

  PyThreadState* state = nullptr;

public:

  //constructor
  ScopedReleaseGil();
  
  //destructor
  ~ScopedReleaseGil();
};

typedef ScopedReleaseGil PythonThreadAllow;


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

#endif //# if VISUS_PYTHON

#endif //_VISUS_PYTHON_ENGINE_H__
