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

#ifndef __VISUS_OS_H__
#define __VISUS_OS_H__

#include <Visus/Kernel.h>
#include <Visus/File.h>
#include <Visus/Semaphore.h>
#include <Visus/Thread.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#if WIN32
#include <Windows.h>
#include <Psapi.h>
#include <io.h>
#include <direct.h>
#include <process.h>
#include <sddl.h>
#include <ShlObj.h>
#include <winsock2.h>

#pragma warning(disable:4996)
#pragma comment(lib, "advapi32.lib")

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

#elif __clang__
#include <unistd.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <climits>
#include <dispatch/dispatch.h>
#include <mach/mach.h>
#include <mach/mach_host.h>
#include <mach-o/dyld.h>
#include <dlfcn.h>
#include <signal.h>
#include <pwd.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <mach/task.h>
#include <mach/mach_init.h>
#include <mach/vm_statistics.h>
#include <mach/mach_types.h>
#include <mach/mach_host.h>

void mm_InitAutoReleasePool();
void mm_DestroyAutoReleasePool();


#else
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <semaphore.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <sys/sendfile.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <dlfcn.h>
#include <signal.h>
#include <pwd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <unistd.h>

//this solve a problem of old Linux distribution (like Centos 5)
#if !__clang__
  #include <arpa/inet.h>
  #include <byteswap.h>
  #ifndef htole32
  extern "C" uint32_t htole32(uint32_t x) {
    return bswap_32(htonl(x));
  }
  #endif
#endif

#endif


#ifndef O_BINARY
#define O_BINARY 0
#endif 

#ifndef S_ISDIR
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif

#ifndef S_ISREG
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#endif


#if WIN32
#define Stat64 ::_stat64
#else
#define Stat64 ::stat
#endif

namespace Visus {

#if WIN32

class Semaphore::Pimpl
{
public:
  HANDLE handle;

  //constructor
  Pimpl(int initial_value)
  {
    handle = CreateSemaphore(nullptr, initial_value, 0x7FFFFFFF, nullptr); VisusAssert(handle != NULL);
  }

  //destructor
  ~Pimpl() {
    CloseHandle(handle);
  }

  //return only if "number of resources" is not zero, otherwise wait
  void down()
  {
    if (WaitForSingleObject(handle, INFINITE) != WAIT_OBJECT_0)
      ThrowException("critical error, cannot down() the semaphore");
  }

  //tryDown
  bool tryDown() {
    return WaitForSingleObject(handle, 0) == WAIT_OBJECT_0;
  }

  //release one "resource"
  void up()
  {
    if (!ReleaseSemaphore(handle, 1, nullptr))
      ThrowException("critical error, cannot up() the semaphore");
  }

};
#elif __clang__

class Semaphore::Pimpl
{
public:

  dispatch_semaphore_t sem;

  //constructor
  Pimpl(int initial_value)
  {
    sem = dispatch_semaphore_create(initial_value);
    if (!sem) ThrowException("critical error, cannot create the dispatch semaphore:");
  }

  //destructor
  ~Pimpl()
  {
    dispatch_release(sem);
  }

  //return only if "number of resources" is not zero, otherwise wait
  void down()
  {
    long retcode = dispatch_semaphore_wait(this->sem, DISPATCH_TIME_FOREVER);
    if (retcode != 0)
      ThrowException("critical error, cannot dispatch_semaphore_wait() the semaphore");
  }

  //tryDown
  bool tryDown() {
    return dispatch_semaphore_wait(this->sem, DISPATCH_TIME_NOW) == 0;
  }

  //release one "resource"
  void up() {
    dispatch_semaphore_signal(this->sem);
  }

};
#else

class Semaphore::Pimpl
{
public:

  sem_t sem;

  //constructor
  Pimpl(int initial_value) {
    sem_init(&sem, 0, initial_value);
  }

  //destructor
  ~Pimpl() {
    sem_destroy(&sem);
  }

  //down
  void down()
  {
    for (int retcode = sem_wait(&sem); retcode == -1; retcode = sem_wait(&sem))
    {
      if (errno != EINTR)
        ThrowException("critical error, cannot down() the semaphore");
    }
  }


  //tryDown
  bool tryDown() {
    return sem_trywait(&sem) == 0;
  }

  //up
  void up()
  {
    if (sem_post(&sem) == -1)
      ThrowException("critical error, cannot up() the semaphore");
  }

};
#endif

/////////////////////////////////////////////////////////////////////////
class PosixFile : public File::Pimpl
{
public:

  //constructor
  PosixFile() {
  }

  //destructor
  virtual ~PosixFile() {
    close();
  }

  //isOpen
  virtual bool isOpen() const override {
    return this->handle != -1;
  }

  //canRead
  virtual bool canRead() const override {
    return can_read;
  }

  //canWrite
  virtual bool canWrite() const override {
    return can_write;
  }

  //getFilename
  virtual String getFilename() const override {
    return this->filename;
  }

  //open
  virtual bool open(String filename, String file_mode, File::Options options) override
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

#if WIN32
      this->handle = ::_open(filename.c_str(), imode, create_flags);
#else
      this->handle = ::open(filename.c_str(), imode, create_flags);
#endif    

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

  //close
  virtual void close() override
  {
    if (!isOpen())
      return;

#if WIN32
    ::_close(this->handle);
#else
    ::close(this->handle);
#endif     

    this->handle = -1;
    this->cursor = -1;
    this->can_read = false;
    this->can_write = false;
    this->filename = "";
  }

  //size
  virtual Int64 size() override
  {
    if (!isOpen())
      return false;

#if WIN32
    Int64 ret = ::_lseeki64(this->handle, 0, SEEK_END);
#else
    Int64 ret = ::lseek(this->handle, 0, SEEK_END);
#endif

    if (ret < 0)
    {
      this->cursor = -1;
      return ret;
    }

    this->cursor = ret;
    return ret;
  }

  //write 
  virtual bool write(Int64 pos, Int64 tot, const unsigned char* buffer) override
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

#if WIN32
      int n = ::_write(this->handle, buffer, chunk);
#else
      int n = ::write(this->handle, buffer, chunk);
#endif 

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

  //read
  virtual bool read(Int64 pos, Int64 tot, unsigned char* buffer) override
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

#if WIN32
      int n = ::_read(this->handle, buffer, chunk);
#else
      int n = ::read(this->handle, buffer, chunk);
#endif     

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

private:

  String filename;
  bool   can_read = false;
  bool   can_write = false;

  int    handle = -1;
  Int64  cursor = -1;

  //seek
  bool seek(Int64 value)
  {
    if (!isOpen())
      return false;

    // useless call
    if (this->cursor >= 0 && this->cursor == value)
      return true;

#if WIN32
    bool bOk = ::_lseeki64(this->handle, value, SEEK_SET) >= 0;
#else
    bool bOk = ::lseek(this->handle, value, SEEK_SET) >= 0;
#endif

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

  //GetOpenErrorExplanation
  static String GetOpenErrorExplanation()
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

};

/////////////////////////////////////////////////////////////////////////
#if WIN32
class Win32File : public File::Pimpl
{
public:

  VISUS_NON_COPYABLE_CLASS(Win32File)

    //constructor
    Win32File() {
  }

  //destructor
  virtual ~Win32File() {
    close();
  }

  //isOpen
  virtual bool isOpen() const override {
    return handle != INVALID_HANDLE_VALUE;
  }

  //open
  virtual bool open(String filename, String file_mode, File::Options options) override {

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

  //close
  virtual void close() override
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


  //canRead
  virtual bool canRead() const override {
    return can_read;
  }

  //canWrite
  virtual bool canWrite() const override {
    return can_write;
  }

  //getFilename
  virtual String getFilename() const override {
    return filename;
  }

  //size
  virtual Int64 size() override {

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


  //write  
  virtual bool write(Int64 pos, Int64 tot, const unsigned char* buffer) override {

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

  //read (should be portable to 32 and 64 bit OS)
  virtual bool read(Int64 pos, Int64 tot, unsigned char* buffer) override {

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

private:

  String filename;
  bool   can_read = false;
  bool   can_write = false;
  HANDLE handle = INVALID_HANDLE_VALUE;
  Int64  cursor = -1;

  //seek
  virtual bool seek(Int64 value)
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

};
#endif

////////////////////////////////////////////////////////////////////////////////////////////
class MemoryMappedFile : public File::Pimpl
{
public:

  //constructor
  MemoryMappedFile() {
  }

  //destryctor
  virtual ~MemoryMappedFile() {
    close();
  }

  //isOpen
  virtual bool isOpen() const override {
    return mem != nullptr;
  }

  //canRead
  virtual bool canRead() const override {
    return can_read;
  }

  //canWrite
  virtual bool canWrite() const override {
    return can_write;
  }

  //getFilename
  virtual String getFilename() const override {
    return this->filename;
  }

  //open
  virtual bool open(String filename, String file_mode, File::Options options) override
  {
    close();

    bool bMustCreate = options & File::MustCreateFile;

    //not supported
    if (file_mode.find("w") != String::npos || bMustCreate) {
      VisusAssert(false);
      return false;
    }

#if defined(WIN32)
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

  //close
  virtual void close() override
  {
    if (!isOpen())
      return;

#if defined(WIN32)
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

  //size
  virtual Int64 size() override {
    return nbytes;
  }

  //write  
  virtual bool write(Int64 pos, Int64 tot, const unsigned char* buffer) override
  {
    if (!isOpen() || (pos + tot) > this->nbytes)
      return false;

    memcpy(mem + pos, buffer, (size_t)tot);
    onWriteEvent(tot);
    return true;
  }

  //read
  virtual bool read(Int64 pos, Int64 tot, unsigned char* buffer) override
  {
    if (!isOpen() || (pos + tot) > this->nbytes)
      return false;

    memcpy(buffer, mem + pos, (size_t)tot);
    onReadEvent(tot);
    return true;
  }

private:

#if defined(_WIN32)
  void* file = nullptr;
  void* mapping = nullptr;
#else
  int         fd = -1;
#endif

  bool        can_read = false;
  bool        can_write = false;
  String      filename;
  Int64       nbytes = 0;
  char* mem = nullptr;

};

/// /////////////////////////////////////////////////////////////////////////////////////////
class Os {

public:

  //getPlatformName
  static String getPlatformName()
  {
#if WIN32
    return "win";
#elif __clang__
    return "osx";
#else
    return "unix";
#endif
  }

  //BreakInDebugger
  static void BreakInDebugger()
  {
#if WIN32
    _CrtDbgBreak();
#elif __clang__
    asm("int $3");
#else
    ::kill(0, SIGTRAP);
    assert(0);
#endif
  }

  //InitAutoReleasePool
  static void InitAutoReleasePool() {
#if __clang__
    mm_InitAutoReleasePool();
#endif
  }

  //DestroyAutoReleasePool
  static void DestroyAutoReleasePool() {
#if __clang__
    mm_DestroyAutoReleasePool();
#endif
  }

  //GetTotalMemory
  static Int64 GetTotalMemory()
  {
#if WIN32
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
    return status.ullTotalPhys;
#elif __clang__
    int mib[2] = { CTL_HW,HW_MEMSIZE };
    size_t length = sizeof(os_total_memory);
    Int64 ret = 0;
    sysctl(mib, 2, &ret, &length, NULL, 0);
    return ret;
#else
    struct sysinfo memInfo;
    sysinfo(&memInfo);
    return ((Int64)memInfo.totalram) * memInfo.mem_unit;
#endif
  };

  ///GetProcessUsedMemory
  static Int64 GetProcessUsedMemory()
  {
#if WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
    return pmc.PagefileUsage;
#elif __clang__
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

  //GetOsUsedMemory
  static Int64 GetOsUsedMemory()
  {
#if WIN32
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
    return status.ullTotalPhys - status.ullAvailPhys;
#elif __clang__
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

}; //end class

}//namespace Visus


#endif //__VISUS_OS_H__


