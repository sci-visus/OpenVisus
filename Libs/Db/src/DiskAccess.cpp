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
#include <Visus/Encoders.h>

#include <cctype>

namespace Visus {

////////////////////////////////////////////////////////////////////
DiskAccess::DiskAccess(Dataset* dataset,StringTree config)
{
  String chmod=config.readString("chmod","rw");
  int default_bitsperblock=dataset->getDefaultBitsPerBlock();

  this->can_read     = StringUtils::find(chmod,"r")>=0;
  this->can_write    = StringUtils::find(chmod,"w")>=0;
  this->path         = Path(config.readString("dir","./"));
  this->bitsperblock = default_bitsperblock;
  this->compression_type  = config.readString("compression","zip");
}

////////////////////////////////////////////////////////////////////
static inline String encode(String value) 
{
  for (int I = 0; I < (int)value.size(); I++) 
  {
    if (!std::isalnum(value[I]))
        value[I] = '_';
  }
  return value;
}

////////////////////////////////////////////////////////////////////
String DiskAccess::getFilename(Field field,double time,BigInt blockid) const
{
  std::ostringstream out;
  out<<this->path.toString();

  //time
  {
    out<<"/"<<"time_"<<encode(cstring(time));
  }

  //fieldname
  {
    String fieldname=StringUtils::removeSpaces(field.name); VisusAssert(!fieldname.empty());
    
    //if the fieldname is not too big
    if (fieldname.length()<32)
      out<<"/"<<encode(fieldname);
    else
      out<<"/"<<StringUtils::computeChecksum(fieldname);
  }

  //blocknum
  {
    std::ostringstream out_blockid;
    out_blockid<<std::hex << std::setw(32) << std::setfill('0') << blockid;
    String blockid=out_blockid.str();
    out<<"/";
    int N=0;for (;N<(int)blockid.size()-4;N+=4)
      out<<blockid.substr(N,4)+"/";
    out<<blockid.substr(N);
  }
  
  //compression
  {
    out<<"."<<compression_type;
  }

  return out.str();

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
  if (!file.openReadBinary(filename.c_str()))
    return readFailed(query);

  if (!file.read(encoded->c_ptr(),encoded->c_size()))
    return readFailed(query);

  String compression=this->compression_type;

  auto decoded=ArrayUtils::decodeArray(compression,query->nsamples,query->field.dtype,encoded);
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

  File file;
  if (!file.createOrTruncateAndWriteBinary(filename.c_str()))
  {
    VisusInfo()<<"Failed to write block filename("<<filename<<") cannot create file and/or directory";
    return writeFailed(query);
  }

  auto decoded=query->buffer;
  auto encoded=ArrayUtils::encodeArray(compression_type,decoded);
  if (!encoded)
  {
    VisusInfo()<<"Failed to write block filename("<<filename<<") file.write failed";
    return writeFailed(query);
  }

  if (!file.write(encoded->c_ptr(),encoded->c_size()))
  {
    VisusInfo()<<"Failed to write block filename("<<filename<<") compression or file.write failed";
    return writeFailed(query);
  }

  return writeOk(query);
}

} //namespace Visus







  

