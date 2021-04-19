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
  int default_bitsperblock=dataset->getDefaultBitsPerBlock();
  this->can_read          = StringUtils::find(config.readString("chmod", DefaultChMod),"r")>=0;
  this->can_write         = StringUtils::find(config.readString("chmod", DefaultChMod),"w")>=0;
  this->path              = Path(config.readString("dir","."));
  this->bitsperblock      = default_bitsperblock;
  this->compression       = config.readString("compression", "lz4");
  this->filename_template = config.readString("filename_template", "$(prefix)/time_$(time)/$(field)/$(block).$(compression)");
}


////////////////////////////////////////////////////////////////////
String DiskAccess::getFilename(Field field,double time,BigInt blockid) const
{
  String fieldname = StringUtils::removeSpaces(field.name);
  String ret = filename_template;
  ret = StringUtils::replaceFirst(ret, "$(prefix)", this->path.toString());
  ret = StringUtils::replaceFirst(ret, "$(time)", StringUtils::onlyAlNum(int(time) == time ? cstring((int)time) : cstring(time)));
  ret = StringUtils::replaceFirst(ret, "$(field)", fieldname.length() < 32 ? StringUtils::onlyAlNum(fieldname) : StringUtils::computeChecksum(fieldname));
  ret = StringUtils::replaceFirst(ret, "$(block)", StringUtils::join(StringUtils::splitInChunks(StringUtils::formatNumber("%032x", blockid), 4), "/"));
  ret = StringUtils::replaceFirst(ret, "$(compression)", this->compression);
  VisusAssert(!StringUtils::contains(ret, "$"));
  return ret;
}

////////////////////////////////////////////////////////////////////
void DiskAccess::acquireWriteLock(SharedPtr<BlockQuery> query)
{
  VisusAssert(isWriting());
  if (bDisableWriteLocks) return;
  FileUtils::lock(Access::getFilename(query));
}

////////////////////////////////////////////////////////////////////
void DiskAccess::releaseWriteLock(SharedPtr<BlockQuery> query)
{
  VisusAssert(isWriting());
  if (bDisableWriteLocks) return;
  FileUtils::unlock(Access::getFilename(query));
}

////////////////////////////////////////////////////////////////////
void DiskAccess::readBlock(SharedPtr<BlockQuery> query)
{
  Int64  blockdim  = query->field.dtype.getByteSize(getSamplesPerBlock());
  String filename  = Access::getFilename(query);

  if (filename.empty())
    return readFailed(query,"filename empty");

  if (query->aborted())
    return readFailed(query,"query aborted");

  auto encoded=std::make_shared<HeapMemory>();
  if (!encoded->resize(FileUtils::getFileSize(filename),__FILE__,__LINE__))
    return readFailed(query,"cannot create encoded buffer");

  File file;
  if (!file.open(filename,"r"))
    return readFailed(query,cstring("cannot open file", filename));

  if (!file.read(0,encoded->c_size(), encoded->c_ptr()))
    return readFailed(query,"cannot read encoded data");

  auto nsamples = query->getNumberOfSamples();
  auto decoded=ArrayUtils::decodeArray(this->compression,nsamples,query->field.dtype, encoded);
  if (!decoded.valid())
    return readFailed(query,"cannot decode data");

  VisusAssert(decoded.dims==query->getNumberOfSamples());
  decoded.layout=""; 
  query->buffer=decoded;

  return readOk(query);
}


////////////////////////////////////////////////////////////////////
void DiskAccess::writeBlock(SharedPtr<BlockQuery> query)
{
  Int64  blockdim        = query->field.dtype.getByteSize(getSamplesPerBlock());
  String filename        = Access::getFilename(query);

  if (filename.empty())
    return writeFailed(query,"filename is empty");
    
  if (query->buffer.c_size()!=blockdim)
    return writeFailed(query,"wrong buffer");

  //only RowMajor is supported!
  auto layout=query->buffer.layout;
  if (!layout.empty())
  {
    PrintInfo("Failed to write block, only RowMajor format is supported");
    return writeFailed(query,"only raw major format is supported");
  }

  if (query->aborted())
    return writeFailed(query,"query aborted");

  FileUtils::removeFile(filename);

  File file;
  if (!file.createAndOpen(filename,"w"))
  {
    PrintInfo("Failed to write block filename", filename, "cannot create file and/or directory");
    return writeFailed(query,"cannot create file or directory");
  }

  auto decoded=query->buffer;
  auto encoded=ArrayUtils::encodeArray(this->compression,decoded);
  if (!encoded)
  {
    PrintInfo("Failed to write block filename", filename, "file.write failed");
    return writeFailed(query, "Failed to encode data");
  }

  if (!file.write(0, encoded->c_size(), encoded->c_ptr()))
  {
    PrintInfo("Failed to write block filename", filename, "compression or file.write failed");
    return writeFailed(query,"failed to write encoded data");
  }

  return writeOk(query);
}

} //namespace Visus







  

