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

#include <Visus/Viewer.h>
#include <Visus/ApplicationInfo.h>
#include <Visus/ModVisus.h>
#include <Visus/Path.h>

#if VISUS_PYTHON
#include <Visus/Python.h>
#endif

////////////////////////////////////////////////////////////////////////
int main(int argn,const char* argv[])
{
  using namespace Visus;
  SetCommandLine(argn, argv);
  GuiModule::attach();

  #if VISUS_PYTHON
  #if PY_MINOR_VERSION<=4
  Py_SetProgramName(_Py_char2wchar((char*)argv[0], NULL));
  #else
  Py_SetProgramName(Py_DecodeLocale((char*)argv[0], NULL));
  #endif

  Py_InitializeEx(0);
  PyEval_InitThreads();
  auto python_thread_state = PyEval_SaveThread();
  {
    ScopedAcquireGil acquire_gil;
    String cmd = StringUtils::replaceAll(R"EOF(
import os,sys;
sys.path.append(os.path.realpath('${bin_dir}/../..'))
from OpenVisus import *
from OpenVisus.gui import *
)EOF", "${bin_dir}", KnownPaths::BinaryDirectory.toString());
    PyRun_SimpleString(cmd.c_str());
  }
  #endif

  {
    auto viewer=std::make_shared<Viewer>();
    auto args = std::vector<String>(ApplicationInfo::args.begin() + 1, ApplicationInfo::args.end());
    viewer->configureFromArgs(args);
    QApplication::exec();
  }

#if VISUS_PYTHON
  PyEval_RestoreThread(python_thread_state);
  Py_Finalize();
#endif

  GuiModule::detach();
  return 0;
}


