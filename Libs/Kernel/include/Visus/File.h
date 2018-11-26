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

#ifndef __VISUS_FILE_H__
#define __VISUS_FILE_H__

#include <Visus/Kernel.h>
#include <Visus/Path.h>
#include <Visus/Log.h>

namespace Visus {


/////////////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API File
{
public:

  enum Options
  {
    NoOptions=0,
    MustCreateFile=0x01,
    PreferMemoryMapping=0x02,
    PreferWin32Api=0x04
  };

  //__________________________________________________________________
#if !SWIG
  class VISUS_KERNEL_API Pimpl
  {
  public:

    VISUS_NON_COPYABLE_CLASS(Pimpl)

    //constructor
    Pimpl() {
    }

    //destructor
    virtual ~Pimpl() {
    }

    //isOpen
    virtual bool isOpen() const = 0;

    //open
    virtual bool open(String filename, String mode, Options options) = 0;

    //close
    virtual void close() = 0;

    //size
    virtual Int64 size() = 0;

    //canRead
    virtual bool canRead() const = 0;

    //canWrite
    virtual bool canWrite() const = 0;

    //getFilename
    virtual String getFilename() const = 0;

    //write  
    virtual bool write(Int64 pos, Int64 count, const unsigned char* buffer) = 0;

    //read (should be portable to 32 and 64 bit OS)
    virtual bool read(Int64 pos, Int64 count, unsigned char* buffer) = 0;

  };
#endif

  //constructor
  File() {
  }

  //destructor
  virtual ~File() {
  }

  //isOpen
  bool isOpen() const  {
    return pimpl ? true : false;
  }

  //open
  bool open(String filename, String mode) {
    return open(filename, mode, NoOptions);
  }

  //createAndOpen (return false if already exists)
  bool createAndOpen(String filename, String mode) {
    return open(filename, mode, MustCreateFile);
  }

  //close
  void close()  {
    pimpl.reset();
  }

  //size
  Int64 size()  {
    return pimpl ? pimpl->size() : -1;
  }

  //canRead
  bool canRead() const  {
    return pimpl ? pimpl->canRead() : false;
  }

  //canWrite
  bool canWrite() const  {
    return pimpl ? pimpl->canWrite() : false;
  }

  //getMode
  String getMode() const {
    auto can_read = canRead();
    auto can_write = canWrite();
    return can_read && can_write ? "rw" : (can_read ? "r" : (can_write ? "w" : ""));
  }


  //getFilename
  String getFilename() const  {
    return pimpl ? pimpl->getFilename() : String("");
  }

  //write  
  bool write(Int64 pos, Int64 count, const unsigned char* buffer)  {
    return pimpl ? pimpl->write(pos, count, buffer) : false;
  }

  //read (should be portable to 32 and 64 bit OS)
  bool read(Int64 pos, Int64 count, unsigned char* buffer)  {
    return pimpl ? pimpl->read(pos, count, buffer) : false;
  }

private:

  UniquePtr<Pimpl> pimpl;

  //open
  bool open(String filename, String mode, Options options);

};




////////////////////////////////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API FileUtils
{
public:

  //test existance
  static bool existsFile(Path path);
  static bool existsDirectory(Path path);

  //remove file
  static bool removeFile(Path path);

  //mkdir/rmdir
  static bool createDirectory(Path path,bool bCreateParents=true);
  static bool removeDirectory(Path path);
  
  //return the size of the file
  static Int64 getFileSize(Path path);

  //getTimeLastModified
  static Int64 getTimeLastModified(Path path);

  //getTimeLastAccessed
  static Int64 getTimeLastAccessed(Path path);

  //special lock/unlock functions
  static void lock  (Path path);
  static void unlock(Path path);

  //touch
  static bool touch(Path path);

  //copyFile
  static bool copyFile(String src, String dst, bool bFailIfExist);

  //moveFile
  static bool moveFile(String src, String dst);

  //createLink
  static bool createLink(String existing_file, String new_file);

private:

  FileUtils()=delete;

};


} //namespace Visus

#endif //__VISUS_FILE_IO_H__
