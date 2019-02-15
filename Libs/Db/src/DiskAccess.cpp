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

#include <Visus/DiskAccess.h>
#include <Visus/Dataset.h>
#include <Visus/File.h>
#include <Visus/Encoder.h>

#include <cctype>

namespace Visus {

////////////////////////////////////////////////////////////////////
DiskAccess::DiskAccess(Dataset* dataset,StringTree config)
{
  String chmod=config.readString("chmod","rw");
  int default_bitsperblock=dataset->getDefaultBitsPerBlock();

  this->can_read          = StringUtils::find(chmod,"r")>=0;
  this->can_write         = StringUtils::find(chmod,"w")>=0;
  this->path              = Path(config.readString("dir","."));
  this->bitsperblock      = default_bitsperblock;
  this->compression       = config.readString("compression", "lz4");
  this->filename_template = config.readString("filename_template", guessBlockFilenameTemplate());
}


////////////////////////////////////////////////////////////////////
String DiskAccess::getFilename(Field field,double time,BigInt blockid) const
{
  auto ret = guessBlockFilename(this->path.toString(), field, time, blockid, "." + this->compression, this->filename_template);
  VisusAssert(!StringUtils::contains(ret, "$"));
  return ret;
}

////////////////////////////////////////////////////////////////////
void DiskAccess::acquireWriteLock(SharedPtr<BlockQuery> query)
{
  FileUtils::lock(Access::getFilename(query));
}

////////////////////////////////////////////////////////////////////
void DiskAccess::releaseWriteLock(SharedPtr<BlockQuery> query)
{
  FileUtils::unlock(Access::getFilename(query));
}

////////////////////////////////////////////////////////////////////
void DiskAccess::readBlock(SharedPtr<BlockQuery> query)
{
  Int64  blockdim  = query->field.dtype.getByteSize(getSamplesPerBlock());
  String filename  = Access::getFilename(query);

  if (filename.empty())
    return readFailed(query);

  if (query->aborted())
    return readFailed(query);

  auto encoded=std::make_shared<HeapMemory>();
  if (!encoded->resize(FileUtils::getFileSize(filename),__FILE__,__LINE__))
    return readFailed(query);

  File file;
  if (!file.open(filename,"r"))
    return readFailed(query);

  if (!file.read(0,encoded->c_size(), encoded->c_ptr()))
    return readFailed(query);

  auto decoded=ArrayUtils::decodeArray(this->compression,query->nsamples,query->field.dtype, encoded);
  if (!decoded)
    return readFailed(query);

  VisusAssert(decoded.dims==query->nsamples);
  decoded.layout=""; //rowmajor
  query->buffer=decoded;

  return readOk(query);
}


////////////////////////////////////////////////////////////////////
void DiskAccess::writeBlock(SharedPtr<BlockQuery> query)
{
  Int64  blockdim        = query->field.dtype.getByteSize(getSamplesPerBlock());
  String filename        = Access::getFilename(query);

  if (filename.empty() || query->buffer.c_size()!=blockdim)
    return writeFailed(query);

  //only RowMajor is supported!
  auto layout=query->buffer.layout;
  if (!layout.empty())
  {
    VisusInfo()<<"Failed to write block, only RowMajor format is supported";
    return writeFailed(query);
  }

  if (query->aborted())
    return writeFailed(query);

  FileUtils::removeFile(filename);

  File file;
  if (!file.createAndOpen(filename,"w"))
  {
    VisusInfo()<<"Failed to write block filename("<<filename<<") cannot create file and/or directory";
    return writeFailed(query);
  }

  auto decoded=query->buffer;
  auto encoded=ArrayUtils::encodeArray(this->compression,decoded);
  if (!encoded)
  {
    VisusInfo()<<"Failed to write block filename("<<filename<<") file.write failed";
    return writeFailed(query);
  }

  if (!file.write(0, encoded->c_size(), encoded->c_ptr()))
  {
    VisusInfo()<<"Failed to write block filename("<<filename<<") compression or file.write failed";
    return writeFailed(query);
  }

  return writeOk(query);
}

} //namespace Visus







  

