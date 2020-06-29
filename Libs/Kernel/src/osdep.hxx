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

#ifndef __VISUS_OS_DEP_H__
#define __VISUS_OS_DEP_H__

#include <Visus/Kernel.h>
#include <Visus/File.h>
#include <Visus/Semaphore.h>
#include <Visus/Thread.h>
#include <Visus/Time.h>

#include <iostream>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>

//////////////////////////////////////////////////
#if WIN32
	#include <Windows.h>
	#include <Psapi.h>
	#include <io.h>
	#include <direct.h>
	#include <process.h>
	#include <sddl.h>
	#include <ShlObj.h>
	#include <winsock2.h>
	#include <time.h>
	#include <sys/timeb.h>
	
	#pragma warning(disable:4996)
	
	#define getIpCat(__value__)    htonl(__value__)
	typedef int socklen_t;
	#define SHUT_RD   SD_RECEIVE 
	#define SHUT_WR   SD_SEND 
	#define SHUT_RDWR SD_BOTH 
	
	#define Stat64 ::_stat64
	#define LSeeki64 _lseeki64

//////////////////////////////////////////////////
#elif __APPLE__
	#include <unistd.h>
	#include <signal.h>
	#include <dlfcn.h>
	#include <errno.h>
	#include <pwd.h>
	#include <netdb.h> 
	#include <strings.h>
	#include <pthread.h>
	#include <climits>
	
	#include <sys/socket.h>
	#include <sys/mman.h>
	#include <sys/stat.h>
	#include <sys/sysctl.h>
	#include <sys/ioctl.h>
	#include <sys/time.h>
	
	#include <mach/mach.h>
	#include <mach/task.h>
	#include <mach/mach_init.h>
	#include <mach/mach_host.h>
	#include <mach/vm_statistics.h>
	#include <mach/mach_types.h>
	#include <mach-o/dyld.h>
	
	#include <arpa/inet.h>
	#include <dispatch/dispatch.h>
	#include <netinet/tcp.h>
	
	#if __clang__
	void mm_InitAutoReleasePool();
	void mm_DestroyAutoReleasePool();
	#endif
	
	#define getIpCat(__value__)    __value__
	#define closesocket(socketref) ::close(socketref)
	#define Stat64                 ::stat
	#define LSeeki64               ::lseek

//////////////////////////////////////////////////
#else
	
	#ifndef _GNU_SOURCE
	#define _GNU_SOURCE
	#endif
	
	#include <semaphore.h>
	#include <errno.h>
	#include <unistd.h>
	#include <limits.h>
	#include <dlfcn.h>
	#include <pwd.h>
	#include <pthread.h>
	#include <netdb.h> 
	#include <strings.h>
	#include <signal.h>
	#include <sys/sendfile.h>
	#include <sys/socket.h>
	#include <sys/sysinfo.h>
	
	#include <sys/mman.h>
	#include <sys/stat.h>
	#include <sys/types.h>
	#include <sys/ioctl.h>
	#include <sys/time.h>
	
	#include <arpa/inet.h>
	#include <netinet/tcp.h>
	
	#define getIpCat(__value__)    __value__
	#define closesocket(socketref) ::close(socketref)
	#define Stat64                 ::stat
	#define LSeeki64               ::lseek

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

namespace Visus {

/////////////////////////////////////////////////////////////////////////////////////////
class Semaphore::Pimpl
{
public:
#if WIN32
  HANDLE handle;
   Pimpl(int initial_value) {handle = CreateSemaphore(nullptr, initial_value, 0x7FFFFFFF, nullptr); VisusAssert(handle != NULL);}
  ~Pimpl()                  {CloseHandle(handle);}
  void down()               {VisusReleaseAssert(WaitForSingleObject(handle, INFINITE) == WAIT_OBJECT_0);}}
  void up  ()               {VisusReleaseAssert(ReleaseSemaphore(handle, 1, nullptr));}

#elif __APPLE__
  dispatch_semaphore_t sem;
   Pimpl(int initial_value) {sem = dispatch_semaphore_create(initial_value);VisusReleaseAssert(sem);}
  ~Pimpl()                  {dispatch_release(sem);}
  void down()               {VisusReleaseAssert(dispatch_semaphore_wait(this->sem, DISPATCH_TIME_FOREVER) == 0);}
  bool tryDown()            {return dispatch_semaphore_wait(this->sem, DISPATCH_TIME_NOW) == 0;}
  void up()                 {dispatch_semaphore_signal(this->sem);}
  
#else
  sem_t sem;
   Pimpl(int initial_value) {sem_init(&sem, 0, initial_value);}
  ~Pimpl()                  {sem_destroy(&sem);}
  void down()               {while (sem_wait(&sem)== -1) VisusReleaseAssert(errno == EINTR);}
  bool tryDown()            {return sem_trywait(&sem) == 0;}
  void up()                 {VisusReleaseAssert(sem_post(&sem) == 0);}
  
#endif

}; 

/////////////////////////////////////////////////////////////////////////////////////////
class RWLock::Pimpl
{
public:
#if WIN32
  SRWLOCK lock;
  inline Pimpl()           { InitializeSRWLock(&lock); }
  inline void enterRead()  { AcquireSRWLockShared(&lock); }
  inline void exitRead()   { ReleaseSRWLockShared(&lock); }
  inline void enterWrite() { AcquireSRWLockExclusive(&lock); }
  inline void exitWrite()  { ReleaseSRWLockExclusive(&lock); }
#else 
  pthread_rwlock_t lock;
   Pimpl()                 { pthread_rwlock_init(&lock, 0); }
  ~Pimpl()                 { pthread_rwlock_destroy(&lock); }
  inline void enterRead()  { pthread_rwlock_rdlock(&lock); }
  inline void exitRead()   { pthread_rwlock_unlock(&lock); }
  inline void enterWrite() { pthread_rwlock_wrlock(&lock); }
  inline void exitWrite()  { pthread_rwlock_unlock(&lock); }
#endif 
}; //end class

/////////////////////////////////////////////////////////////////////////////////////////
class PosixFile : public File::Pimpl
{
public:


  String filename;
  bool   can_read = false;
  bool   can_write = false;

  int    handle = -1;
  Int64  cursor = -1;

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
  virtual bool open(String filename, String file_mode, File::Options options) override;

  //close
  virtual void close() override;

  //size
  virtual Int64 size() override;

  //write 
  virtual bool write(Int64 pos, Int64 tot, const unsigned char* buffer) override;

  //read
  virtual bool read(Int64 pos, Int64 tot, unsigned char* buffer) override;

  //seek
  bool seek(Int64 value);

  //GetOpenErrorExplanation
  static String GetOpenErrorExplanation();

};

/////////////////////////////////////////////////////////////////////////////////////////
class MemoryMappedFile : public File::Pimpl
{
public:

#if WIN32
  void* file = nullptr;
  void* mapping = nullptr;
#else
  int fd = -1;
#endif

  bool        can_read = false;
  bool        can_write = false;
  String      filename;
  Int64       nbytes = 0;
  char* mem = nullptr;

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
  virtual bool open(String filename, String file_mode, File::Options options) override;

  //close
  virtual void close() override;

  //size
  virtual Int64 size() override {
    return nbytes;
  }

  //write  
  virtual bool write(Int64 pos, Int64 tot, const unsigned char* buffer) override;

  //read
  virtual bool read(Int64 pos, Int64 tot, unsigned char* buffer) override;

};


/////////////////////////////////////////////////////////////////////////////////////////
#if WIN32
class Win32File : public File::Pimpl
{
public:

  VISUS_NON_COPYABLE_CLASS(Win32File)
  
  String filename;
  bool   can_read = false;
  bool   can_write = false;
  HANDLE handle = INVALID_HANDLE_VALUE;
  Int64  cursor = -1;  

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
  virtual bool open(String filename, String file_mode, File::Options options) override;
  
  //close
  virtual void close() override;

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
  virtual Int64 size() override;

  //write  
  virtual bool write(Int64 pos, Int64 tot, const unsigned char* buffer) override;

  //read (should be portable to 32 and 64 bit OS)
  virtual bool read(Int64 pos, Int64 tot, unsigned char* buffer) override;

  //seek
  virtual bool seek(Int64 value);

};
#endif


/// /////////////////////////////////////////////////////////////////////////////////////////
class osdep {

public:

  //getPlatformName
  static String getPlatformName();

  //BreakInDebugger
  static void BreakInDebugger();

  //InitAutoReleasePool
  static void InitAutoReleasePool();

  //DestroyAutoReleasePool
  static void DestroyAutoReleasePool();

  //GetTotalMemory
  static Int64 GetTotalMemory();

  ///GetProcessUsedMemory
  static Int64 GetProcessUsedMemory();

  //GetOsUsedMemory
  static Int64 GetOsUsedMemory();

  //millisToLocal
  static struct tm millisToLocal(const Int64 millis);

  //Utils::
  static double GetRandDouble(double a, double b) ;

  //createDirectory
  static bool createDirectory(String dirname);

  //removeDirectory
  static bool removeDirectory(String value);

  //createLink
  static bool createLink(String existing_file, String new_file);

  //getTimeStamp
  static Int64 getTimeStamp();

  //safe_strerror
  static String safe_strerror(int err);

  //CurrentWorkingDirectory
  static String CurrentWorkingDirectory();

  //PrintMessageToTerminal
  static void PrintMessageToTerminal(const String& value);

  //getHomeDirectory
  static String getHomeDirectory();

  //getCurrentApplicationFile
  static String getCurrentApplicationFile();

  //startup
  static void startup();

}; //end class

}//namespace Visus

#endif //__VISUS_OS_DEP_H__


