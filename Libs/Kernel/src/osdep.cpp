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
#include "osdep.hxx"

#if __GNUC__ && !__APPLE__ && !WIN32
	//this solve a problem of old Linux distribution (like Centos 5)
	#ifndef htole32
		#include <byteswap.h>
		extern "C" uint32_t htole32(uint32_t x) {
		  return bswap_32(htonl(x));
		}	
	#endif
#endif

namespace Visus {
	
#if WIN32 
  static void __do_not_remove_my_function__() {
  }
#else
  VISUS_SHARED_EXPORT void __do_not_remove_my_function__() {
  }
#endif



#if WIN32 
static std::string Win32FormatErrorMessage(DWORD ErrorCode)
{
  TCHAR* buff = nullptr;
  const int flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
  auto language_id = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
  DWORD   len = FormatMessage(flags, nullptr, ErrorCode, language_id, reinterpret_cast<LPTSTR>(&buff), 0, nullptr);
  std::string ret(buff, len);
  LocalFree(buff);
  return ret;
}
#endif

/////////////////////////////////////////////////////////////////////
bool PosixFile::open(String filename, String file_mode, File::Options options) 
{
  bool bRead = StringUtils::contains(file_mode, "r");
  bool bWrite = StringUtils::contains(file_mode, "w");
  bool bMustCreate = options & File::MustCreateFile;

  int imode = O_BINARY;
  if (bRead && bWrite) imode |= O_RDWR;
  else if (bRead)           imode |= O_RDONLY;
  else if (bWrite)          imode |= O_WRONLY;
  else  VisusAssert(false);

  int create_flags = 0;

  if (bMustCreate)
  {
    imode |= O_CREAT | O_EXCL;

#if WIN32 
    create_flags |= (S_IREAD | S_IWRITE);
#else
    create_flags |= (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
#endif
  }

  for (int nattempt = 0; nattempt < (bMustCreate ? 2 : 1); nattempt++)
  {
    if (nattempt)
      FileUtils::createDirectory(Path(filename).getParent());

    this->handle = ::open(filename.c_str(), imode, create_flags);

    if (isOpen())
      break;
  }

  if (!isOpen())
  {
    if (!(bMustCreate && errno == EEXIST) && !(!bMustCreate && errno == ENOENT))
    {
      std::ostringstream out; out << Thread::getThreadId();
      PrintWarning("Thread[", out.str(), "] ERROR opening file ", filename, GetOpenErrorExplanation());
    }
    return false;
  }

  onOpenEvent();
  this->can_read = bRead;
  this->can_write = bWrite;
  this->filename = filename;
  this->cursor = 0;
  return true;
}

/////////////////////////////////////////////////////////////////////
void PosixFile::close() 
{
  if (!isOpen())
    return;

  ::close(this->handle);

  this->handle = -1;
  this->cursor = -1;
  this->can_read = false;
  this->can_write = false;
  this->filename = "";
}

/////////////////////////////////////////////////////////////////////
Int64 PosixFile::size() 
{
  if (!isOpen())
    return false;

  Int64 ret = LSeeki64(this->handle, 0, SEEK_END);

  if (ret < 0)
  {
    this->cursor = -1;
    return ret;
  }

  this->cursor = ret;
  return ret;
}

/////////////////////////////////////////////////////////////////////
bool PosixFile::write(Int64 pos, Int64 tot, const unsigned char* buffer) 
{
  if (!isOpen() || tot < 0 || !can_write)
    return false;

  if (tot == 0)
    return true;

  if (!seek(pos))
    return false;

  for (Int64 remaining = tot; remaining;)
  {
    int chunk = (remaining >= INT_MAX) ? INT_MAX : (int)remaining;
    int n = ::write(this->handle, buffer, chunk);

    if (n <= 0)
    {
      this->cursor = -1;
      return false;
    }

    onWriteEvent(n);
    remaining -= n;
    buffer += n;
  }

  if (this->cursor >= 0)
    this->cursor += tot;

  return true;
}

/////////////////////////////////////////////////////////////////////
bool PosixFile::read(Int64 pos, Int64 tot, unsigned char* buffer) 
{
  if (!isOpen() || tot < 0 || !can_read)
    return false;

  if (tot == 0)
    return true;

  if (!seek(pos))
    return false;

  for (Int64 remaining = tot; remaining;)
  {
    int chunk = (remaining >= INT_MAX) ? INT_MAX : (int)remaining;
    int n = ::read(this->handle, buffer, chunk);

    if (n <= 0)
    {
      this->cursor = -1;
      return false;
    }

    onReadEvent(n);
    remaining -= n;
    buffer += n;
  }

  if (this->cursor >= 0)
    this->cursor += tot;

  return true;
}


/////////////////////////////////////////////////////////////////////
bool PosixFile::seek(Int64 value)
{
  if (!isOpen())
    return false;

  // useless call
  if (this->cursor >= 0 && this->cursor == value)
    return true;

  bool bOk = LSeeki64(this->handle, value, SEEK_SET) >= 0;

  if (!bOk) {
    this->cursor = -1;
    return false;
  }
  else
  {
    this->cursor = value;
    return true;
  }
}

/////////////////////////////////////////////////////////////////////
String PosixFile::GetOpenErrorExplanation()
{
  switch (errno)
  {
  case EACCES:
    return "EACCES Tried to open a read-only file for writing, file's sharing mode does not allow the specified operations, or the given path is a directory.";
  case EEXIST:
    return"EEXIST _O_CREAT and _O_EXCL flags specified, but filename already exists.";
  case EINVAL:
    return"EINVAL Invalid oflag or pmode argument.";
  case EMFILE:
    return"EMFILE No more file descriptors are available (too many files are open).";
  case ENOENT:
    return"ENOENT File or path not found.";
  default:
    return cstring("Unknown errno", errno);
  }
}



/////////////////////////////////////////////////////////////////////
bool MemoryMappedFile::open(String filename, String file_mode, File::Options options) 
{
  close();

  bool bMustCreate = options & File::MustCreateFile;

  //not supported
  if (file_mode.find("w") != String::npos || bMustCreate) {
    VisusAssert(false);
    return false;
  }

#if WIN32 
  {
    this->file = CreateFileA(filename.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (file == INVALID_HANDLE_VALUE) {
      close();
      return false;
    }

    this->nbytes = GetFileSize(file, nullptr);
    this->mapping = CreateFileMapping(file, nullptr, PAGE_READONLY, 0, 0, nullptr);

    if (mapping == nullptr) {
      close();
      return false;
    }

    this->mem = (char*)MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
  }
#else
  {
    this->fd = ::open(filename.c_str(), O_RDONLY);
    if (this->fd == -1) {
      close();
      return false;
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1) {
      close();
      return false;
    }

    this->nbytes = sb.st_size;
    this->mem = (char*)mmap(nullptr, nbytes, PROT_READ, MAP_PRIVATE, fd, 0);
  }
#endif

  if (!mem) {
    close();
    return false;
  }


  onOpenEvent();
  this->filename = filename;
  this->can_read = file_mode.find("r") != String::npos;
  this->can_write = file_mode.find("w") != String::npos;
  return true;
}

/////////////////////////////////////////////////////////////////////
void MemoryMappedFile::close() 
{
  if (!isOpen())
    return;

#if WIN32 
  {
    if (mem)
      UnmapViewOfFile(mem);

    if (mapping)
      CloseHandle(mapping);

    if (file != INVALID_HANDLE_VALUE)
      CloseHandle(file);

    mapping = nullptr;
    file = INVALID_HANDLE_VALUE;
  }
#else
  {
    if (mem)
      munmap(mem, nbytes);

    if (fd != -1)
    {
      ::close(fd);
      fd = -1;
    }
  }
#endif

  this->can_read = false;
  this->can_write = false;
  this->nbytes = 0;
  this->mem = nullptr;
  this->filename = "";
}



/////////////////////////////////////////////////////////////////////
bool MemoryMappedFile::write(Int64 pos, Int64 tot, const unsigned char* buffer) 
{
  if (!isOpen() || (pos + tot) > this->nbytes)
    return false;

  memcpy(mem + pos, buffer, (size_t)tot);
  onWriteEvent(tot);
  return true;
}

/////////////////////////////////////////////////////////////////////
bool MemoryMappedFile::read(Int64 pos, Int64 tot, unsigned char* buffer) 
{
  if (!isOpen() || (pos + tot) > this->nbytes)
    return false;

  memcpy(buffer, mem + pos, (size_t)tot);
  onReadEvent(tot);
  return true;
}


#if WIN32 


/////////////////////////////////////////////////////////////////////
bool Win32File::open(String filename, String file_mode, File::Options options) {

  bool bRead = StringUtils::contains(file_mode, "r");
  bool bWrite = StringUtils::contains(file_mode, "w");
  bool bMustCreate = options & File::MustCreateFile;

  this->handle = CreateFile(
    filename.c_str(),
    (bRead ? GENERIC_READ : 0) | (bWrite ? GENERIC_WRITE : 0),
    bWrite ? 0 : FILE_SHARE_READ,
    NULL,
    bMustCreate ? CREATE_NEW : OPEN_EXISTING,
    FILE_ATTRIBUTE_NORMAL, // | FILE_FLAG_NO_BUFFERING DOES NOT WORK..because I must align to blocksize
    NULL);

  if (!isOpen())
  {
    close();
    return false;
  }

  onOpenEvent();
  this->can_read = bRead;
  this->can_write = bWrite;
  this->filename = filename;
  this->cursor = 0;
  return true;
}

/////////////////////////////////////////////////////////////////////
void Win32File::close() 
{
  if (!isOpen())
    return;

  CloseHandle(handle);
  this->handle = INVALID_HANDLE_VALUE;
  this->can_read = false;
  this->can_write = false;
  this->filename = "";
  this->cursor = -1;
}


/////////////////////////////////////////////////////////////////////
Int64 Win32File::size() {

  if (!isOpen())
    return false;

  LARGE_INTEGER ret;
  ZeroMemory(&ret, sizeof(ret));

  LARGE_INTEGER zero;
  ZeroMemory(&zero, sizeof(zero));

  bool bOk = SetFilePointerEx(this->handle, zero, &ret, FILE_END);
  if (!bOk)
    return (this->cursor = -1);
  else
    return (this->cursor = ret.QuadPart);
}


/////////////////////////////////////////////////////////////////////
bool Win32File::write(Int64 pos, Int64 tot, const unsigned char* buffer) {

  if (!isOpen() || tot < 0 || !can_write)
    return false;

  if (tot == 0)
    return true;

  if (!seek(pos))
    return false;

  for (Int64 remaining = tot; remaining;)
  {
    int chunk = (remaining >= INT_MAX) ? INT_MAX : (int)remaining;

    DWORD _num_write_ = 0;
    int n = WriteFile(handle, buffer, chunk, &_num_write_, NULL) ? _num_write_ : 0;

    if (n <= 0)
    {
      this->cursor = -1;
      return false;
    }

    onWriteEvent(n);
    remaining -= n;
    buffer += n;
  }

  if (this->cursor >= 0)
    this->cursor += tot;

  return true;

}

/////////////////////////////////////////////////////////////////////
bool Win32File::read(Int64 pos, Int64 tot, unsigned char* buffer)  {

  if (!isOpen() || tot < 0 || !can_read)
    return false;

  if (tot == 0)
    return true;

  if (!seek(pos))
    return false;

  for (Int64 remaining = tot; remaining;)
  {
    int chunk = (remaining >= INT_MAX) ? INT_MAX : (int)remaining;

    DWORD __num_read__ = 0;
    int n = ReadFile(handle, buffer, chunk, &__num_read__, NULL) ? __num_read__ : 0;

    if (n <= 0)
    {
      this->cursor = -1;
      return false;
    }

    onReadEvent(n);
    remaining -= n;
    buffer += n;
  }

  if (this->cursor >= 0)
    this->cursor += tot;

  return true;

}


/////////////////////////////////////////////////////////////////////
bool Win32File::seek(Int64 value)
{
  if (!isOpen())
    return false;

  // useless call
  if (this->cursor >= 0 && this->cursor == value)
    return true;

  LARGE_INTEGER offset;
  ZeroMemory(&offset, sizeof(offset));
  offset.QuadPart = value;

  LARGE_INTEGER new_file_pointer;
  ZeroMemory(&new_file_pointer, sizeof(new_file_pointer));
  bool bOk = SetFilePointerEx(handle, offset, &new_file_pointer, FILE_BEGIN);

  if (!bOk) {
    this->cursor = -1;
    return false;
  }
  else
  {
    this->cursor = value;
    return true;
  }
}


#endif


//////////////////////////////////////////////////////////
String osdep::getPlatformName()
{
#if WIN32
  return "win";
#elif __APPLE__
  return "osx";
#else
  return "unix";
#endif
}

//////////////////////////////////////////////////////////
void osdep::BreakInDebugger()
{
#if WIN32 
  #if __MSVC_VER
    _CrtDbgBreak();
  #else
    DebugBreak();
  #endif
#elif __APPLE__
  asm("int $3");
#else
  ::kill(0, SIGTRAP);
  assert(0);
#endif
}

//////////////////////////////////////////////////////////
void osdep::InitAutoReleasePool() {
#if __clang__
  mm_InitAutoReleasePool();
#endif
}

//////////////////////////////////////////////////////////
void osdep::DestroyAutoReleasePool() {
#if __clang__
  mm_DestroyAutoReleasePool();
#endif
}

//////////////////////////////////////////////////////////
Int64 osdep::GetTotalMemory()
{
#if WIN32 
  MEMORYSTATUSEX status;
  status.dwLength = sizeof(status);
  GlobalMemoryStatusEx(&status);
  return status.ullTotalPhys;
#elif __APPLE__
  int mib[2] = { CTL_HW,HW_MEMSIZE };
  Int64 ret = 0;
  size_t length = sizeof(ret);
  sysctl(mib, 2, &ret, &length, NULL, 0);
  return ret;
#else
  struct sysinfo memInfo;
  sysinfo(&memInfo);
  return ((Int64)memInfo.totalram) * memInfo.mem_unit;
#endif
};

//////////////////////////////////////////////////////////
Int64 osdep::GetProcessUsedMemory()
{
#if WIN32 
  PROCESS_MEMORY_COUNTERS pmc;
  GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
  return pmc.PagefileUsage;
#elif __APPLE__
  struct task_basic_info t_info;
  mach_msg_type_number_t t_info_count = TASK_BASIC_INFO_COUNT;
  task_info(current_task(), TASK_BASIC_INFO, (task_info_t)&t_info, &t_info_count);
  size_t size = t_info.resident_size;
  return (Int64)size;
#else
  long rss = 0L;
  FILE* fp = NULL;
  if ((fp = fopen("/proc/self/statm", "r")) == NULL) return 0;
  if (fscanf(fp, "%*s%ld", &rss) != 1)
  {
    fclose(fp); return 0;
  }
  fclose(fp);
  return (Int64)rss * (Int64)sysconf(_SC_PAGESIZE);
#endif
}



//////////////////////////////////////////////////////////
double osdep::GetRandDouble(double a, double b) {
#if WIN32 
  {return a + (((double)rand()) / (double)RAND_MAX) * (b - a); }
#else
  {return a + drand48() * (b - a); }
#endif
}

//////////////////////////////////////////////////////////
bool osdep::createDirectory(String dirname)
{
#if WIN32 
  return CreateDirectory(TEXT(dirname.c_str()), NULL) != 0;
#else
  return ::mkdir(dirname.c_str(), 0775) == 0; //user(rwx) group(rwx) others(r-x)
#endif
}

//////////////////////////////////////////////////////////
bool osdep::removeDirectory(String value)
{
  return ::rmdir(value.c_str()) == 0 ? true : false;
}

//////////////////////////////////////////////////////////
bool osdep::createLink(String existing_file, String new_file)
{
#if WIN32 
  if (CreateHardLink(new_file.c_str(), existing_file.c_str(), nullptr) == 0)
  {
    PrintWarning("Error creating link", Win32FormatErrorMessage(GetLastError()));
    return false;
  }
  return true;
#else
  return symlink(existing_file.c_str(), new_file.c_str()) == 0;
#endif
}

//////////////////////////////////////////////////////////
Int64 osdep::getTimeStamp()
{
#if WIN32 
  struct _timeb t;
#ifdef _INC_TIME_INL
  _ftime_s(&t);
#else
  _ftime(&t);
#endif
  return ((Int64)t.time) * 1000 + t.millitm;
#else
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  return ((Int64)tv.tv_sec) * 1000 + tv.tv_usec / 1000;
#endif
}

//////////////////////////////////////////////////////////
String osdep::safe_strerror(int err)
{
  const int buffer_size = 512;
  char buf[buffer_size];
#if WIN32 
  strerror_s(buf, sizeof(buf), err);
#else
  if (strerror_r(err, buf, sizeof(buf)) != 0)
    buf[0] = 0;
#endif

  return String(buf);
}

//////////////////////////////////////////////////////////
String osdep::CurrentWorkingDirectory()
{
#if WIN32 
  {
    char buff[2048];
    ::GetCurrentDirectory(sizeof(buff), buff);
    std::replace(buff, buff + sizeof(buff), '\\', '/');
    return buff;
  }
#else
  {
    char buff[2048];
    return getcwd(buff, sizeof(buff));
  }
#endif
}

//////////////////////////////////////////////////////////
void osdep::PrintMessageToTerminal(const String& value) {
#if WIN32 
  OutputDebugStringA(value.c_str());
#endif
  std::cout << value;
}

//////////////////////////////////////////////////////////
String osdep::getHomeDirectory()
{
#if WIN32 
  {
    char buff[2048]; 
    memset(buff, 0, sizeof(buff));
    SHGetSpecialFolderPath(0, buff, CSIDL_PERSONAL, FALSE);
    return buff;
  }
#else
  {
    if (auto homedir = getenv("HOME"))
      return homedir;

    else if (auto pw = getpwuid(getuid()))
      return pw->pw_dir;
  }
#endif

  ThrowException("internal error");
  return "/";
}

//////////////////////////////////////////////////////////
void osdep::startup()
{
  //this is for generic network code
#if WIN32 
  WSADATA data;
  WSAStartup(MAKEWORD(2, 2), &data);
#else
  struct sigaction act, oact; //The SIGPIPE signal will be received if the peer has gone away
  act.sa_handler = SIG_IGN;   //and an attempt is made to write data to the peer. Ignoring this
  sigemptyset(&act.sa_mask);  //signal causes the write operation to receive an EPIPE error.
  act.sa_flags = 0;           //Thus, the user is informed about what happened.
  sigaction(SIGPIPE, &act, &oact);
#endif
}

//////////////////////////////////////////////////////////
struct tm osdep::millisToLocal(const Int64 millis)
{
  struct tm result;
  const Int64 seconds = millis / 1000;

  if (seconds < 86400LL || seconds >= 2145916800LL)
  {
    // use extended maths for dates beyond 1970 to 2037..
    const int timeZoneAdjustment = 31536000 - (int)(Time(1971, 0, 1, 0, 0).getUTCMilliseconds() / 1000);
    const Int64 jdm = seconds + timeZoneAdjustment + 210866803200LL;

    const int days = (int)(jdm / 86400LL);
    const int a = 32044 + days;
    const int b = (4 * a + 3) / 146097;
    const int c = a - (b * 146097) / 4;
    const int d = (4 * c + 3) / 1461;
    const int e = c - (d * 1461) / 4;
    const int m = (5 * e + 2) / 153;

    result.tm_mday = e - (153 * m + 2) / 5 + 1;
    result.tm_mon = m + 2 - 12 * (m / 10);
    result.tm_year = b * 100 + d - 6700 + (m / 10);
    result.tm_wday = (days + 1) % 7;
    result.tm_yday = -1;

    int t = (int)(jdm % 86400LL);
    result.tm_hour = t / 3600;
    t %= 3600;
    result.tm_min = t / 60;
    result.tm_sec = t % 60;
    result.tm_isdst = -1;
  }
  else
  {
    time_t now = static_cast <time_t> (seconds);

#if WIN32 
#ifdef _INC_TIME_INL
    if (now >= 0 && now <= 0x793406fff)
      localtime_s(&result, &now);
    else
      memset(&result, 0, sizeof(result));
#else
    result = *localtime(&now);
#endif
#else
    localtime_r(&now, &result); // more thread-safe
#endif
  }

  return result;
}

//////////////////////////////////////////////////////////
Int64 osdep::GetOsUsedMemory()
{
#if WIN32 
  MEMORYSTATUSEX status;
  status.dwLength = sizeof(status);
  GlobalMemoryStatusEx(&status);
  return status.ullTotalPhys - status.ullAvailPhys;
#elif __APPLE__
  vm_statistics_data_t vm_stats;
  mach_port_t mach_port = mach_host_self();
  mach_msg_type_number_t count = sizeof(vm_stats) / sizeof(natural_t);
  vm_size_t page_size;
  host_page_size(mach_port, &page_size);
  host_statistics(mach_port, HOST_VM_INFO, (host_info_t)&vm_stats, &count);
  return ((int64_t)vm_stats.active_count +
    (int64_t)vm_stats.inactive_count +
    (int64_t)vm_stats.wire_count) * (int64_t)page_size;
#else
  struct sysinfo memInfo;
  sysinfo(&memInfo);

  Int64 ret = memInfo.totalram - memInfo.freeram;
  //Read /proc/meminfo to get cached ram (freed upon request)
  FILE* fp;
  int MAXLEN = 1000;
  char buf[MAXLEN];
  fp = fopen("/proc/meminfo", "r");

  if (fp) {
    for (int i = 0; i <= 3; i++) {
      if (fgets(buf, MAXLEN, fp) == nullptr)
        buf[0] = 0;
    }
    char* p1 = strchr(buf, (int)':');
    unsigned long cacheram = strtoull(p1 + 1, NULL, 10) * 1000;
    ret -= cacheram;
    fclose(fp);
  }

  ret *= memInfo.mem_unit;
  return ret;
#endif
}


//////////////////////////////////////////////////////////
String osdep::getCurrentApplicationFile()
{
#if WIN32 
    //see https://stackoverflow.com/questions/6924195/get-dll-path-at-runtime
    HMODULE handle;
    GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)__do_not_remove_my_function__, &handle);
    VisusReleaseAssert(handle);
    char buff[2048];
    memset(buff, 0, sizeof(buff));
    GetModuleFileName(handle, buff, sizeof(buff));
    return buff;
#else
    Dl_info dlInfo;
    dladdr((const void*)__do_not_remove_my_function__, &dlInfo);
    VisusReleaseAssert(dlInfo.dli_sname && dlInfo.dli_saddr);
    return dlInfo.dli_fname;
#endif
}

//////////////////////////////////////////////////////////
void osdep::setBitThreadSafe(unsigned char* buffer, Int64 bit, bool value)
{
  volatile char* Byte = (char*)buffer + (bit >> 3);
  char Mask = 1 << (bit & 0x07);
#if WIN32
  value ? _InterlockedOr8(Byte, Mask) : _InterlockedAnd8(Byte, ~Mask);
#else
  value ? __sync_fetch_and_or(Byte, Mask) : __sync_fetch_and_and(Byte, ~Mask);
#endif
}


} //namespace Visus