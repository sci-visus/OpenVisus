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

#include <Visus/Visus.h>

#include <Visus/Thread.h>
#include <Visus/NetService.h>
#include <Visus/RamResource.h>
#include <Visus/Path.h>
#include <Visus/File.h>
#include <Visus/Encoder.h>
#include <Visus/UUID.h>
#include <Visus/VisusConfig.h>
#include <Visus/ApplicationInfo.h>
#include <Visus/NetService.h>

#include <assert.h>
#include <type_traits>

#include <fstream>
#include <iostream>
#include <atomic>
#include <clocale>

#if WIN32

#pragma warning(disable:4996)

#include <Windows.h>
#include <ShlObj.h>
#include <winsock2.h>

#elif __APPLE__

#include <mach/mach.h>
#include <mach/mach_host.h>
#include <signal.h>
#include <pwd.h>
#include <sys/socket.h>
#include <unistd.h>
#include <mach-o/dyld.h>

#else

#include <signal.h>
#include <sys/sysinfo.h>
#include <pwd.h>
#include <sys/socket.h>
#include <unistd.h>

#endif


#include <Visus/Frustum.h>
#include <Visus/Graph.h>

#include <Visus/KdArray.h>
#include <Visus/TransferFunction.h>
#include <Visus/Statistics.h>
#include <Visus/Array.h>

#include <Visus/PythonEngine.h>


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

#if __APPLE__
  
//see Kernel.mm
String GetMainBundlePath();
  
void InitAutoReleasePool();
  
void DestroyAutoReleasePool();
  
#endif


void InitPython();

void ShutdownPython();

//////////////////////////////////////////////////////////////////
void PrintMessageToTerminal(const String& value) {

#if WIN32
  OutputDebugStringA(value.c_str());
#endif

  std::cout << value;
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


int          Private::CommandLine::argn=0;
const char** Private::CommandLine::argv ;

///////////////////////////////////////////////////////////////////////////////
void ParseCommandLine()
{
  int argn = Private::CommandLine::argn;
  const char** argv = Private::CommandLine::argv;

  // parse command line
  for (int I = 0; I < argn; I++)
  {
    //override visus.config
    if (argv[I] == String("--visus-config") && I < (argn - 1))
    {
      VisusConfig::filename = argv[++I];
      continue;
    }

    //xcode debugger always passes this; just ignore it
    if (argv[I] == String("-NSDocumentRevisionsDebugMode") && I < (argn - 1))
    {
      String ignoring_enabled = argv[++I];
      continue;
    }

    ApplicationInfo::args.push_back(argv[I]);
  }
}

///////////////////////////////////////////////////////////////////////////////
void SetCommandLine(int argn, const char** argv)
{
  Private::CommandLine::argn = argn;
  Private::CommandLine::argv = argv;
  ParseCommandLine();
}

///////////////////////////////////////////////////////////////////////////////
void SetCommandLine(String value)
{
  static String keep_in_memory = value;
  static const char* argv[] = { keep_in_memory.c_str()};
  Private::CommandLine::argn = 1;
  Private::CommandLine::argv = argv;
  ParseCommandLine();
}

/////////////////////////////////////////////////////
bool VisusHasMessageLock()
{
  return Thread::isMainThread();
}


//////////////////////////////////////////////////////
void VisusAssertFailed(const char* file,int line,const char* expr)
{
  if (ApplicationInfo::debug)
    Utils::breakInDebugger();
  else
    ThrowExceptionEx(file,line,expr);
}


//////////////////////////////////////////////////////
void ThrowExceptionEx(String file, int line, String expr)
{
  std::ostringstream out;
  out << "Visus throwing exception file(" << file << ") line(" << line << ") expr(" << expr << ")...";
  String msg = out.str();
  throw std::runtime_error(msg);
}



///////////////////////////////////////////////////////////
static void InitKnownPaths()
{
  std::vector<char> _mem_(2048, 0);

  auto buff = &_mem_[0];
  auto buff_size = (int)_mem_.size() - 1;

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

  //Current application file
  {
    #if WIN32
    {
      auto first_arg = ApplicationInfo::args.empty() ? nullptr : ApplicationInfo::args[0].c_str();
      GetModuleFileName((HINSTANCE)GetModuleHandle(first_arg), buff, buff_size);
      KnownPaths::CurrentApplicationFile = Path(buff);
    }
    #elif __APPLE__
    {
      uint32_t bufsize = buff_size;
      if (_NSGetExecutablePath((char*)buff, &bufsize) == 0)
        KnownPaths::CurrentApplicationFile = Path(buff);
    }
    #else
    {
      int len=readlink("/proc/self/exe", buff, buff_size);
      if (len != -1)  buff[len] = 0;
      KnownPaths::CurrentApplicationFile = Path(buff);
    }
    #endif
  }
}
  
///////////////////////////////////////////////////////////
static bool TryVisusConfig(String value)
{
  if (value.empty())
    return false;

  bool bOk=FileUtils::existsFile(value);
  VisusInfo() << "VisusConfig::filename value(" << value << ") ok(" << ( bOk ? "YES" : "NO") << ")";
  if (!bOk)
    return false;

  VisusConfig::filename = Path(value);
  return true;
}

///////////////////////////////////////////////////////////
static void InitVisusConfig()
{
  TryVisusConfig(VisusConfig::filename) ||
  TryVisusConfig(KnownPaths::CurrentWorkingDirectory().getChild("visus.config")) ||
  TryVisusConfig(KnownPaths::VisusHome.getChild("visus.config"));
}


bool KernelModule::bAttached = false;

//////////////////////////////////////////////////////
void KernelModule::attach()
{
  if (bAttached)  
    return;

  VisusInfo() << "Attaching KernelModule...";
  
  bAttached = true;

  ApplicationInfo::start = Time::now();

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

  NetService::attach();

  InitKnownPaths();
  InitVisusConfig();
  InitPython();

  VisusInfo() << "git_revision            " << ApplicationInfo::git_revision;
  VisusInfo() << "VisusHome               " << KnownPaths::VisusHome.toString();
  VisusInfo() << "CurrentApplicationFile  " << KnownPaths::CurrentApplicationFile.toString();
  VisusInfo() << "CurrentWorkingDirectory " << KnownPaths::CurrentWorkingDirectory().toString();

  ObjectFactory::allocSingleton();

  ArrayPlugins::allocSingleton();
  Encoders::allocSingleton();
  RamResource::allocSingleton();
  UUIDGenerator::allocSingleton();

  VisusConfig::reload();

  VISUS_REGISTER_OBJECT_CLASS(DType);
  VISUS_REGISTER_OBJECT_CLASS(Object);

  VISUS_REGISTER_OBJECT_CLASS(Model);

  VISUS_REGISTER_OBJECT_CLASS(BoolObject);
  VISUS_REGISTER_OBJECT_CLASS(IntObject);
  VISUS_REGISTER_OBJECT_CLASS(Int64Object);
  VISUS_REGISTER_OBJECT_CLASS(DoubleObject);
  VISUS_REGISTER_OBJECT_CLASS(StringObject);
  VISUS_REGISTER_OBJECT_CLASS(ListObject);
  VISUS_REGISTER_OBJECT_CLASS(DictObject);
  VISUS_REGISTER_OBJECT_CLASS(StringTree);
  VISUS_REGISTER_OBJECT_CLASS(Position);
  VISUS_REGISTER_OBJECT_CLASS(Frustum);
  VISUS_REGISTER_OBJECT_CLASS(Array);
  VISUS_REGISTER_OBJECT_CLASS(Field);

  VISUS_REGISTER_OBJECT_CLASS(KdArray);
  VISUS_REGISTER_OBJECT_CLASS(KdArrayNode);
  VISUS_REGISTER_OBJECT_CLASS(Range);

  VISUS_REGISTER_OBJECT_CLASS(GraphInt8);
  VISUS_REGISTER_OBJECT_CLASS(GraphUint8);
  VISUS_REGISTER_OBJECT_CLASS(GraphInt16);
  VISUS_REGISTER_OBJECT_CLASS(GraphUint16);
  VISUS_REGISTER_OBJECT_CLASS(GraphInt32);
  VISUS_REGISTER_OBJECT_CLASS(GraphUint32);
  VISUS_REGISTER_OBJECT_CLASS(GraphInt64);
  VISUS_REGISTER_OBJECT_CLASS(GraphUint64);
  VISUS_REGISTER_OBJECT_CLASS(GraphFloat32);
  VISUS_REGISTER_OBJECT_CLASS(GraphFloat64);
  VISUS_REGISTER_OBJECT_CLASS(FGraph);
  VISUS_REGISTER_OBJECT_CLASS(CGraph);

  VISUS_REGISTER_OBJECT_CLASS(TransferFunction);

  //this is to make sure PythonEngine works
  if (auto engine = std::make_shared<PythonEngine>(true) )
  {
    ScopedAcquireGil acquire_gil;
    engine->execCode("print('PythonEngine is working fine')");
  }

  VisusInfo() << "Attached KernelModule";
}


//////////////////////////////////////////////
void KernelModule::detach()
{
  if (!bAttached)  
    return;
  
  bAttached = false;

  VisusInfo() << "Detaching KernelModule...";

  ObjectFactory::releaseSingleton();
  ArrayPlugins::releaseSingleton();
  Encoders::releaseSingleton();
  RamResource::releaseSingleton();
  UUIDGenerator::releaseSingleton();

  ShutdownPython();

  NetService::detach();

#if __APPLE__
  DestroyAutoReleasePool();
#endif

  VisusInfo() << "Detached KernelModule...";

}

} //namespace Visus


