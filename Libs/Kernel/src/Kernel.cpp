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
#include <Visus/StringTree.h>
#include <Visus/NetService.h>
#include <Visus/SharedLibrary.h>

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

#include "EncoderId.hxx"
#include "EncoderLz4.hxx"
#include "EncoderZip.hxx"
#include "EncoderZfp.hxx"

#include "ArrayPluginDevnull.hxx"
#include "ArrayPluginRawArray.hxx"

#if VISUS_IMAGE

#  if WIN32
#    include <WinSock2.h>
#  elif __APPLE__
#  else
#    include <arpa/inet.h>
#  endif

#  include <FreeImage.h>

#  include "ArrayPluginFreeimage.hxx"
#  include "EncoderFreeImage.hxx"

#endif

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

String OpenVisus_VERSION="";

#ifdef GIT_REVISION
  #define __str__(s) #s
  #define __xstr__(s) __str__(s)
  String OpenVisus_GIT_REVISION = __xstr__(GIT_REVISION);
#else
  String OpenVisus_GIT_REVISION = "";
#endif

std::vector<String> CommandLine::args;
  
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



//check 64 bit file IO is enabled!
#if __GNUC__ && !__APPLE__
VisusAssert(sizeof(off_t) == 8, "internal error");
#endif

int          CommandLine::argn=0;
const char** CommandLine::argv ;

static String visus_config_commandline_filename;

///////////////////////////////////////////////////////////////////////////////
String cstring10(double value) {
  std::ostringstream out;
  out << std::setprecision(std::numeric_limits<double>::max_digits10) << value;
  return out.str();
}

///////////////////////////////////////////////////////////////////////////////
void SetCommandLine(int argn, const char** argv)
{
  CommandLine::argn = argn;
  CommandLine::argv = argv;

  // parse command line
  for (int I = 0; I < argn; I++)
  {
    //override visus.config
    if (argv[I] == String("--visus-config") && I < (argn - 1))
    {
      visus_config_commandline_filename = argv[++I];
      continue;
    }

    //xcode debugger always passes this; just ignore it
    if (argv[I] == String("-NSDocumentRevisionsDebugMode") && I < (argn - 1))
    {
      String ignoring_enabled = argv[++I];
      continue;
    }

    CommandLine::args.push_back(argv[I]);
  }
}

/////////////////////////////////////////////////////
void SetCommandLine(std::vector<String> args)
{
  static auto keep_in_memory = args;
  static const int argn = (int)args.size();
  static const char* argv[256];
  memset(argv, 0, sizeof(argv));
  for (int I = 0; I < args.size(); I++)
    argv[I] = args[I].c_str();

  SetCommandLine(argn, argv);
}


/////////////////////////////////////////////////////
bool VisusHasMessageLock()
{
  return Thread::isMainThread();
}


//////////////////////////////////////////////////////
void VisusAssertFailed(const char* file,int line,const char* expr)
{
#if _DEBUG
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


  bAttached = true;

  //check types
  VisusReleaseAssert(sizeof(Int8) == 1 && sizeof(Uint8) == 1 && sizeof(char) == 1);
  VisusReleaseAssert(sizeof(Int16) == 2 && sizeof(Int16) == 2 && sizeof(short) == 2);
  VisusReleaseAssert(sizeof(Int32) == 4 && sizeof(Int32) == 4 && sizeof(int) == 4);
  VisusReleaseAssert(sizeof(Int64) == 8 && sizeof(Uint64) == 8);
  VisusReleaseAssert(sizeof(Float32) == 4 && sizeof(float) == 4);
  VisusReleaseAssert(sizeof(Float64) == 8 && sizeof(double) == 8);

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

  VisusReleaseAssert(sizeof(S009) == 9);
  VisusReleaseAssert(sizeof(S017) == 17);
  VisusReleaseAssert(sizeof(S021) == 21);
  VisusReleaseAssert(sizeof(S033) == 33);
  VisusReleaseAssert(sizeof(S037) == 37);
  VisusReleaseAssert(sizeof(S065) == 65);
  VisusReleaseAssert(sizeof(S129) == 129);
  VisusReleaseAssert(sizeof(S133) == 133);


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
#if _DEBUG
    PrintInfo("VisusConfig filename",filename,"ok",bOk ? "YES" : "NO");
#endif
    if (bOk) break;
  }

#if _DEBUG
  PrintInfo(
    "VERSION", OpenVisus_VERSION,
    "GIT_REVISION", OpenVisus_GIT_REVISION,
    "VisusHome", KnownPaths::VisusHome, 
    "BinaryDirectory", KnownPaths::BinaryDirectory,
    "CurrentWorkingDirectory ", KnownPaths::CurrentWorkingDirectory());
#endif

  ArrayPlugins::allocSingleton();
  Encoders::allocSingleton();
  RamResource::allocSingleton();

  //in case the user whant to simulate I have a certain amount of RAM
  if (Int64 total = StringUtils::getByteSizeFromString(config->readString("Configuration/RamResource/total", "0")))
    RamResource::getSingleton()->setOsTotalMemory(total);

  NetService::Defaults::proxy = config->readString("Configuration/NetService/proxy");
  NetService::Defaults::proxy_port = cint(config->readString("Configuration/NetService/proxyport"));

  NetSocket::Defaults::send_buffer_size = config->readInt("Configuration/NetSocket/send_buffer_size");
  NetSocket::Defaults::recv_buffer_size = config->readInt("Configuration/NetSocket/recv_buffer_size");
  NetSocket::Defaults::tcp_no_delay = config->readBool("Configuration/NetSocket/tcp_no_delay", "1");

  //array plugins
  {
    ArrayPlugins::getSingleton()->values.push_back(std::make_shared<DevNullArrayPlugin>());
    ArrayPlugins::getSingleton()->values.push_back(std::make_shared<RawArrayPlugin>());

#if VISUS_IMAGE
    ArrayPlugins::getSingleton()->values.push_back(std::make_shared<FreeImageArrayPlugin>());
#endif
  }

  //encoders
  {
    Encoders::getSingleton()->registerEncoder("",    [](String specs) {return std::make_shared<IdEncoder>(specs); }); 
    Encoders::getSingleton()->registerEncoder("raw", [](String specs) {return std::make_shared<IdEncoder>(specs); });
    Encoders::getSingleton()->registerEncoder("bin", [](String specs) {return std::make_shared<IdEncoder>(specs); });
    Encoders::getSingleton()->registerEncoder("lz4", [](String specs) {return std::make_shared<LZ4Encoder>(specs); });
    Encoders::getSingleton()->registerEncoder("zip", [](String specs) {return std::make_shared<ZipEncoder>(specs); });
    Encoders::getSingleton()->registerEncoder("zfp", [](String specs) {return std::make_shared<ZfpEncoder>(specs); });

#if VISUS_IMAGE
    Encoders::getSingleton()->registerEncoder("png", [](String specs) {return std::make_shared<FreeImageEncoder>(specs); });
    Encoders::getSingleton()->registerEncoder("jpg", [](String specs) {return std::make_shared<FreeImageEncoder>(specs); });
    Encoders::getSingleton()->registerEncoder("tif", [](String specs) {return std::make_shared<FreeImageEncoder>(specs); });
#endif
  }

  //test plugin
#if 0
  auto lib = std::make_shared<SharedLibrary>();
  if (lib->load(SharedLibrary::getFilenameInBinaryDirectory("MyPlugin")))
  {
    auto get_instance = (SharedPlugin * (*)())example.findSymbol("GetSharedPluginInstance");
    VisusReleaseAssert(get_instance);
    auto plugin = get_instance();
    plugin->lib = lib;
  }
#endif
}


//////////////////////////////////////////////
void KernelModule::detach()
{
  if (!bAttached)  
    return;
  
  bAttached = false;

  ArrayPlugins::releaseSingleton();
  Encoders::releaseSingleton();
  RamResource::releaseSingleton();

  NetService::detach();

  Private::VisusConfig::releaseSingleton();

#if __APPLE__
  DestroyAutoReleasePool();
#endif
}

} //namespace Visus


