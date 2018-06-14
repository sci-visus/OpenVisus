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

#include <Visus/File.h>
#include <Visus/Thread.h>
#include <Visus/Utils.h>
#include <Visus/ApplicationStats.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#if WIN32
  #include <io.h>
  #include <Windows.h>
  #include <direct.h>
  #include <process.h>
  #include <sddl.h>
  #pragma warning(disable:4996)
  #pragma comment(lib, "advapi32.lib")

#elif __APPLE__
  #include <unistd.h>
  #include <sys/socket.h>

#else
  #include <unistd.h>
  #include <limits.h>
  #include <sys/sendfile.h>

#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif 

namespace Visus {

/////////////////////////////////////////////////////////////////////////
#ifdef WIN32
static std::string FormatErrorMessage(DWORD ErrorCode)
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

/////////////////////////////////////////////////////////////////////////
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
    return StringUtils::format() << "Unknown errno(" << errno << ")";
  }
}


/////////////////////////////////////////////////////////////////////////
bool File::open(String filename,String mode,CreateMode create_mode)
{
  close();

  bool bRead  = StringUtils::contains(mode, "r");
  bool bWrite = StringUtils::contains(mode, "w");

  int imode = O_BINARY;

  if (bRead && bWrite)
    imode |= O_RDWR;

  else if (bRead)
    imode |= O_RDONLY;

  else if (bWrite)
    imode |= O_WRONLY;

  else {
    VisusAssert(false);
    return false;
  }

  int create_flags = 0;

  if (create_mode)
  {
    imode |= O_CREAT;

    VisusAssert(create_mode == TruncateIfExists || create_mode == MustCreate);

    if (create_mode & TruncateIfExists)
      imode |= O_TRUNC;

    if (create_mode & MustCreate)
      imode |= O_EXCL;

#if WIN32
    create_flags |= (S_IREAD | S_IWRITE);
#else
    create_flags |= (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
#endif
  }

#if WIN32
  this->handle = ::_open(filename.c_str(), imode, create_flags);
#else
  this->handle = ::open(filename.c_str(), imode, create_flags);
#endif     

  if (!isOpen())
  {
    if (create_mode)
    {
      FileUtils::createDirectory(Path(filename).getParent());

#if WIN32
      this->handle = ::_open(filename.c_str(), imode, create_flags);
#else
      this->handle = ::open(filename.c_str(), imode, create_flags);
#endif   

      if (!isOpen())
        return false;
    }
    else
    {
      if (errno != ENOENT)
        VisusWarning() << "Thread[" << Thread::getThreadId() << "] ERROR opening file " << GetOpenErrorExplanation();

      return false;
    }
  }

  this->can_read = bRead;
  this->can_write= bWrite;
  this->filename = filename;

  ApplicationStats::io.trackOpen();
  
  return true;
}


/////////////////////////////////////////////////////////////////////////
void File::close()
{
  if (this->handle!=-1)
  {
#if WIN32
    ::_close(this->handle);
#else
    ::close(this->handle);
#endif     

    this->handle    = -1;
  }

  this->can_read  = false;
  this->can_write = false;
  this->filename="";
}

/////////////////////////////////////////////////////////////////////////
Int64 File::tell() 
{
  return seek(0,SEEK_CUR);
}

/////////////////////////////////////////////////////////////////////////
Int64 File::seek(Int64 offset,int origin)
{
  if (!isOpen())
    return -1;
#if WIN32
  return ::_lseeki64(this->handle, offset, origin);
#else
  return ::lseek(this->handle, offset, origin);
#endif
}

/////////////////////////////////////////////////////////////////////////
bool File::write(const unsigned char* buffer,Int64 wbytes)
{
  if (!isOpen() || wbytes<0 || !canWrite()) 
    return false;
  
  for (Int64 remaining=wbytes;remaining;)
  {
    int chunk=(remaining>=INT_MAX)? INT_MAX : (int)remaining;

#if WIN32
    int n = ::_write(this->handle, buffer, chunk);
#else
    int n = ::write(this->handle, buffer, chunk);
#endif 

    if (n<=0) 
      return false;
    
    remaining-=n;
    buffer+=n;
  }

  ApplicationStats::io.trackWriteOperation(wbytes);
  return true;
}

/////////////////////////////////////////////////////////////////////////
bool File::read(unsigned char* buffer,Int64 rbytes)
{
  if (!isOpen() || rbytes<0 || !canRead()) 
    return false;
  
  for (Int64 remaining=rbytes;remaining;)
  {
    int chunk=(remaining>=INT_MAX)? INT_MAX : (int)remaining;

#if WIN32
    int n = ::_read(this->handle, buffer, chunk);
#else
    int n = ::read(this->handle, buffer, chunk);
#endif     

    if (n<=0) 
      return false; 

    remaining-=n;
    buffer+=n;
  }

  ApplicationStats::io.trackReadOperation(rbytes);
  return true;
}

#if WIN32
#define PimplStat ::_stat64
#else
#define PimplStat ::stat
#endif

/////////////////////////////////////////////////////////////////////////
bool FileUtils::existsDirectory(Path path)
{
  if (path.empty()) 
    return false;

  //special case [/] | [c:]
  if (path.isRootDirectory())
    return true;

  String fullpath=path.toString();

  struct PimplStat status;
  return PimplStat(fullpath.c_str(),&status)==0 && (status.st_mode & S_IFDIR)==S_IFDIR;
}

/////////////////////////////////////////////////////////////////////////
bool FileUtils::existsFile(Path path)
{
  if (path.empty()) 
    return false;

  String fullpath=path.toString();

  //TODO: probably here i need to specific test if it's a regular file or symbolic link
  struct PimplStat status;
  return PimplStat(fullpath.c_str(),&status)==0 && (status.st_mode & S_IFDIR)!=S_IFDIR;
}

/////////////////////////////////////////////////////////////////////////
Int64 FileUtils::getFileSize(Path path)
{
  if (path.empty()) 
    return false;

  String fullpath=path.toString();

  struct PimplStat status;
  if (PimplStat(fullpath.c_str(),&status)!=0 || (status.st_mode & S_IFDIR)==S_IFDIR) return 0;
  return status.st_size;

}

/////////////////////////////////////////////////////////////////////////
Int64 FileUtils::getTimeLastModified(Path path)
{
  if (path.empty()) 
    return false;

  String fullpath=path.toString();

  struct PimplStat status;
  if (PimplStat(fullpath.c_str(),&status)!=0 || (status.st_mode & S_IFDIR)==S_IFDIR) return 0;
  return static_cast<Int64>(status.st_mtime);
}

/////////////////////////////////////////////////////////////////////////
Int64 FileUtils::getTimeLastAccessed(Path path)
{
  if (path.empty()) 
    return false;

  String fullpath=path.toString();

  struct PimplStat status;
  if (PimplStat(fullpath.c_str(),&status)!=0 || (status.st_mode & S_IFDIR)==S_IFDIR) return 0;
  return static_cast<Int64>(status.st_atime);
}

/////////////////////////////////////////////////////////////////////////
bool FileUtils::removeFile(Path path)
{
  if (path.empty()) 
    return false;

  String fullpath=path.toString();

  return ::remove(fullpath.c_str())==0?true:false;
}

/////////////////////////////////////////////////////////////////////////
bool FileUtils::createDirectory(Path path,bool bCreateParents)
{
  //path invalid or already exists
  if (path.empty() || existsDirectory(path))
    return false;

  //need to create my parent
  if (bCreateParents)
  {
    Path parent=path.getParent(); VisusAssert(!parent.empty());
    if (!existsDirectory(parent) && !createDirectory(parent,true))
      return false;
  }

  auto dirname = path.toString();

#if WIN32
  return CreateDirectory(TEXT(dirname.c_str()), NULL)!=0;
#else
  return ::mkdir(dirname.c_str(), 0775) == 0; //user(rwx) group(rwx) others(r-x)
#endif
}

/////////////////////////////////////////////////////////////////////////
bool FileUtils::removeDirectory(Path path)
{
  if (path.empty()) 
    return false;

  String fullpath=path.toString();

#if WIN32
  return ::_rmdir(fullpath.c_str()) == 0 ? true : false;
#else
  return ::rmdir(fullpath.c_str()) == 0 ? true : false;
#endif
}

/////////////////////////////////////////////////////////////////////////
void FileUtils::lock(Path path)
{
  VisusAssert(!path.empty());
  String fullpath=path.toString();

  //trying to make this operation as fast as possible
  const bool bWritePid=false; 

#if WIN32
  int pid = ::_getpid();
#else
  int pid = ::getpid();
#endif

  String lock_filename=fullpath+ ".lock";

  //let's try a little more
  Time T1=Time::now();
  Time last_info_time=T1;
  bool bPrintOnReturn=false;
  for (int nattempt=0; ;nattempt++)
  {
    File file;
    if (file.createAndOpenReadWriteBinary(lock_filename))
    {
      //write the pid
      if (bWritePid) 
        file.write((unsigned char*)&pid,sizeof(pid)); 

      file.close();

      if (bPrintOnReturn) 
        VisusInfo()<<"[PID="<<pid<<"] got file lock "<<lock_filename;

      return;
    }

    //let the user know that I'm still waiting
    if (last_info_time.elapsedMsec()>1000)
    {
      VisusInfo()<<"[PID="<<pid<<"] waiting for lock on "<<lock_filename;
      last_info_time=Time::now();
      bPrintOnReturn=true;
    }

    Thread::yield();
  }
}

/////////////////////////////////////////////////////////////////////////
void FileUtils::unlock(Path path)
{
  VisusAssert(!path.empty());

  String fullpath = path.toString();

  String lock_filename = fullpath + ".lock";
  bool bRemoved = ::remove(lock_filename.c_str()) == 0 ? true : false;
  if (!bRemoved)
  {
    String msg = StringUtils::format() << "cannot remove lock file " << lock_filename;
    ThrowException(msg);
  }
}


/////////////////////////////////////////////////////////////////////////
bool FileUtils::copyFile(String src_filename, String dst_filename, bool bFailIfExist)
{
#if 0
  if (CopyFile(src_filename.c_str(), dst_filename.c_str(), bFailIfExist) == 0) {
    VisusWarning() << "Error copying file " << FormatErrorMessage(GetLastError());
    return false;
  }
  return true;
#endif

  int buffer_size = 1024 * 1024; //1 Mb
  char* buffer=new char[buffer_size];
  
  int src = ::open(src_filename.c_str(), O_RDONLY | O_BINARY, 0);
  if (src == -1) {
    delete [] buffer;
    return false;
  }

  int dst = ::open(dst_filename.c_str(), O_WRONLY | O_CREAT | (bFailIfExist ? 0 : O_TRUNC), 0644);
  if (dst == -1) {
    delete [] buffer;
    return false;
  }

  int size;
  while ((size = ::read(src, buffer, buffer_size)) > 0)
  {
    if (::write(dst, buffer, size)!=size)
      return false;
  }

  close(src);
  close(dst);

  delete [] buffer;
  return true;
}

/////////////////////////////////////////////////////////////////////////
bool FileUtils::moveFile(String src_filename, String dst_filename)
{
#ifdef WIN32

  if  (MoveFile(src_filename.c_str(), dst_filename.c_str()) == 0)
  {
    VisusWarning() << "Error moving file " << FormatErrorMessage(GetLastError());
    return false;
  }
  return true;

#else
  return rename(src_filename.c_str(), dst_filename.c_str()) == 0;
#endif
}

/////////////////////////////////////////////////////////////////////////
bool FileUtils::createLink(String existing_file, String new_file)
{
#ifdef WIN32

  if (CreateHardLink(new_file.c_str(), existing_file.c_str(), nullptr) == 0)
  {
    VisusWarning()<<"Error creating link "<<FormatErrorMessage(GetLastError());
    return false;
  }

  return true;

#else
  return symlink(existing_file.c_str(), new_file.c_str()) == 0;
#endif
}


} //namespace Visus
