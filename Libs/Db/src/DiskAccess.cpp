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
  this->bitsperblock      = default_bitsperblock;
  this->compression       = config.readString("compression", Url(dataset->getUrl()).getParam("compression", "zip"));

  Url url = config.hasAttribute("url") ? config.readString("url") : dataset->getUrl();
  Path path = Path(url.getPath());
  
  //example: "s3://bucket-name/whatever/$(time)/$(field)/$(block:%016x:%04x).$(compression)";
  //NOTE 16x is enough for 16*4 bits==64 bit for block number
  //     splitting by 4 means 2^16= 64K files inside a directory with max 64/16=4 levels of directories
  this->filename_template = config.readString("filename_template", 
    url.isRemote() ? 
      "$(VisusCache)/$(HostName)/$(HostPort)$(FullPathWithoutExt)/$(time)/$(field)/$(block:%016x:%04x).bin" :
                                           "$(FullPathWithoutExt)/$(time)/$(field)/$(block:%016x:%04x).bin");
 
  this->filename_template = StringUtils::replaceAll(this->filename_template, "$(HostName)", url.getHostname());
  this->filename_template = StringUtils::replaceAll(this->filename_template, "$(HostPort)", cstring(url.getPort()));
  this->filename_template = StringUtils::replaceAll(this->filename_template, "$(FullPathWithoutExt)", path.withoutExtension());
  this->filename_template = StringUtils::replaceAll(this->filename_template, "$(VisusCache)", GetVisusCache());

  //PrintInfo("Created DiskAccess","url",url,"filename_template",filename_template,"compression", compression);
}


////////////////////////////////////////////////////////////////////
String DiskAccess::getFilename(Field field,double time,BigInt blockid) const
{
  auto reverse_filename = false;
  auto compression = guessCompression(field);
  return Access::getBlockFilename(filename_template, field, time, compression, blockid, reverse_filename);
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

  auto compression = guessCompression(query->field);

  auto nsamples = query->getNumberOfSamples();
  auto decoded=ArrayUtils::decodeArray(compression,nsamples,query->field.dtype, encoded);
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
  String filename = Access::getFilename(query);

  if (filename.empty())
    return writeFailed(query,"filename is empty");

  if (query->aborted())
    return writeFailed(query, "query aborted");

  //relaxing a little for VISUS_IDX2 (until I have the layout)
#if 0
    Int64  blockdim = query->field.dtype.getByteSize(getSamplesPerBlock());
    if (query->buffer.c_size() != blockdim)
      return writeFailed(query, "wrong buffer");

    //only RowMajor is supported!
    auto layout = query->buffer.layout;
    if (!(layout.empty() || layout == "rowmajor"))
    {
      PrintInfo("Failed to write block, only RowMajor format is supported");
      return writeFailed(query, "only raw major format is supported");
    }
#endif

  FileUtils::removeFile(filename);

  File file;
  if (!file.createAndOpen(filename,"w"))
  {
    PrintInfo("Failed to write block filename", filename, "cannot create file and/or directory");
    return writeFailed(query,"cannot create file or directory");
  }

  auto compression = guessCompression(query->field);

  auto decoded=query->buffer;
  auto encoded=ArrayUtils::encodeArray(compression,decoded);
  if (!encoded)
  {
    PrintInfo("Failed to write block filename", filename, "encodeArray failed");
    return writeFailed(query, "Failed to encode data");
  }

  if (!file.write(0, encoded->c_size(), encoded->c_ptr()))
  {
    PrintInfo("Failed to write block filename", filename, "file.write failed");
    return writeFailed(query,"failed to write encoded data");
  }

  return writeOk(query);
}

} //namespace Visus







  

