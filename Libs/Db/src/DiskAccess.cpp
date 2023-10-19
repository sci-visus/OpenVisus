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
  //caching
  //DiskAccess     (*) Access::compression (*) field.default_compression

  this->dataset = dataset;
  this->idxfile = dataset->idxfile;
  this->name = config.readString("name", "DiskAccess");
  this->can_read = StringUtils::find(config.readString("chmod", DefaultChMod), "r") >= 0;
  this->can_write = StringUtils::find(config.readString("chmod", DefaultChMod), "w") >= 0;
  this->bitsperblock = this->idxfile.bitsperblock;
  this->compression = config.readString("compression");

  Url url = config.readString("url", dataset->getUrl());
  VisusReleaseAssert(url.valid());

  auto blob_extension = config.readString("blob_extension", url.getParam("blob_extension", ".bin"));

  String local_idx_filename;
  if (url.isRemote())
  {
    if (this->compression.empty())
      this->compression = "raw";

    //automatic guess local *.idx filename for caching
    std::ostringstream out;
    out
      << config.readString("cache_dir", GetVisusCache()) << "/"
      << "DiskAccess" << "/"
      << url.getHostname() << "/"
      << url.getPort() << "/"
      << compression;

    if (StringUtils::contains(url.toString(), "mod_visus"))
      out << url.getPath() << "/" << url.getParam("dataset") << "/" << "visus.idx";
    else
      out << url.getPath(); //cloud storage, path should be unique and visus.idx is ALREADY at the end of the  the path for cloud storage (!)

    local_idx_filename = out.str();
  }
  else
  {
    VisusReleaseAssert(url.isFile());
    local_idx_filename = url.getPath();
  }

  if (!FileUtils::existsFile(local_idx_filename))
  {
    //automatically create the *.idx file, useful for caching
    auto  tmp = this->idxfile;
    tmp.version = 0;                                                              //need to fill put for the validate step
    tmp.blocksperfile = 1;                                                        //one file per block
    tmp.arco = std::max(tmp.arco, (1 << bitsperblock) * tmp.getMaxFieldSize());   //force arco
    tmp.setDefaultCompression(compression);                                       //"" will be equivalent to raw/uncompressed

    tmp.save(local_idx_filename);
    PrintInfo("DiskAccess creating IdxFile at", local_idx_filename);
    this->idxfile = tmp;
  }
  else
  {
    PrintInfo("DiskAccess using existing IdxFile at", local_idx_filename);

    //need to load it again since it can be different from the dataset
    if (local_idx_filename != dataset->getUrl())
    {
      this->idxfile = IdxFile();
      this->idxfile.load(local_idx_filename);
    }
  }
  VisusAssert(this->idxfile.version >= 1 && idxfile.version <= 6);


  //NOTE I am ignoring what it is stored in the idx file, but using filename template to guess filenames
  //example: "s3://bucket-name/whatever/$(time)/$(field)/$(block:%016x:%04x).$(compression)";
  //NOTE 16x is enough for 16*4 bits==64 bit for block number
  //     splitting by 4 means 2^16= 64K files inside a directory with max 64/16=4 levels of directories
  this->filename_template = config.readString("filename_template", Path(local_idx_filename).withoutExtension() + "/$(time)/$(field)/$(block:%016x:%04x)" + blob_extension);

  //0 == no verbose
  //1 == read verbose, write verbose
  //2 ==               write verbose
  this->verbose |= cint(Utils::getEnv("VISUS_VERBOSE_DISKACCESS"));

  //this->verbose = 1;

  PrintInfo("Created DiskAccess", "local_idx_filename", local_idx_filename, "filename_template", filename_template, "compression", compression, "bDisableWriteLocks", bDisableWriteLocks);
}


////////////////////////////////////////////////////////////////////
String DiskAccess::getFilename(Field field,double time,BigInt blockid) const
{
  auto reverse_filename = false;
  auto compression = getCompression(field.default_compression);
  return getBlockFilename(this->dataset, this->bitsperblock, filename_template, field, time, compression, blockid, reverse_filename);
}

////////////////////////////////////////////////////////////////////
void DiskAccess::acquireWriteLock(SharedPtr<BlockQuery> query)
{
  VisusAssert(isWriting());
  if (bDisableWriteLocks) 
    return;

  bool bVerbose = this->verbose ? true : false;

  String filename = Access::getFilename(query);
  
  if (bVerbose)
    PrintInfo("Aquiring write lock", filename);

  FileUtils::lock(filename);
}

////////////////////////////////////////////////////////////////////
void DiskAccess::releaseWriteLock(SharedPtr<BlockQuery> query)
{
  VisusAssert(isWriting());
  if (bDisableWriteLocks) 
    return;

  bool bVerbose = this->verbose ? true : false;

  String filename = Access::getFilename(query);

  if (bVerbose)
    PrintInfo("Release write lock", filename);

  FileUtils::unlock(filename);
}

////////////////////////////////////////////////////////////////////
void DiskAccess::readBlock(SharedPtr<BlockQuery> query)
{
  Int64  blockdim  = query->field.dtype.getByteSize(getSamplesPerBlock());
  String filename  = Access::getFilename(query);
  bool bVerbose = (this->verbose & 1) ? true : false;

  auto FAILED = [&](String reason) {
    if (bVerbose)
      PrintInfo("DiskAccess::read blockid", query->blockid, "filename", filename, "failed ", reason);
    return readFailed(query, "filename empty");
  };

  auto OK = [&]() {
    if (bVerbose)
      PrintInfo("DiskAccess::read blockid", query->blockid, "filename", filename, "OK");
    return readOk(query);
  };

  if (filename.empty())
    return FAILED("filename empty");

  if (query->aborted())
    return FAILED("query aborted");

  auto encoded=std::make_shared<HeapMemory>();
  if (!encoded->resize(FileUtils::getFileSize(filename),__FILE__,__LINE__))
    return FAILED("cannot create encoded buffer");

  File file;
  if (!file.open(filename,"r"))
    return FAILED(cstring("cannot open file", filename));

  if (!file.read(0,encoded->c_size(), encoded->c_ptr()))
    return FAILED("cannot read encoded data");

  auto nsamples = query->getNumberOfSamples();
  auto compression = getCompression(query->field.default_compression);
  auto decoded=ArrayUtils::decodeArray(compression,nsamples,query->field.dtype, encoded);
  if (!decoded.valid())
    return FAILED("cannot decode data");

  VisusAssert(decoded.dims==query->getNumberOfSamples());
  decoded.layout= query->field.default_layout;
  query->buffer=decoded;

  return OK();
}


////////////////////////////////////////////////////////////////////
void DiskAccess::writeBlock(SharedPtr<BlockQuery> query)
{
  String filename = Access::getFilename(query);
  bool bVerbose = this->verbose ? true : false;

  auto FAILED = [&](String reason) {
    if (bVerbose)
      PrintInfo("DiskAccess::writeBlock", query->blockid, "filename", filename, "failed ", reason);
    return writeFailed(query, "filename empty");
  };

  auto OK = [&]() {
    if (bVerbose)
      PrintInfo("DiskAccess::writeBlock", query->blockid, "filename", filename, "OK");
    return writeOk(query);
  };

  if (filename.empty())
    return FAILED("filename is empty");

  if (query->aborted())
    return FAILED("query aborted");

  //relaxing a little for VISUS_IDX2 (until I have the layout)
#if 0
    Int64  blockdim = query->field.dtype.getByteSize(getSamplesPerBlock());
    if (query->buffer.c_size() != blockdim)
      return FAILED("wrong buffer");

    //only RowMajor is supported!
    auto layout = query->buffer.layout;
    if (!(layout.empty() || layout == "rowmajor"))
      return FAILED("only raw major format is supported");
#endif

  FileUtils::removeFile(filename);

  File file;
  if (!file.createAndOpen(filename,"w"))
  {
    PrintInfo("Failed to write block filename", filename, "cannot create file and/or directory");
    return FAILED("cannot create file or directory");
  }

  auto decoded=query->buffer;
  auto compression = getCompression(query->field.default_compression);
  auto encoded=ArrayUtils::encodeArray(compression,decoded);
  if (!encoded)
  {
    PrintInfo("Failed to write block filename", filename, "encodeArray failed");
    return FAILED("Failed to encode data");
  }

  if (!file.write(0, encoded->c_size(), encoded->c_ptr()))
  {
    PrintInfo("Failed to write block filename", filename, "file.write failed");
    return FAILED("failed to write encoded data");
  }

  return OK();
}

} //namespace Visus







  

