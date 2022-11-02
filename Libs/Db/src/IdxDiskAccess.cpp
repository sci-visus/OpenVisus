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

#include <Visus/IdxDiskAccess.h>
#include <Visus/StringMap.h>
#include <Visus/Path.h>
#include <Visus/Url.h>
#include <Visus/File.h>
#include <Visus/BlockQuery.h>
#include <Visus/Encoder.h>
#include <Visus/IdxHzOrder.h>
#include <Visus/StringTree.h>
#include <Visus/ByteOrder.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <Visus/IdxDataset.h>
#include <Visus/IdxMultipleDataset.h>

namespace Visus {


//////////////////////////////////////////////////////////////////////////////
static String GetFilenameV1234(const IdxFile& idxfile, String TimeTemplate, String FilenameTemplate, Field field, double time, BigInt blockid)
{
  //not really a template... one file contains all blocks
  if (StringUtils::find(FilenameTemplate, "%")<0)
    return FilenameTemplate;

  char temp[2048] = { 0 };

  if (TimeTemplate.empty())
  {
    int nwritten = sprintf(temp, FilenameTemplate.c_str(), (int)cint64(idxfile.getFirstBlockInFile(blockid)));
    VisusAssert(nwritten<(sizeof(temp) - 1));
    return temp;
  }

  //before string-block-template 
  int n = StringUtils::find(FilenameTemplate, "%");
  VisusAssert(n >= 0);

  std::ostringstream out;

  //add what is before the first block template
  out << FilenameTemplate.substr(0, n);

  //add the string-timestep template
  {
    int nwritten = sprintf(temp, TimeTemplate.c_str(), (int)time);
    VisusAssert(nwritten<(sizeof(temp) - 1));
    out << temp;
  }

  //apply the string template with  block id
  {
    int nwritten = sprintf(temp, FilenameTemplate.c_str() + n, (int)cint64(idxfile.getFirstBlockInFile(blockid)));
    VisusAssert(nwritten<(sizeof(temp) - 1));
    out << temp;
  }

  return out.str();
}

//////////////////////////////////////////////////////////////////////////////
static String GetFilenameV56(const IdxFile& idxfile, String TimeTemplate, String FilenameTemplate, Field field, double time, BigInt blockid)
{
  //not really a template... one file contains all blocks
  if (StringUtils::find(FilenameTemplate, "%")<0)
    return FilenameTemplate;

  /*
  this version can be a little slower, but I don't think it could be the bottleneck
  should produce exactly the same name of the old Visus code, the only difference is that
  it creates filenames going from right to left instead of from left to right (in this way I can support regular expression!)

  example:

  idxdata/%03x/%02x/%01x.bin

  then %01x represents the less significant 4 bits of the address  (____________________BBBB)
  %02x represents about the middle 8 bits                     (____________BBBBBBBB____)
  %03x represents the most significant 12 bits                (BBBBBBBBBBBB____________)
  */

  const int MaxFilenameLen = 1024;

  const char hexdigits[16] = { '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f' };
  int digit, numbits, len, k;
  BigInt address = idxfile.getFirstBlockInFile(blockid), partial_address;
  char  filename[MaxFilenameLen];
  int   N = MaxFilenameLen - 1;
  int   S = (int)FilenameTemplate.length() - 1;
  int   C = S;
  int   LastC = -1;

  //special case invalid block number
  if (address<0)
    return "";

  filename[N--] = 0;
  for (; C >= 0; C--) //going from right to left
  {
    if (FilenameTemplate[C] != '%') continue;
    LastC = C;
    digit = FilenameTemplate[C + 2] - '0';
    numbits = digit * 4;
    len = 1 + S - (C + 4);
    partial_address = address & ((((BigInt)1) << numbits) - 1);

    //IMPORTANT NOTE: do not use _snprintf or snprintf since they have different behaviour on windows and macosx (with the terminating zero!)
    memcpy(filename + 1 + N - len, FilenameTemplate.c_str() + C + 4, len); N -= len;
    for (k = 0; k<digit; k++, partial_address >>= 4) filename[N--] = hexdigits[cint64(partial_address & 0xf)];
    address >>= numbits;
    S = C - 1;
  }

  while (address != 0) //still address is non zero, must recycle the last template
  {
    C = LastC;
    VisusAssert(LastC >= 0);
    digit = FilenameTemplate[C + 2] - '0';
    numbits = digit * 4;
    partial_address = address & ((((BigInt)1) << numbits) - 1);
    filename[N--] = '/'; //ignore what is in the template, use a simple separator!
    for (k = 0; k<digit; k++, partial_address >>= 4) filename[N--] = hexdigits[cint64(partial_address & 0xf)];
    address >>= numbits;
  }

  //time template
  if (!TimeTemplate.empty())
  {
    char temp[1024] = { 0 }; //1024 seems enough only for the time!
    int nwritten = sprintf(temp, TimeTemplate.c_str(), (int)time);
    VisusAssert(nwritten<(sizeof(temp) - 1));
    TimeTemplate = temp;

    int len = (int)TimeTemplate.length();
    memcpy(filename + 1 + N - len, TimeTemplate.c_str(), len);
    N -= len;
  }

  //dump what is remained on the right
  memcpy(filename + 1 + N - (1 + S), FilenameTemplate.c_str(), 1 + S);
  return String(filename + N - S);
}



//////////////////////////////////////////////////////////////////////////////////
class IdxDiskAccessV5 : public Access
{
public:

  VISUS_NON_COPYABLE_CLASS(IdxDiskAccessV5)

  //constructor
  IdxDiskAccessV5(IdxDiskAccess* owner_, const IdxFile& idxfile_, String time_template_,String filename_template_, bool bVerbose)
    : owner(owner_), idxfile(idxfile_), time_template(time_template_), filename_template(filename_template_)
  {
    this->bVerbose = bVerbose;
    this->bitsperblock = idxfile.bitsperblock;

    int file_header_size = (idxfile.version == 1) ? 0 : (4 * sizeof(Int32));
    this->headers.resize(file_header_size + (idxfile.blocksperfile * (int)idxfile.fields.size()) * sizeof(BlockHeader), __FILE__, __LINE__);
    this->block_headers = (BlockHeader*)(this->headers.c_ptr() + file_header_size);

    if (cbool(Utils::getEnv("VISUS_VERBOSE_DISKACCESS")))
      this->bVerbose = true;

  }

  //destructor
  virtual ~IdxDiskAccessV5() {
    VisusReleaseAssert(!file.isOpen());
  }

  //getFilename
  virtual String getFilename(Field field, double time, BigInt blockid) const override
  {
    if (idxfile.version < 5)
      return GetFilenameV1234(idxfile, time_template, filename_template, field, time, blockid);
    else
      return GetFilenameV56(idxfile, time_template, filename_template, field, time, blockid);
  }

  //endIO
  virtual void endIO() override {
    closeFile("endIO");
    Access::endIO();
  }

  //readBlock
  virtual void readBlock(SharedPtr<BlockQuery> query) override
  {
    BigInt blockid = query->blockid;

    auto failed = [&](String reason) {

      if (bVerbose)
        PrintInfo("IdxDiskAccess::read blockid",blockid,"failed",reason);

      return owner->readFailed(query,reason);
    };

    //try to open the existing file
    String filename = getFilename(query->field, query->time, blockid);
    if (!openFile(filename, "r"))
      return failed(cstring("cannot open file", filename));

    const auto& block_header = block_headers[cint(query->field.index)*idxfile.blocksperfile + idxfile.getBlockPositionInFile(blockid)];
    
    Int64 block_offset = block_header.offset;
    Int32 block_size   = block_header.len;
    String compression = block_header.compressed ? "zip" : "";

    if (bVerbose)
      PrintInfo("Block header contains the following: block_offset",block_offset,"block_size",block_size,"compression",compression);

    if (!block_offset || !block_size)
      return failed(cstring("the idx data seeems not stored in the file","block_offset", block_offset,"block_size", block_size));

    auto encoded = std::make_shared<HeapMemory>();
    if (!encoded->resize(block_size, __FILE__, __LINE__))
      return failed(cstring("cannot resize block block_size",block_size));

    if (bVerbose)
      PrintInfo("Reading buffer: read block_offset",block_offset,"encoded->c_size",encoded->c_size());

    if (!file.read(block_offset, encoded->c_size(), encoded->c_ptr()))
      return failed("cannot read encoded buffer");

    if (bVerbose)
      PrintInfo("Decoding buffer");

    //only zip was supported
    auto decoded = ArrayUtils::decodeArray(compression, query->getNumberOfSamples(), query->field.dtype, encoded);
    if (!decoded.valid())
      return failed("cannot decode the data");

    decoded.layout = "hzorder";

    //i'm reading the entire block stored on this
    VisusAssert(decoded.dims == query->getNumberOfSamples());
    query->buffer = decoded;

    //for very old file I need to swap endian notation for FLOAT32
    if (idxfile.version <= 2 && query->field.dtype.isVectorOf(DTypes::FLOAT32))
    {
      if (bVerbose)
        PrintInfo("Swapping endian notation for Float32 type");

      if (!ByteOrder::isNetworkByteOrder())
      {
        Float32* ptr = query->buffer.c_ptr<Float32*>();
        for (int I = 0, N = (int)(query->buffer.c_size() / sizeof(Float32)); I < N; I++)
          ptr[I] = ByteOrder::swapByteOrder(ptr[I]);
      }
    }

    if (bVerbose)
    {
      PrintInfo("Read block", blockid, "from file", file.getFilename(), "ok");
      PrintInfo("IdxDiskAccess::read blockid", blockid, "ok");
    }

    owner->readOk(query);
  }

  //writeBlock
  virtual void writeBlock(SharedPtr<BlockQuery> query) override
  {
    VisusAssert(false);
    return owner->writeFailed(query,"not supported");
  }

private:

  /*
  file header:=

  NOTE For Version 5

  if the dataset has a bitmask then inside the file you will have

  ----------------
  | block header |
  - --------------
  |
  |--------------------> [block data] (bitmask len::int32) [bitmask data]

  where the <int32> is the compressed/uncompressed size of the bitmask

  */

  class BlockHeader
  {
  public:
    Uint32  offset     = 0;
    Uint32  len        = 0;
    Uint32  compressed = 0;
  };

  IdxDiskAccess* owner;
  IdxFile        idxfile;
  String         filename_template;
  String         time_template;
  HeapMemory     headers;
  BlockHeader*   block_headers=nullptr;
  File           file;

  //openFile
  bool openFile(String filename, String file_mode)
  {
    VisusReleaseAssert(!file_mode.empty());
    VisusReleaseAssert(file_mode == "r");

    //useless code, already opened in the desired file_mode
    if (filename == this->file.getFilename() && "r" == this->file.getFileMode())
      return true;

    if (this->file.isOpen())
      closeFile("need to openFile");

    if (bVerbose)
      PrintInfo("Opening file", filename, "file_mode", "r");

    if (!this->file.open(filename, "r"))
    {
      closeFile(cstring("Cannot open file", filename));
      return false;
    }

    //read the headers
    if (!this->file.read(0, this->headers.c_size(), this->headers.c_ptr()))
    {
      closeFile("cannot read headers");
      return false;
    }

    if (!ByteOrder::isNetworkByteOrder())
    {
      Int32* ptr = (Int32*)(this->headers.c_ptr());
      for (int I = 0, Tot = (int)this->headers.c_size() / (int)sizeof(Int32); I < Tot; I++)
        ptr[I] = ByteOrder::swapByteOrder(ptr[I]);
    }

    return true;
  }

  //closeFile
  void closeFile(String reason)
  {
    if (!this->file.isOpen())
      return;

    if (bVerbose)
      PrintInfo("Closing file",this->file.getFilename(),"file_mode", "r" ,"reason",reason);

    this->file.close();
  }

};

//////////////////////////////////////////////////////////////////////
class IdxDiskAccessV6 : public Access
{
public:

  VISUS_NON_COPYABLE_CLASS(IdxDiskAccessV6)

  //constructor
    IdxDiskAccessV6(IdxDiskAccess* owner_, const IdxFile& idxfile_, String time_template_, String filename_template_, bool bVerbose)
    : owner(owner_), idxfile(idxfile_), time_template(time_template_), filename_template(filename_template_)
  {
    this->bVerbose = bVerbose;
    this->bitsperblock = idxfile.bitsperblock;
    this->headers.resize(sizeof(FileHeader) + (idxfile.blocksperfile * (int)idxfile.fields.size()) * sizeof(BlockHeader), __FILE__, __LINE__);
    this->file_header   = (FileHeader* )(this->headers.c_ptr());
    this->block_headers = (BlockHeader*)(this->headers.c_ptr() + sizeof(FileHeader));

    this->file = std::make_shared<File>();

    if (cbool(Utils::getEnv("VISUS_VERBOSE_DISKACCESS")))
      this->bVerbose = true;
  }

  //destructor
  virtual ~IdxDiskAccessV6() {
    VisusReleaseAssert(!file->isOpen());
    file.reset();
  }

  //getFilename
  virtual String getFilename(Field field, double time, BigInt blockid) const override
  {
    return GetFilenameV56(idxfile, time_template, filename_template, field, time, blockid);
  }

  //endIO
  virtual void endIO() override {
    closeFile("endIO");
    Access::endIO();
  }

  //readBlock
  virtual void readBlock(SharedPtr<BlockQuery> query) override
  {
    BigInt blockid = query->blockid;

    auto failed = [&](String reason) {

      if (bVerbose)
        PrintInfo("IdxDiskAccess::read blockid",blockid,"failed ",reason);

      return owner->readFailed(query,reason);
    };

    auto& aborted = query->aborted;

    if (aborted())
      return failed("aborted");

    //try to open the existing file
    String filename = getFilename(query->field, query->time, blockid);
    if (!openFile(filename, isWriting() ? "rw" : "r"))
      return failed(cstring("cannot open file", filename));

    if (aborted())
      return failed("aborted");

    const BlockHeader& block_header = getBlockHeader(query->field, blockid);
    Int64 block_offset = block_header.getOffset();
    Int32 block_size   = block_header.getSize();
    String compression = block_header.getCompression();
    String layout      = block_header.getLayout();

    if (bVerbose)
      PrintInfo("Block header contains the following: block_offset",block_offset,"block_size",block_size,"compression",compression,"layout",layout);

    if (!block_offset || !block_size)
      return failed(cstring("the idx data seeems not stored in the file","block_offset", block_offset,"block_size", block_size));

    auto encoded = std::make_shared<HeapMemory>();
    if (!encoded->resize(block_size, __FILE__, __LINE__))
      return failed(cstring("cannot resize block block_size",block_size));

    if (bVerbose)
      PrintInfo("Reading buffer: read block_offset",block_offset,"encoded->c_size",encoded->c_size());

    if (aborted())
      return failed("aborted");

    if (!file->read(block_offset, encoded->c_size(), encoded->c_ptr()))
      return failed("cannot read encoded buffer");

    if (bVerbose)
      PrintInfo("Decoding buffer");

    if (aborted())
      return failed("aborted");

#if 1
    //problem with zfp. In the block header I just write it's zfp, but I don't know the number of bitplanes
    //so I am trying to get the full information from the field default_compression (example "zfp-64")
    //TODO: can we be sure we will get the full specs always from default_compression? not so sure
    if (compression == "zfp" && StringUtils::startsWith(query->field.default_compression, "zfp"))
      compression = query->field.default_compression;
#endif

    //TODO: noninterruptile
    auto decoded = ArrayUtils::decodeArray(compression, query->getNumberOfSamples(), query->field.dtype, encoded);
    if (!decoded.valid())
      return failed("cannot decode the data");

    decoded.layout = layout;

    VisusAssert(decoded.dims == query->getNumberOfSamples());
    query->buffer = decoded;

    if (bVerbose)
      PrintInfo("Read block",blockid,"from file",file->getFilename(),"ok");

    owner->readOk(query);
  }

  //writeBlock
  virtual void writeBlock(SharedPtr<BlockQuery> query) override
  {
    BigInt blockid = query->blockid;

    //NOTE: ignoring aborted in writing!
    auto& aborted = query->aborted; 

    auto failed = [&](String reason) {

      //if (bVerbose)
        PrintInfo("IdxDiskAccess::write blockid",blockid,"failed",reason);

      return owner->writeFailed(query,reason);
    };

    if (idxfile.version < 6)
    {
      VisusAssert(false);
      return failed("Writing not supported");
    }

    Int64 blockdim = query->field.dtype.getByteSize(((Int64)1) << idxfile.bitsperblock);
    VisusAssert(query->buffer.c_size() == blockdim);

    //check that the current block and file descriptor is correct
    if (!query->field.valid() || blockid<0 || query->buffer.c_size() != blockdim)
    {
      VisusAssert(false);
      return failed("Failed to write block, input arguments are wrong");
    }

    //encode the data
    String compression = query->field.default_compression;
    auto decoded = query->buffer;
    auto encoded = ArrayUtils::encodeArray(compression, decoded);
    if (!encoded)
    {
      VisusAssert(false);
      return failed("Failed to encode the data");
    }

    BlockHeader block_header;
    block_header.setLayout(query->buffer.layout);
    block_header.setSize((Int32)encoded->c_size());
    block_header.setCompression(compression);

    String filename = getFilename(query->field, query->time, blockid);
    if (!openFile(filename, "rw"))
      return failed(cstring("cannot open file", filename));

    BlockHeader existing = getBlockHeader(query->field,blockid);

    if (bool bCanOverWrite = (existing.getOffset() && existing.getSize()) && (block_header.getSize() <= existing.getSize()))
    {
      if (block_header.getSize())
        block_header.setOffset(existing.getOffset());
    }
    else
    {
      Int64 filesize = file->size();

      if (filesize <=0)
      {
        VisusAssert(false);
        return failed("Failed to write block, gotoEnd() failed");
      }

      block_header.setOffset(filesize);
    }

    VisusAssert(block_header.getSize() && block_header.getOffset());

    if (!file->write(block_header.getOffset(), block_header.getSize(), encoded->c_ptr()))
    {
      VisusAssert(false);
      return failed("Failed to write block write failed");
    }

    getBlockHeader(query->field, blockid) = block_header;

    if (bVerbose)
      PrintInfo("IdxDiskAccess::write blockid",blockid,"ok");

    return owner->writeOk(query);
  }

  //acquireWriteLock
  virtual void acquireWriteLock(SharedPtr<BlockQuery> query) override
  {
    VisusAssert(isWriting());
    if (bDisableWriteLocks) return;

    auto filename = getFilename(query->field, query->time, query->blockid);

    if (++file_locks[filename] == 1)
    {
      FileUtils::lock(filename);

      if (bVerbose)
        PrintInfo("Locked file",filename);
    }
  }

  //releaseWriteLock
  virtual void releaseWriteLock(SharedPtr<BlockQuery> query) override
  {
    VisusAssert(isWriting());
    if (bDisableWriteLocks) return;

    auto filename = getFilename(query->field, query->time, query->blockid);

    if (--file_locks[filename] == 0)
    {
      file_locks.erase(filename);
      FileUtils::unlock(filename);

      if (bVerbose)
        PrintInfo("Unlocked file",filename);
    }
  }

private:

  enum
  {
    NoCompression = 0,
    ZipCompression = 0x03,
    JpgCompression = 0x04,
    //ExrCompression =0x05,
    PngCompression = 0x06,
    Lz4Compression = 0x07,
    ZfpCompression = 0x08,    
    CompressionMask = 0x0f
  };

  enum
  {
    FormatRowMajor = 0x10
  };

  //___________________________________________
  class FileHeader
  {
  public:

    Uint32 preamble_0 = 0; //not used
    Uint32 preamble_1 = 0;
    Uint32 preamble_2 = 0;
    Uint32 preamble_3 = 0;
    Uint32 preamble_4 = 0;
    Uint32 preamble_5 = 0;
    Uint32 preamble_6 = 0;
    Uint32 preamble_7 = 0;
    Uint32 preamble_8 = 0;
    Uint32 preamble_9 = 0;
  };

  //___________________________________________
  class BlockHeader
  {
    Uint32  prefix_0    = 0; //not used
    Uint32  prefix_1    = 0; 
    Uint32  offset_low  = 0;
    Uint32  offset_high = 0;
    Uint32  size        = 0;
    Uint32  flags       = 0;
    Uint32  suffix_0    = 0; //not used
    Uint32  suffix_1    = 0; //not used
    Uint32  suffix_2    = 0; //not used
    Uint32  suffix_3    = 0; //not used

  public:

    //getOffset
    Int64 getOffset() const {
      Uint64 ret = (Uint64(offset_high) << 32) | (Uint64(offset_low) << 0);
      VisusAssert((Int64)ret==ret);
      return (Int64)ret;
    }

    //setOffset
    void setOffset(Int64 value) {
      VisusAssert(value >= 0);
      VisusAssert(Uint64(value)==value);
      offset_low  = (Uint32)(Uint64(value) & 0xffffffff);
      offset_high = (Uint32)(Uint64(value) >> 32);
      VisusAssert(value == getOffset());
    }

    //getSize
    Int32 getSize() const {
      VisusAssert((Int32)size == size);
      return (Int32)size;
    }
    
    //setSize
    void setSize(Int32 value) {
      VisusAssert(value >= 0);
      this->size = (Uint32)value; 
    }

    //getLayout
    String getLayout() const {
      return (flags & FormatRowMajor) ? "" : "hzorder";
    }

    //setLayout
    void setLayout(String value) 
    {
      if (value == "hzorder")
        flags |= 0;
      else
        flags |= FormatRowMajor;
    }

    //getCompression
    String getCompression() const {

      switch (flags & CompressionMask)
      {
        case NoCompression: return ""; break;
        case Lz4Compression:return "lz4"; break;
        case ZipCompression:return "zip"; break;
        case JpgCompression:return "jpg"; break;
        case PngCompression:return "png"; break;
        case ZfpCompression:return "zfp"; break;
        default: VisusAssert(false); return "";
      }
    }

    //setCompression
    void setCompression(String value) 
    {
      if      (value.empty())  flags |= NoCompression;
      else if (StringUtils::startsWith(value, "lz4")) flags |= Lz4Compression;
      else if (StringUtils::startsWith(value, "zip")) flags |= ZipCompression;
      else if (StringUtils::startsWith(value, "jpg")) flags |= JpgCompression;
      else if (StringUtils::startsWith(value, "png")) flags |= PngCompression;
      else if (StringUtils::startsWith(value, "zfp")) flags |= ZfpCompression;
      else VisusAssert(false);
    }

  };

  IdxDiskAccess*  owner;
  IdxFile         idxfile;
  String          time_template;
  String          filename_template;
  HeapMemory      headers;
  FileHeader*     file_header=nullptr;
  BlockHeader*    block_headers = nullptr;
  SharedPtr<File> file;

  //re-entrant file lock
  std::map<String, int> file_locks;

  //getBlockHeader
  BlockHeader& getBlockHeader(Field& field, Int64 blockid) {
    return block_headers[cint(field.index)*idxfile.blocksperfile + idxfile.getBlockPositionInFile(blockid)];
  }

  //openFile
  bool openFile(String filename, String file_mode)
  {
    VisusReleaseAssert(!file_mode.empty());
    VisusReleaseAssert(file_mode=="rw" || file_mode=="r");

    //useless code, already opened in the desired mode
    if (filename == this->file->getFilename() && file_mode == this->file->getFileMode())
      return true;

    if (this->file->isOpen())
      closeFile("need to openFile");

    if (bVerbose)
      PrintInfo("Opening file",filename,"mode", file_mode);

    //already exist
    if (this->file->open(filename, file_mode))
    {
      //read the headers
      if (!this->file->read(0, this->headers.c_size(), this->headers.c_ptr()))
      {
        closeFile("cannot read headers");
        return false;
      }

      // network to host order
      if (!ByteOrder::isNetworkByteOrder())
      {
        Uint32* ptr = (Uint32*)(this->headers.c_ptr());
        for (int I = 0, Tot = (int)this->headers.c_size() / (int)sizeof(Uint32); I < Tot; I++)
          ptr[I] = ByteOrder::swapByteOrder(ptr[I]);
      }

      return true;
    }

    //cannot read the file
    if (!StringUtils::contains(file_mode,"w"))
    {
      closeFile("Cannot open file(" + filename + ")");
      return false;
    }

    //create a new file and fill up the headers
    if (!this->file->createAndOpen(filename, "rw"))
    {
      //should not fail here!
      VisusAssert(false);
      closeFile("Cannot create file(" + filename + ")");
      FileUtils::removeFile(filename);
      return false;
    }

    //write an empty header
    this->headers.fill(0);
    if (!this->file->write(0, this->headers.c_size(), this->headers.c_ptr()))
    {
      //should not fail here!
      VisusAssert(false);
      closeFile("Cannot write zero headers file(" + filename + ")");
      FileUtils::removeFile(filename);
      return false;
    }

    return true;
  }

  //closeFile
  void closeFile(String reason)
  {
    if (!this->file->isOpen())
      return;

    if (bVerbose)
      PrintInfo("Closing file",this->file->getFilename(),"file_mode",this->file->getFileMode(),"reason",reason);

    //need to write the headers
    if (this->file->canWrite())
    {
      if (!ByteOrder::isNetworkByteOrder())
      {
        auto ptr = (Uint32*)(this->headers.c_ptr());
        for (int I = 0, Tot = (int)this->headers.c_size() / (int)sizeof(Uint32); I < Tot; I++)
          ptr[I] = ByteOrder::swapByteOrder(ptr[I]);
      }

      if (!this->file->write(0, this->headers.c_size(), this->headers.c_ptr()))
      {
        VisusAssert(false);
        if (bVerbose)
          PrintInfo("cannot write headers");
      }
    }

    this->file->close();
  }

};



////////////////////////////////////////////////////////////////////
IdxDiskAccess::IdxDiskAccess(IdxDataset* dataset,IdxFile idxfile, StringTree config) 
{
  Url url = dataset->getUrl();
  
  //create if the file does not exist
  if (config.hasAttribute("url"))
  {
    url = config.readString("url");

    if (!url.valid())
      ThrowException(cstring("cannot use", url, "for IdxDiskAccess::create, reason wrong url"));

    PrintInfo("Trying to use", url, "as cache location...");

    //can create the file if it does not exists, this is useful if you want
    //to create a disk cache for remote datasets
    if (url.isFile() && !FileUtils::existsFile(url.getPath()))
    {
      auto filename = Path(url.getPath()).toString();
      idxfile.createNewOne(filename);
    }

    //need to load it again since it can be different 
    idxfile.load(url.toString());
  }

  VisusAssert(idxfile.version>=1 && idxfile.version<=6);

  this->name = config.readString("name", "IdxDiskAccess");
  this->idxfile = idxfile;
  this->can_read  = StringUtils::find(config.readString("chmod", DefaultChMod), "r") >= 0;
  this->can_write = StringUtils::find(config.readString("chmod", DefaultChMod), "w") >= 0;
  this->bitsperblock = idxfile.bitsperblock;
  this->bVerbose = config.readInt("verbose", 0);

  //special case, a "./" at the beginning means a reference to the url
  auto resoveAlias = [&](String value) {

    String dir = Path(url.getPath()).getParent().toString();
    if (dir.empty())
      return value;

    if (StringUtils::startsWith(value, "./"))
      value = StringUtils::replaceFirst(value, ".", dir);

    value = StringUtils::replaceAll(value, "$(CurrentFileDirectory)", dir);
    return value;
  };
  
  //NOTE: time_template will go inside filename_template so there is no reason to resolve alias
  auto createAccess = [&]()->Access*{
    if (idxfile.version < 6)
      return new IdxDiskAccessV5(this, idxfile, resoveAlias(idxfile.time_template), resoveAlias(idxfile.filename_template), bVerbose);
    else
      return new IdxDiskAccessV6(this, idxfile, resoveAlias(idxfile.time_template), resoveAlias(idxfile.filename_template), bVerbose);
  };

  this-> sync.reset(createAccess());
  this->async.reset(createAccess());

  //set this only if you know what you are doing (example visus convert with only one process)
  this->bDisableWriteLocks = 
    config.readBool("disable_write_locks") == true ||
    std::find(CommandLine::args.begin(), CommandLine::args.end(), "--disable-write-locks") != CommandLine::args.end();

  if (auto env = getenv("VISUS_DISABLE_WRITE_LOCK"))
    this->bDisableWriteLocks = cbool(String(env));

  if (auto env = getenv("VISUS_IDX_SKIP_READING"))
    this->bSkipReading = cbool(String(env));

  if (auto env = getenv("VISUS_IDX_SKIP_WRITING"))
    this->bSkipWriting = cbool(String(env));

  //if (this->bDisableWriteLocks)
  //  PrintInfo("IdxDiskAccess::IdxDiskAccess disabling write locsk. be careful");

  
#if 1
  bool disable_async=false;
  if (auto env = getenv("VISUS_IDX_DISABLE_ASYNC"))
    disable_async = cbool(String(env));
  else
    disable_async = config.readBool("disable_async", dataset->isServerMode());

  // important!number of threads must be <=1 
  if (int nthreads = disable_async ? 0 : 1)
  {
    async_tpool = std::make_shared<ThreadPool>("IdxDiskAccess Thread", nthreads);
  }
#endif

  if (bVerbose)
    PrintInfo("IdxDiskAccess created url",url,"async",async_tpool?"yes":"no");
}


////////////////////////////////////////////////////////////////////
IdxDiskAccess::IdxDiskAccess(IdxDataset* dataset, StringTree config)
    : IdxDiskAccess(dataset, dataset->idxfile, config) {
}

////////////////////////////////////////////////////////////////////
IdxDiskAccess::~IdxDiskAccess()
{
  if (bVerbose)
    PrintInfo("IdxDiskAccess destroyed");

  if (async_tpool)
  {
    async_tpool->waitAll();
    async_tpool.reset();
  }

  //scrgiorgio: I have a problem here, don't know why
  //VisusReleaseAssert(!isReading() && !isWriting());
}

////////////////////////////////////////////////////////////////////
void IdxDiskAccess::disableAsync()
{
  async_tpool.reset();
}


////////////////////////////////////////////////////////////////////
void IdxDiskAccess::disableWriteLock()
{
  this->bDisableWriteLocks=true;
}


////////////////////////////////////////////////////////////////////
String IdxDiskAccess::getFilename(Field field,double time,BigInt blockid) const 
{
  return sync->getFilename(field, time, blockid);
}

////////////////////////////////////////////////////////////////////
void IdxDiskAccess::beginIO(int mode) 
{
  if (async_tpool)
    async_tpool->waitAll();

  Access::beginIO(mode);

  if (!isWriting() && async_tpool)
  {
    ThreadPool::push(async_tpool, [this, mode]() {
      async->beginIO(mode);
    });
  }
  else
  {
    sync->beginIO(mode);
  }
}

////////////////////////////////////////////////////////////////////
void IdxDiskAccess::endIO() 
{
  if (!isWriting() && async_tpool)
  {
    ThreadPool::push(async_tpool, [this]() {
      async->endIO();
    });
    async_tpool->waitAll();
  }
  else
  {
    sync->endIO();
  }

  if (async_tpool)
    async_tpool->waitAll();

  Access::endIO();
}

////////////////////////////////////////////////////////////////////
void IdxDiskAccess::readBlock(SharedPtr<BlockQuery> query)
{
  VisusAssert(isReading() || isWriting());

  BigInt blockid = query->blockid;

  if (bVerbose)
    PrintInfo("got request to read block blockid",blockid);

  //check that the current block and file descriptor is correct
  if (blockid < 0)
  {
    if (bVerbose)
      PrintInfo("IdxDiskAccess::read blockid",blockid,"failed blockid is wrong",blockid);

    return readFailed(query,"blockid negative");
  }

  if (bSkipReading)
  {
    query->allocateBufferIfNeeded();
    query->buffer.fillWithValue(0);
    return readOk(query);
  }

  if (bool bAsync = !isWriting() && async_tpool)
  {
    ThreadPool::push(async_tpool, [this, query]() {
      return async->readBlock(query);
    });
  }
  else
  {
    return sync->readBlock(query);
  }
}


////////////////////////////////////////////////////////////////////
void IdxDiskAccess::writeBlock(SharedPtr<BlockQuery> query)
{
  VisusAssert(isWriting());

  BigInt blockid = query->blockid;

  if (bVerbose)
    PrintInfo("got request to write block blockid",blockid);

  if (bSkipWriting)
  {
    query->allocateBufferIfNeeded();
    query->buffer.fillWithValue(0);
    return writeOk(query);
  }

  acquireWriteLock(query);
  sync->writeBlock(query);
  releaseWriteLock(query);
}


///////////////////////////////////////////////////////
void IdxDiskAccess::acquireWriteLock(SharedPtr<BlockQuery> query)
{
  VisusAssert(isWriting());

  if (bDisableWriteLocks || bSkipWriting) 
    return;
  
  sync->acquireWriteLock(query);
}

///////////////////////////////////////////////////////
void IdxDiskAccess::releaseWriteLock(SharedPtr<BlockQuery> query)
{
  if (bDisableWriteLocks || bSkipWriting)
    return;

  VisusAssert(isWriting());
  sync->releaseWriteLock(query);
}

} //namespace Visus







  

