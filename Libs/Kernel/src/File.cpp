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
#include <Visus/Time.h>
#include "Os.hxx"

namespace Visus {


/////////////////////////////////////////////////////////////////////////
bool File::open(String filename, String file_mode, Options options)
{
  close();

  //NOTE fopen/fclose is even slower than _open/_close
  //pimpl.reset(new Win32File()); //don't see any advantage using Win32File
  //pimpl.reset(new MemoryMappedFile()); THIS IS THE SLOWEST
  pimpl.reset(new PosixFile());

  if (!pimpl->open(filename, file_mode, options)) {
    pimpl.reset();
    return false;
  }

  return true;
}

/////////////////////////////////////////////////////////////////////////
bool FileUtils::existsDirectory(Path path)
{
  if (path.empty()) 
    return false;

  //special case [/] | [c:]
  if (path.isRootDirectory())
    return true;

  String fullpath=path.toString();

  struct Stat64 status;
  if (Stat64(fullpath.c_str(), &status) != 0)
    return false;


  if (!S_ISDIR(status.st_mode))
    return false;

  return true;
}

/////////////////////////////////////////////////////////////////////////
bool FileUtils::existsFile(Path path)
{
  if (path.empty()) 
    return false;

  String fullpath=path.toString();

  struct Stat64 status;

  if (Stat64(fullpath.c_str(), &status) != 0)
    return false;

  //TODO: probably here i need to specific test if it's a regular file or symbolic link
  if (!S_ISREG(status.st_mode))
    return false;

  return  true;
}

/////////////////////////////////////////////////////////////////////////
Int64 FileUtils::getFileSize(Path path)
{
  if (path.empty()) 
    return false;

  String fullpath=path.toString();

  struct Stat64 status;
  if (Stat64(fullpath.c_str(), &status) != 0)
    return 0;

  return status.st_size;

}

/////////////////////////////////////////////////////////////////////////
Int64 FileUtils::getTimeLastModified(Path path)
{
  if (path.empty()) 
    return false;

  String fullpath=path.toString();

  struct Stat64 status;
  if (Stat64(fullpath.c_str(), &status) != 0)
    return 0;

  return static_cast<Int64>(status.st_mtime);
}

/////////////////////////////////////////////////////////////////////////
Int64 FileUtils::getTimeLastAccessed(Path path)
{
  if (path.empty()) 
    return false;

  String fullpath=path.toString();

  struct Stat64 status;
  if (Stat64(fullpath.c_str(), &status) != 0)
    return 0;

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
bool FileUtils::touch(Path path)
{
  File file;
  return file.createAndOpen(path.toString(), "rw");
}

/////////////////////////////////////////////////////////////////////////
void FileUtils::lock(Path path)
{
  VisusAssert(!path.empty());
  String fullpath=path.toString();

#if WIN32
  int pid = ::_getpid();
#else
  int pid = ::getpid();
#endif

  String lock_filename=fullpath+ ".lock";

  //let's try a little more
  Time T1=Time::now();
  Time last_info_time=T1;
  bool bVerboseReturn=false;
  for (int nattempt=0; ;nattempt++)
  {
    File file;
    if (file.createAndOpen(lock_filename, "rw"))
    {
      file.close();

      if (bVerboseReturn)
        PrintInfo("PID",pid,"got file lock",lock_filename);

      return;
    }

    //let the user know that I'm still waiting
    if (last_info_time.elapsedMsec()>1000)
    {
      PrintInfo("PID",pid,"waiting for lock on",lock_filename);
      last_info_time=Time::now();
      bVerboseReturn =true;
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
    ThrowException("cannot remove lock file",lock_filename);
}

/////////////////////////////////////////////////////////////////////////
bool FileUtils::copyFile(String src_filename, String dst_filename, bool bFailIfExist)
{
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
  return std::rename(src_filename.c_str(), dst_filename.c_str()) == 0;
}

/////////////////////////////////////////////////////////////////////////
bool FileUtils::createLink(String existing_file, String new_file)
{
#ifdef WIN32
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

} //namespace Visus
