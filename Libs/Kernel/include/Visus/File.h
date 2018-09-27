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

////////////////////////////////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API File
{
public:

  VISUS_NON_COPYABLE_CLASS(File)

  enum CreateMode
  {
    DoNotCreate,
    TruncateIfExists = 1,
    MustCreate = 2
  };


  //constructor
  File(){
  }

  //destructor
  ~File() {
    close();
  }

  //isOpen
  bool isOpen() const {
    return handle != -1;
  }

  //getFilename
  String getFilename() const {
    return filename;
  }

  //open
  bool open(String filename, String mode, CreateMode create_mode = DoNotCreate);

  //openReadBinary ("rb")
  bool openReadBinary(String filename) {
    return open(filename, "r");
  }

  //openReadWriteBinary
  bool openReadWriteBinary(String filename) {
    return open(filename, "rw");
  }

  //createOrTruncateAndWriteBinary ("wb") == if the file does exists, truncate it to zero length, if the file does not exists, create it
  bool createOrTruncateAndWriteBinary(String filename) {
    return open(filename, "w", TruncateIfExists);
  }

  //createAndOpenReadWriteBinary (return false if already exists)
  bool createAndOpenReadWriteBinary(String filename) {
    return open(filename, "rw", MustCreate);
  }

  //canRead
  bool canRead() const {
    return can_read;
  }

  //canWrite
  bool canWrite() const {
    return can_write;
  }

  //getMode
  String getMode() const {
    std::ostringstream out;
    if (can_read ) out << "r";
    if (can_write) out << "w";
    return out.str();
  }

  //close
  void close();

  //getCursor
  Int64 getCursor();

  //setCursor
  bool setCursor(Int64 offset);

  //gotoEnd
  bool gotoEnd();

  //write 
  bool write(const unsigned char* buffer, Int64 count);

  //read (should be portable to 32 and 64 bit OS)
  bool read(unsigned char* buffer, Int64 count);

  //seekAndWrite
  bool seekAndWrite(Int64 pos, Int64 count, unsigned char* buffer) {
    return isOpen() && count >= 0 && setCursor(pos) && write(buffer, count);
  }

  //seekAndRead
  bool seekAndRead(Int64 pos, Int64 count, unsigned char* buffer){
    return isOpen() && count >= 0 && setCursor(pos) && read(buffer, count);
  }

private:

  int  handle=-1;
  bool can_read=false;
  bool can_write=false;
  String filename;
  Int64 cursor = -1;

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
