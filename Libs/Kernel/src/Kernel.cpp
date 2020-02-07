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

#include <Visus/Kernel.h>

#include <Visus/Thread.h>
#include <Visus/NetService.h>
#include <Visus/RamResource.h>
#include <Visus/Path.h>
#include <Visus/File.h>
#include <Visus/Encoder.h>
#include <Visus/UUID.h>
#include <Visus/StringTree.h>
#include <Visus/NetService.h>
#include <Visus/SharedLibrary.h>
#include <Visus/Python.h>

#include <assert.h>
#include <type_traits>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <atomic>
#include <clocale>
#include <cctype>

#if WIN32
#  pragma warning(disable:4996)
#  include <Windows.h>
#  include <ShlObj.h>
#  include <winsock2.h>
#else
#  ifndef _GNU_SOURCE
#    define _GNU_SOURCE
#  endif
#  include <dlfcn.h>
#  include <signal.h>
#  include <pwd.h>
#  include <sys/socket.h>
#  include <unistd.h>
#  if __APPLE__
#    include <mach/mach.h>
#    include <mach/mach_host.h>
#    include <mach-o/dyld.h>
#  else
#    include <sys/sysinfo.h>
#  endif
#endif

#include <Visus/Frustum.h>
#include <Visus/Graph.h>

#include <Visus/KdArray.h>
#include <Visus/TransferFunction.h>
#include <Visus/Statistics.h>
#include <Visus/Array.h>

#include <clocale>

//this solve a problem of old Linux distribution (like Centos 5)
#if __GNUC__ && !__APPLE__
	#include <arpa/inet.h>
	#include <byteswap.h>

	#ifndef htole32
		extern "C" uint32_t htole32(uint32_t x) { 
		  return bswap_32(htonl(x));
		}
	#endif
#endif


namespace Visus {

static PyThreadState* __main__thread_state__ = nullptr;

String VisusGetGitRevision()
{
  #ifdef GIT_REVISION
    #define __str__(s) #s
    #define __xstr__(s) __str__(s)
    return __xstr__(GIT_REVISION);
    #undef __str__
    #undef __xstr__
  #else
    return "";
  #endif
}


#if __APPLE__
  
//see Kernel.mm
String GetMainBundlePath();
  
void InitAutoReleasePool();
  
void DestroyAutoReleasePool();
  
#endif

ConfigFile* VisusModule::getModuleConfig() {
  return Private::VisusConfig::getSingleton();
}

//////////////////////////////////////////////////////////////////
void PrintMessageToTerminal(const String& value) {

#if WIN32
  OutputDebugStringA(value.c_str());
#endif

  std::cout << value;
}


//////////////////////////////////////////////////////////////////
static std::pair< void (*)(String msg, void*), void*> __redirect_log__;

void RedirectLogTo(void(*callback)(String msg, void*), void* user_data) {
  __redirect_log__ = std::make_pair(callback, user_data);
}

///////////////////////////////////////////////////////////////////////
void PrintLine(String file, int line, int level, String msg)
{
  auto t1 = Time::now();

  file = file.substr(file.find_last_of("/\\") + 1);
  file = file.substr(0, file.find_last_of('.'));

  std::ostringstream out;
  out << std::setfill('0')
    << std::setw(2) << t1.getHours()
    << std::setw(2) << t1.getMinutes()
    << std::setw(2) << t1.getSeconds()
    << std::setw(3) << t1.getMilliseconds()
    << " " << file << ":" << line
    << " " << Utils::getPid() << ":" << Thread::getThreadId()
    << " " << msg << std::endl;

  msg = out.str();
  PrintMessageToTerminal(msg);

  if (__redirect_log__.first)
    __redirect_log__.first(msg, __redirect_log__.second);
}


//check types
static_assert(sizeof(Int8) == 1 && sizeof(Uint8) == 1 && sizeof(char) == 1, "internal error");
static_assert(sizeof(Int16) == 2 && sizeof(Int16) == 2 && sizeof(short) == 2, "internal error");
static_assert(sizeof(Int32) == 4 && sizeof(Int32) == 4 && sizeof(int) == 4, "internal error");
static_assert(sizeof(Int64) == 8 && sizeof(Uint64) == 8, "internal error");
static_assert(sizeof(Float32) == 4 && sizeof(float) == 4, "internal error");
static_assert(sizeof(Float64) == 8 && sizeof(double) == 8, "internal error");

#pragma pack(push)
#pragma pack(1) 
typedef struct { Int64 a[1];           Int8 b; }S009;
typedef struct { Int64 a[2];           Int8 b; }S017;
typedef struct { Int64 a[2]; Int32 b[1]; Int8 c; }S021;
typedef struct { Int64 a[4];           Int8 b; }S033;
typedef struct { Int64 a[4]; Int32 b[1]; Int8 c; }S037;
typedef struct { Int64 a[8]; Int8 b; }S065;
typedef struct { Int64 a[16]; Int8 b; }S129;
typedef struct { Int64 a[16]; Int32 b[1]; Int8 c; }S133;
#pragma pack(pop)

static_assert(sizeof(S009) == 9, "internal error");
static_assert(sizeof(S017) == 17, "internal error");
static_assert(sizeof(S021) == 21, "internal error");
static_assert(sizeof(S033) == 33, "internal error");
static_assert(sizeof(S037) == 37, "internal error");
static_assert(sizeof(S065) == 65, "internal error");
static_assert(sizeof(S129) == 129, "internal error");
static_assert(sizeof(S133) == 133, "internal error");

//check 64 bit file IO is enabled!
#if __GNUC__ && !__APPLE__
static_assert(sizeof(off_t) == 8, "internal error");
#endif

static int                      __argn__;
static std::vector<const char*> __argv__;
static std::vector<String>      __args__;

static String visus_config_commandline_filename;

///////////////////////////////////////////////////////////////////////////////
String cstring10(double value) {
  std::ostringstream out;
  out << std::setprecision(std::numeric_limits<double>::max_digits10) << value;
  return out.str();
}

///////////////////////////////////////////////////////////////////////////////
void VisusSetCommandLine(int argn, const char** argv)
{
  __argn__ = 0;
  __argv__.clear();
  __args__.clear();

  for (int I = 0; I < argn; I++)
  {
    auto arg = argv[I];

    //override visus.config
    if (arg == String("--visus-config") && I < (argn - 1))
    {
      visus_config_commandline_filename = argv[++I];
      continue;
    }

    //xcode debugger always passes this; just ignore it
    if (arg == String("-NSDocumentRevisionsDebugMode") && I < (argn - 1))
    {
      String ignoring_enabled = argv[++I];
      continue;
    }

    if (StringUtils::startsWith(arg, "-v")) {
      int value = (int)String(arg).size() - 1;
      PythonEngine::setVerboseFlag(value);
      continue;
    }


    __argn__++;
    __argv__.push_back(arg);
    __args__.push_back(arg);
  }
}


///////////////////////////////////////////////////////////////////////////////
std::vector<String> VisusGetCommandLine()
{
  if (__args__.empty())
    return {"__main__.py"};
  else
    return __args__;
}

/////////////////////////////////////////////////////
bool VisusHasMessageLock()
{
  return Thread::isMainThread();
}


//////////////////////////////////////////////////////
void VisusAssertFailed(const char* file,int line,const char* expr)
{
#ifdef _DEBUG
    Utils::breakInDebugger();
#else
    ThrowExceptionEx(file,line,expr);
#endif
}

String cnamed(String name, String value) {
  return name + "(" + value + ")";
}


//////////////////////////////////////////////////////
void ThrowExceptionEx(String file,int line, String what)
{
  String msg = cstring("Visus throwing exception", cnamed("where", file + ":" + cstring(line)), cnamed("what", what));
  PrintInfo(msg);
  throw std::runtime_error(msg);
}

#if WIN32
static void __do_not_remove_my_function__() {
}
#else
VISUS_SHARED_EXPORT void __do_not_remove_my_function__() {
}
#endif


///////////////////////////////////////////////////////////
static void InitKnownPaths()
{
  //VisusHome
  {
    // Allow override of VisusHome
    if (auto VISUS_HOME = getenv("VISUS_HOME"))
    {
      KnownPaths::VisusHome = Path(String(VISUS_HOME));
    }
    else
    {
      #if WIN32
      {
        char buff[2048]; memset(buff, 0, sizeof(buff));
        SHGetSpecialFolderPath(0, buff, CSIDL_PERSONAL, FALSE);
        KnownPaths::VisusHome = Path(buff).getChild("visus");
      }
      #else
      {
				if (auto homedir = getenv("HOME"))
          KnownPaths::VisusHome = Path(homedir).getChild("visus");

        else if (auto pw = getpwuid(getuid()))
          KnownPaths::VisusHome = Path(pw->pw_dir).getChild("visus");
      }
      #endif
    }
  }

  FileUtils::createDirectory(KnownPaths::VisusHome);

  //Current application file (i.e. where VisusKernel shared library is)
  {
    #if WIN32
    {
      //see https://stackoverflow.com/questions/6924195/get-dll-path-at-runtime
      HMODULE handle;
      GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)__do_not_remove_my_function__, &handle);
      VisusReleaseAssert(handle);
      char buff[2048]; memset(buff, 0, sizeof(buff));
      GetModuleFileName(handle, buff, sizeof(buff));
      KnownPaths::BinaryDirectory = Path(buff).getParent();
    }
    #else
    {
      Dl_info dlInfo;
      dladdr((const void*)__do_not_remove_my_function__, &dlInfo);
      VisusReleaseAssert(dlInfo.dli_sname && dlInfo.dli_saddr);
      KnownPaths::BinaryDirectory = Path(dlInfo.dli_fname).getParent();
    }
    #endif
  }
}
  
bool KernelModule::bAttached = false;

//////////////////////////////////////////////////////
void KernelModule::attach()
{
  if (bAttached)
    return;

  PrintInfo("Attaching KernelModule...");

  bAttached = true;

#if __APPLE__
  InitAutoReleasePool();
#endif

  srand(0);
  std::setlocale(LC_ALL, "en_US.UTF-8");
  Thread::getMainThreadId() = std::this_thread::get_id();

  //this is for generic network code
#if WIN32
  {
    WSADATA data;
    WSAStartup(MAKEWORD(2, 2), &data);
  }
#else
  {
    struct sigaction act, oact; //The SIGPIPE signal will be received if the peer has gone away
    act.sa_handler = SIG_IGN;   //and an attempt is made to write data to the peer. Ignoring this
    sigemptyset(&act.sa_mask);  //signal causes the write operation to receive an EPIPE error.
    act.sa_flags = 0;           //Thus, the user is informed about what happened.
    sigaction(SIGPIPE, &act, &oact);
  }
#endif

  Private::VisusConfig::allocSingleton();

  NetService::attach();

  InitKnownPaths();

  auto config = getModuleConfig();

  for (auto filename : {
    visus_config_commandline_filename ,
    KnownPaths::CurrentWorkingDirectory().getChild("visus.config").toString(),
    KnownPaths::VisusHome.getChild("visus.config").toString() })
  {
    if (filename.empty())
      continue;

    bool bOk = config->load(filename);
    PrintInfo("VisusConfig filename",filename,"ok",bOk ? "YES" : "NO");
    if (bOk)
      break;
  }

  PrintInfo("git_revision            ",VisusGetGitRevision());
  PrintInfo("VisusHome               ",KnownPaths::VisusHome);
  PrintInfo("BinaryDirectory         ",KnownPaths::BinaryDirectory);
  PrintInfo("CurrentWorkingDirectory ",KnownPaths::CurrentWorkingDirectory());

  ArrayPlugins::allocSingleton();
  Encoders::allocSingleton();
  RamResource::allocSingleton();
  UUIDGenerator::allocSingleton();

  //this is to make sure PythonEngine works
#if VISUS_PYTHON
  {
    if (Py_IsInitialized())
    {
      PrintInfo("Visus is running (i.e. extending) python");
    }
    else
    {
      PrintInfo("Initializing embedded python...");

      PythonEngine::setProgramName(VisusGetCommandLine()[0]);

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
      //see https://issues.apache.org/jira/browse/MODPYTHON-217https://stackoverflow.com/questions/23706573/embedded-python-extern-pyrun-simplestring-expected-identifier
      //see https://trac.xapian.org/ticket/185
      __main__thread_state__ = PyEval_SaveThread();
    }

    PrintInfo("Python initialization done");

    //this is important to import swig libraries
#if 1
    if (auto engine = std::make_shared<PythonEngine>(true))
    {
      ScopedAcquireGil acquire_gil;
      engine->execCode("print('PythonEngine is working fine')");
    }
#endif
  }
#endif

  //in case the user whant to simulate I have a certain amount of RAM
  if (Int64 total = StringUtils::getByteSizeFromString(config->readString("Configuration/RamResource/total", "0")))
    RamResource::getSingleton()->setOsTotalMemory(total);

  NetService::Defaults::proxy = config->readString("Configuration/NetService/proxy");
  NetService::Defaults::proxy_port = cint(config->readString("Configuration/NetService/proxyport"));

  NetSocket::Defaults::send_buffer_size = config->readInt("Configuration/NetSocket/send_buffer_size");
  NetSocket::Defaults::recv_buffer_size = config->readInt("Configuration/NetSocket/recv_buffer_size");
  NetSocket::Defaults::tcp_no_delay = config->readBool("Configuration/NetSocket/tcp_no_delay", "1");

  PrintInfo("Attached KernelModule");
}


//////////////////////////////////////////////
void KernelModule::detach()
{
  if (!bAttached)  
    return;
  
  bAttached = false;

  PrintInfo("Detaching KernelModule...");

  ArrayPlugins::releaseSingleton();
  Encoders::releaseSingleton();
  RamResource::releaseSingleton();
  UUIDGenerator::releaseSingleton();

#if VISUS_PYTHON
  if (__main__thread_state__)
  {
    //PrintInfo("Shutting down python...");
    PyEval_RestoreThread(__main__thread_state__);
    Py_Finalize();
  }
#endif

  NetService::detach();

  Private::VisusConfig::releaseSingleton();

#if __APPLE__
  DestroyAutoReleasePool();
#endif

  PrintInfo("Detached KernelModule...");

}

} //namespace Visus


