#/*-----------------------------------------------------------------------------
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

#ifndef VISUS_PYTHON_H__
#define VISUS_PYTHON_H__


#pragma push_macro("slots")
#undef slots

#if defined(WIN32) && defined(_DEBUG) 
#undef _DEBUG
#include <Python.h>
#define _DEBUG 1
#else
#include <Python.h>
#endif

#pragma pop_macro("slots")

#include <string>
#include <sstream>
#include <vector>

inline PyThreadState*& __python_thread_state__() {
  static PyThreadState* ret = nullptr;
  return ret;
}

inline void InitEmbeddedPython(int argn, const char* argv[], std::string sys_path = "", std::vector<std::string> commands = {})
{
  #if PY_VERSION_HEX >= 0x03050000
  Py_SetProgramName(Py_DecodeLocale((char*)argv[0], NULL));
  #else
  Py_SetProgramName(_Py_char2wchar((char*)argv[0], NULL));
  #endif
  
  Py_InitializeEx(0);
  PyEval_InitThreads();
  __python_thread_state__() = PyEval_SaveThread();
  auto acquire_gil = PyGILState_Ensure();

  std::ostringstream out;
  out << "import os, sys;\n";

  if (!sys_path.empty())
    out << "sys.path.append(os.path.realpath('" + sys_path + "'))\n";

  for (auto cmd : commands)
    out << cmd << "\n";

  PyRun_SimpleString(out.str().c_str());
  PyGILState_Release(acquire_gil);
}

inline void ShutdownEmbeddedPython()
{
  PyEval_RestoreThread(__python_thread_state__());
  Py_Finalize();
}


#endif //VISUS_PYTHON_H__

