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

#if VISUS_PYTHON

#include <Visus/Array.h>
#include <Visus/StringUtils.h>

#include <queue>
#include <functional>

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

namespace Visus {

///////////////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API PythonEngine
{
public:

  VISUS_CLASS(PythonEngine)

  typedef std::function<PyObject*(PyObject*, PyObject*)> Function;

  //constructor
  PythonEngine(bool bVerbose=false);

  //destructor
  virtual ~PythonEngine();

public:

  //getAttr (borrowed)
  PyObject* getAttr(PyObject* self, String name);

  //hasAttr
  bool hasAttr(PyObject* self, String name);

  //setAttr
  void setAttr(PyObject* self, String name, PyObject* value);

  //delAttr
  void delAttr(PyObject* self, String name);

public:

  //getGlobals
  PyObject* getGlobals() {
    return globals;
  }

  //getGlobalAttr
  PyObject* getGlobalAttr(String name) {
    return getAttr(getGlobals(), name.c_str());
  }

  //setGlobalAttr
  void setGlobalAttr(String name, PyObject* value) {
    setAttr(getGlobals(), name.c_str(), value);
  }

  //hasGlobalAttr
  bool hasGlobalAttr(String name) {
    return hasAttr(getGlobals(), name) ? true : false;
  }

  //delGlobalAttr
  void delGlobalAttr(String name) {
    delAttr(getGlobals(), name);
  }

public:

  //wrapDouble
  PyObject* wrapDouble(double value);

  //unwrapDouble
  double unwrapDouble(PyObject* obj);

  //wrapInt
  PyObject* wrapInt(int value);

  //unwrapInt
  int unwrapInt(PyObject* obj);

  //wrapString
  PyObject* wrapString(String s);

  //unwrapString
  static String unwrapString(PyObject* value);

  //wrapAborted
  PyObject* wrapAborted(Aborted value);

  //unwrapAborted
  Aborted unwrapAborted(PyObject* py_object);

  //wrapArray
  PyObject* wrapArray(Array value);

  //unwrapArray
  Array unwrapArray(PyObject* py_object);

  //internalNewPyFunction
  PyObject* wrapFunction(PyObject* self, String name, Function fn, PyObject* module);

  //addModuleFunction
  void addModuleFunction(String name,Function fn);

  //addObjectMethod
  void addObjectMethod(PyObject* self, String name, Function fn);

public:

  //addSysPath
  void addSysPath(String value,bool bVerbose=true);

  //printMessage (must have the GIL)
  void printMessage(String message);

  //setError 
  void setError(String explanation);

  //execCode
  void execCode(String s);

  //evalCode
  PyObject* evalCode(String s);

  //main
  static int main(std::vector<String> args);

private:

  String     module_name;
  PyObject*  module = nullptr;
  PyObject*  globals = nullptr;

  //getLastPythonErrorMessage
  String getLastPythonErrorMessage(bool bClear);

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

/////////////////////////////////////////////////////
class VISUS_KERNEL_API ScopedReleaseGil {

  PyThreadState* state = nullptr;

public:

  //constructor
  ScopedReleaseGil();
  
  //destructor
  ~ScopedReleaseGil();
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

#endif //VISUS_PYTHON

#endif //_VISUS_PYTHON_ENGINE_H__
