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
#include <Visus/Encoders.h>
#include <Visus/IdxHzOrder.h>
#include <Visus/VisusConfig.h>
#include <Visus/ApplicationInfo.h>

#if WIN32
#pragma warning(disable:4996)
#include <WinSock2.h>
#elif __APPLE__
/*pass*/
#else
#include <netinet/in.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <Visus/IdxDataset.h>
#include <Visus/IdxMultipleDataset.h>

namespace Visus {


//////////////////////////////////////////////////////////////////////////////
static String GetFilenameV1234(const IdxFile& idxfile, Field field, double time, BigInt blockid)
{
  //not really a template... one file contains all blocks
  if (StringUtils::find(idxfile.filename_template, "%")<0)
    return idxfile.filename_template;


  char temp[2048] = { 0 };

  if (idxfile.time_template.empty())
  {
    int nwritten = sprintf(temp, idxfile.filename_template.c_str(), (int)cint64(idxfile.getFirstBlockInFile(blockid)));
    VisusAssert(nwritten<(sizeof(temp) - 1));
    return temp;
  }

  //before string-block-template 
  int n = StringUtils::find(idxfile.filename_template, "%");
  VisusAssert(n >= 0);

  std::ostringstream out;

  //add what is before the first block template
  out << idxfile.filename_template.substr(0, n);

  //add the string-timestep template
  {
    int nwritten = sprintf(temp, idxfile.time_template.c_str(), (int)time);
    VisusAssert(nwritten<(sizeof(temp) - 1));
    out << temp;
  }

  //apply the string template with  block id
  {
    int nwritten = sprintf(temp, idxfile.filename_template.c_str() + n, (int)cint64(idxfile.getFirstBlockInFile(blockid)));
    VisusAssert(nwritten<(sizeof(temp) - 1));
    out << temp;
  }

  return out.str();
}

//////////////////////////////////////////////////////////////////////////////
static String GetFilenameV56(const IdxFile& idxfile, Field field, double time, BigInt blockid)
{
  //not really a template... one file contains all blocks
  if (StringUtils::find(idxfile.filename_template, "%")<0)
    return idxfile.filename_template;

  /*
  this version can be a little slower, but I don't think it could be the bottleneck
  should produce exactly the same name of the old Visus code, the only difference is that
  it creates filenames going from right to left instead of from left to right (in this way I can support regular expression!)

  example:

  filename_template=idxdata/%03x/%02x/%01x.bin

  then %01x represents the less significant 4 bits of the address  (____________________BBBB)
  %02x represents about the middle 8 bits                     (____________BBBBBBBB____)
  %03x represents the most significant 12 bits                (BBBBBBBBBBBB____________)

  IMPORTANT: the last one (%03x) can be used many times in case of regex expression(example V010101{01}*)
  */

  const int MaxFilenameLen = 1024;

  const char hexdigits[16] = { '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f' };
  int digit, numbits, len, k;
  BigInt address = idxfile.getFirstBlockInFile(blockid), partial_address;
  char  filename[MaxFilenameLen];
  int   N = MaxFilenameLen - 1;
  String filename_template = idxfile.filename_template;
  int   S = (int)filename_template.length() - 1;
  int   C = S;
  int   LastC = -1;

  //special case invalid block number
  if (address<0)
    return "";

  filename[N--] = 0;
  for (; C >= 0; C--) //going from right to left
  {
    if (filename_template[C] != '%') continue;
    LastC = C;
    digit = filename_template[C + 2] - '0';
    numbits = digit * 4;
    len = 1 + S - (C + 4);
    partial_address = address & ((((BigInt)1) << numbits) - 1);

    //IMPORTANT NOTE: do not use _snprintf or snprintf since they have different behaviour on windows and macosx (with the terminating zero!)
    memcpy(filename + 1 + N - len, filename_template.c_str() + C + 4, len); N -= len;
    for (k = 0; k<digit; k++, partial_address >>= 4) filename[N--] = hexdigits[cint64(partial_address & 0xf)];
    address >>= numbits;
    S = C - 1;
  }

  while (address != 0) //still address is non zero, must recycle the last template
  {
    C = LastC;
    VisusAssert(LastC >= 0);
    digit = filename_template[C + 2] - '0';
    numbits = digit * 4;
    partial_address = address & ((((BigInt)1) << numbits) - 1);
    filename[N--] = '/'; //ignore what is in the template, use a simple separator!
    for (k = 0; k<digit; k++, partial_address >>= 4) filename[N--] = hexdigits[cint64(partial_address & 0xf)];
    address >>= numbits;
  }

  //time template
  String time_template = idxfile.time_template;
  if (!time_template.empty())
  {
    char temp[1024] = { 0 }; //1024 seems enough only for the time!
    int nwritten = sprintf(temp, time_template.c_str(), (int)time);
    VisusAssert(nwritten<(sizeof(temp) - 1));
    time_template = temp;

    int len = (int)time_template.length();
    memcpy(filename + 1 + N - len, time_template.c_str(), len);
    N -= len;
  }

  //dump what is remained on the right
  memcpy(filename + 1 + N - (1 + S), filename_template.c_str(), 1 + S);
  return String(filename + N - S);
}



//////////////////////////////////////////////////////////////////////////////////
class IdxDiskAccessV5 : public Access
{
public:

  VISUS_NON_COPYABLE_CLASS(IdxDiskAccessV5)

  //constructor
  IdxDiskAccessV5(IdxDiskAccess* owner_, const IdxFile& idxfile_, bool bVerbose)
    : owner(owner_), idxfile(idxfile_)
  {
    this->bVerbose = bVerbose;
    this->bitsperblock = idxfile.bitsperblock;

    int file_header_size = (idxfile.version == 1) ? 0 : (4 * sizeof(Int32));
    this->headers.resize(file_header_size + (idxfile.blocksperfile * (int)idxfile.fields.size()) * sizeof(BlockHeader), __FILE__, __LINE__);
    this->block_headers = (BlockHeader*)(this->headers.c_ptr() + file_header_size);
  }

  //destructor
  virtual ~IdxDiskAccessV5() {
    VisusReleaseAssert(!file.isOpen());
  }

  //getFilename
  virtual String getFilename(Field field, double time, BigInt blockid) const override
  {
    if (idxfile.version < 5)
      return GetFilenameV1234(idxfile, field, time, blockid);
    else
      return GetFilenameV56(idxfile, field, time, blockid);
  }

  //beginIO
  virtual void beginIO(String mode) override {
    this->mode = mode;
  }

  //endIO
  virtual void endIO() override {
    closeFile("endIO");
    this->mode = "";
  }

  //readBlock
  virtual void readBlock(SharedPtr<BlockQuery> query) override
  {
    BigInt blockid = query->getBlockNumber(owner->bitsperblock);

    auto failed = [&](String reason) {

      if (bVerbose)
        VisusInfo() << "IdxDiskAccess::read blockid(" << blockid << ") failed " << reason;

      return owner->readFailed(query);
    };

    //try to open the existing file
    String filename = getFilename(query->field, query->time, blockid);
    if (!openFile(filename, "r"))
      return failed("cannot open file");


    const auto& block_header = block_headers[cint(query->field.index)*idxfile.blocksperfile + idxfile.getBlockPositionInFile(blockid)];
    
    Int64 block_offset = block_header.offset;
    Int32 block_size   = block_header.len;
    String compression = block_header.compressed ? "zip" : "";
    String layout="hzorder";

    if (bVerbose)
      VisusInfo() << "Block header contains the following: block_offset(" << block_offset << ") block_size(" << block_size << ") compression(" << compression << ") layout(" << layout << ")";

    if (!block_offset || !block_size)
      return failed("the idx data seeems not stored in the file");

    auto encoded = std::make_shared<HeapMemory>();
    if (!encoded->resize(block_size, __FILE__, __LINE__))
      return failed(StringUtils::format() << "cannot resize block block_size(" << block_size << ")");

    if (bVerbose)
      VisusInfo() << "Reading buffer: read block_offset(" << block_offset << ") encoded->c_size(" << encoded->c_size() << ")";

    if (!file.read(block_offset, encoded->c_size(), encoded->c_ptr()))
      return failed("cannot read encoded buffer");

    if (bVerbose)
      VisusInfo() << "Decoding buffer";

    auto decoded = ArrayUtils::decodeArray(compression, query->nsamples, query->field.dtype, encoded);
    if (!decoded)
      return failed("cannot decode the data");

    decoded.layout = layout;

    //i'm reading the entire block stored on this
    VisusAssert(decoded.dims == query->nsamples);
    query->buffer = decoded;

    //for very old file I need to swap endian notation for FLOAT32
    if (idxfile.version <= 2 && query->field.dtype.isVectorOf(DTypes::FLOAT32))
    {
      if (bVerbose)
        VisusInfo() << "Swapping endian notation for Float32 type";

      auto ptr = query->buffer.c_ptr<Float32*>();

      for (int I = 0, N = (int)(query->buffer.c_size() / sizeof(Float32)); I<N; I++, ptr++)
      {
        u_long temp = ntohl(*((u_long*)ptr));
        *ptr = *((Float32*)(&temp));
      }
    }

    if (bVerbose)
      VisusInfo() << "Read block(" << cstring(blockid) << ") from file(" << file.getFilename() << ") ok";

    if (bVerbose)
      VisusInfo() << "IdxDiskAccess::read blockid(" << blockid << ") ok";

    owner->readOk(query);
  }

  //writeBlock
  virtual void writeBlock(SharedPtr<BlockQuery> query) override
  {
    VisusAssert(false);
    return owner->writeFailed(query);
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
  HeapMemory     headers;
  BlockHeader*   block_headers=nullptr;
  File           file;
  String         mode;

  //openFile
  bool openFile(String filename, String mode)
  {
    VisusReleaseAssert(!mode.empty());

    //useless code, already opened in the desired mode
    if (filename == this->file.getFilename() && mode == this->file.getMode())
      return true;

    if (this->file.isOpen())
      closeFile("need to openFile");

    if (bVerbose)
      VisusInfo() << "Opening file(" << filename << ") mode(" << mode << ")";


    if (!this->file.open(filename,"r"))
    {
      closeFile("Cannot open file(" + filename + ")");
      return false;
    }

    //read the headers
    if (!this->file.read(0, this->headers.c_size(), this->headers.c_ptr()))
    {
      closeFile("cannot read headers");
      return false;
    }


    auto ptr = (Int32*)(this->headers.c_ptr());
    for (int I = 0, Tot = (int)this->headers.c_size() / (int)sizeof(Int32); I < Tot; I++)
      ptr[I] = ntohl(ptr[I]);

    return true;
  }

  //closeFile
  void closeFile(String reason)
  {
    if (!this->file.isOpen())
      return;

    if (bVerbose)
      VisusInfo() << "Closing file(" << this->file.getFilename() << ") mode(" << this->file.getMode() << ") reason(" << reason << ")";

    this->file.close();
  }

};

//////////////////////////////////////////////////////////////////////
class IdxDiskAccessV6 : public Access
{
public:

  VISUS_NON_COPYABLE_CLASS(IdxDiskAccessV6)

  //constructor
    IdxDiskAccessV6(IdxDiskAccess* owner_, const IdxFile& idxfile_, bool bVerbose)
    : owner(owner_), idxfile(idxfile_)
  {
    this->bVerbose = bVerbose;
    this->bitsperblock = idxfile.bitsperblock;
    this->headers.resize(sizeof(FileHeader) + (idxfile.blocksperfile * (int)idxfile.fields.size()) * sizeof(BlockHeader), __FILE__, __LINE__);
    this->file_header   = (FileHeader* )(this->headers.c_ptr());
    this->block_headers = (BlockHeader*)(this->headers.c_ptr() + sizeof(FileHeader));

    if (bool bUsePosix=true)
      this->file = std::make_shared<File>();
    else
      this->file = std::make_shared<MemoryMappedFile>();
  }

  //destructor
  virtual ~IdxDiskAccessV6() {
    VisusReleaseAssert(!file->isOpen());
    file.reset();
  }

  //getFilename
  virtual String getFilename(Field field, double time, BigInt blockid) const override
  {
    return GetFilenameV56(idxfile, field, time, blockid);
  }

  //beginIO
  virtual void beginIO(String mode) override  {
    this->mode = mode;
  }

  //endIO
  virtual void endIO() override {
    closeFile("endIO");
    this->mode = "";
  }

  //readBlock
  virtual void readBlock(SharedPtr<BlockQuery> query) override
  {
    BigInt blockid = query->getBlockNumber(owner->bitsperblock);

    auto failed = [&](String reason) {

      if (bVerbose)
        VisusInfo() << "IdxDiskAccess::read blockid(" << blockid << ") failed " << reason;

      return owner->readFailed(query);
    };

    //try to open the existing file
    String filename = getFilename(query->field, query->time, blockid);
    if (!openFile(filename, StringUtils::contains(mode, "w") ? "rw" : "r"))
      return failed("cannot open file");

    const BlockHeader& block_header = getBlockHeader(query->field, blockid);
    Int64 block_offset = block_header.getOffset();
    Int32 block_size   = block_header.getSize();
    String compression = block_header.getCompression();
    String layout      = block_header.getLayout();

    if (bVerbose)
      VisusInfo() << "Block header contains the following: block_offset(" << block_offset << ") block_size(" << block_size << ") compression(" << compression << ") layout(" << layout << ")";

    if (!block_offset || !block_size)
      return failed("the idx data seeems not stored in the file");

    auto encoded = std::make_shared<HeapMemory>();
    if (!encoded->resize(block_size, __FILE__, __LINE__))
      return failed(StringUtils::format() << "cannot resize block block_size(" << block_size << ")");

    if (bVerbose)
      VisusInfo() << "Reading buffer: read block_offset(" << block_offset << ") encoded->c_size(" << encoded->c_size() << ")";

    if (!file->read(block_offset, encoded->c_size(), encoded->c_ptr()))
      return failed("cannot read encoded buffer");

    if (bVerbose)
      VisusInfo() << "Decoding buffer";

    auto decoded = ArrayUtils::decodeArray(compression, query->nsamples, query->field.dtype, encoded);
    if (!decoded)
      return failed("cannot decode the data");

    decoded.layout = layout;

    VisusAssert(decoded.dims == query->nsamples);
    query->buffer = decoded;

    if (bVerbose)
      VisusInfo() << "Read block(" << cstring(blockid) << ") from file(" << file->getFilename() << ") ok";

    owner->readOk(query);
  }

  //writeBlock
  virtual void writeBlock(SharedPtr<BlockQuery> query) override
  {
    BigInt blockid = query->getBlockNumber(owner->bitsperblock);

    auto failed = [&](String reason) {

      if (bVerbose)
        VisusInfo() << "IdxDiskAccess::write blockid(" << blockid << ")  failed " << reason;

      return owner->writeFailed(query);
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
      return failed("cannot open file");

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

    getBlockHeader(query->field, blockid) =block_header;

    if (bVerbose)
      VisusInfo() << "IdxDiskAccess::write blockid(" << blockid << ") ok";

    return owner->writeOk(query);
  }

  //acquireWriteLock
  virtual void acquireWriteLock(SharedPtr<BlockQuery> query) override
  {
    auto filename = getFilename(query->field, query->time, query->getBlockNumber(bitsperblock));

    if (++file_locks[filename] == 1)
    {
      FileUtils::lock(filename);

      if (bVerbose)
        VisusInfo() << "Locked file " << filename;
    }
  }

  //releaseWriteLock
  virtual void releaseWriteLock(SharedPtr<BlockQuery> query) override
  {
    auto filename = getFilename(query->field, query->time, query->getBlockNumber(bitsperblock));

    if (--file_locks[filename] == 0)
    {
      file_locks.erase(filename);
      FileUtils::unlock(filename);

      if (bVerbose)
        VisusInfo() << "Unlocked file " << filename;
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
    void setLayout(String value) {

      if (value.empty() || value == "rowmajor")
        flags |= FormatRowMajor;
      else 
        VisusAssert(value=="hzorder");

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
        default: VisusAssert(false); return "";
      }
    }

    //setCompression
    void setCompression(String value) 
    {
      if      (value.empty())  flags |= NoCompression;
      else if (value == "lz4") flags |= Lz4Compression;
      else if (value == "zip") flags |= ZipCompression;
      else if (value == "jpg") flags |= JpgCompression;
      else if (value == "png") flags |= PngCompression;
      else VisusAssert(false);
    }

  };

  IdxDiskAccess*  owner;
  IdxFile         idxfile;
  HeapMemory      headers;
  FileHeader*     file_header=nullptr;
  BlockHeader*    block_headers = nullptr;
  SharedPtr<AbstractFile> file;
  String          mode;

  //re-entrant file lock
  std::map<String, int> file_locks;

  //getBlockHeader
  BlockHeader& getBlockHeader(Field& field, Int64 blockid) {
    return block_headers[cint(field.index)*idxfile.blocksperfile + idxfile.getBlockPositionInFile(blockid)];
  }

  //openFile
  bool openFile(String filename, String mode)
  {
    VisusReleaseAssert(!mode.empty());

    //useless code, already opened in the desired mode
    if (filename == this->file->getFilename() && mode == this->file->getMode())
      return true;

    if (this->file->isOpen())
      closeFile("need to openFile");

    if (bVerbose)
      VisusInfo() << "Opening file(" << filename << ") mode(" << mode << ")";

    bool bWriting = StringUtils::contains(mode, "w");

    //already exist
    if (bool bOpened = bWriting ? this->file->open(filename,"rw") : this->file->open(filename,"r"))
    {
      //read the headers
      if (!this->file->read(0, this->headers.c_size(), this->headers.c_ptr()))
      {
        closeFile("cannot read headers");
        return false;
      }

      // network to host order
      // note:  network byte order is defined to always be big-endian
      // Intel x86 are  little-endian
      auto ptr = (Uint32*)(this->headers.c_ptr());
      for (int I = 0, Tot = (int)this->headers.c_size() / (int)sizeof(Uint32); I < Tot; I++)
        ptr[I] = ntohl(ptr[I]);

      return true;
    }

    //cannot read the file
    if (!bWriting)
    {
      closeFile("Cannot open file(" + filename + ")");
      return false;
    }

    //create a new file and fill up the headers
    if (!this->file->createAndOpen(filename,"rw"))
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
      VisusInfo() << "Closing file(" << this->file->getFilename() << ") mode(" << this->file->getMode() << ") reason(" << reason << ")";

    //need to write the headers
    if (this->file->canWrite())
    {
      // host to network order
      // note:  network byte order is defined to always be big-endian
      // Intel x86 are  little-endian
      auto ptr = (Uint32*)(this->headers.c_ptr());
      for (int I = 0, Tot = (int)this->headers.c_size() / (int)sizeof(Uint32); I < Tot; I++)
        ptr[I] = htonl(ptr[I]);

      if (!this->file->write(0, this->headers.c_size(), this->headers.c_ptr()))
      {
        VisusAssert(false);
        if (bVerbose)
          VisusInfo() << "cannot write headers";
      }
    }

    this->file->close();
  }

};



////////////////////////////////////////////////////////////////////
IdxDiskAccess::IdxDiskAccess(IdxDataset* dataset,StringTree config) 
{
  if (!dataset->valid())
    ThrowException("IdxDataset not valid");

  String chmod=config.readString("chmod","rw");
  auto idxfile=dataset->idxfile;

  Url url=config.readString("url",dataset->getUrl().toString());
  if (!url.valid())
    ThrowException(StringUtils::format()<<"cannot use "<<url.toString()<<" for IdxDiskAccess::create, reason wrong url");

  if (url.toString()!=dataset->getUrl().toString())
  {
    VisusInfo()<<"Trying to use "<<url.toString()<<" as cache location...";

    //can create the file if it does not exists, this is useful if you want
    //to create a disk cache for remote datasets
    if (url.isFile() && !FileUtils::existsFile(url.getPath()))
    {
      IdxFile local_idxfile=idxfile;
      local_idxfile.version=0;
      local_idxfile.block_interleaving=0;
      local_idxfile.filename_template="";
      if (!local_idxfile.save(Path(url.getPath()).toString())) {
        String msg=StringUtils::format()<<"cannot use "<<url.toString()<<" as cache location. save failed";
        VisusWarning()<<msg;
        ThrowException(msg);
      }
    }

    //need to load it again since it can be different
    {
      IdxFile local_idxfile=IdxFile::openFromUrl(url);
      if (!local_idxfile.valid()) {
        String msg=StringUtils::format()<<"cannot use "<<url.toString()<<" as cache location. load failed";
        VisusWarning()<<msg;
        ThrowException(msg);
      }

      idxfile=local_idxfile;
    }
  }

  VisusAssert(idxfile.version>=1 && idxfile.version<=6);

  this->name = config.readString("name", "IdxDiskAccess");
  this->idxfile = idxfile;
  this->can_read = StringUtils::find(chmod, "r") >= 0;
  this->can_write = StringUtils::find(chmod, "w") >= 0;
  this->bitsperblock = idxfile.bitsperblock;
  this->bVerbose = config.readInt("verbose", 0);

  if (idxfile.version < 6)
  {
    this-> sync.reset(new IdxDiskAccessV5(this, idxfile, bVerbose));
    this->async.reset(new IdxDiskAccessV5(this, idxfile, bVerbose));
  }
  else
  {
    this-> sync.reset(new IdxDiskAccessV6(this, idxfile, bVerbose));
    this->async.reset(new IdxDiskAccessV6(this, idxfile, bVerbose));
  }

  //special case for caching stuff (example range="0 2048" means that all block >=0 && block<2048 will pass throught)
  auto block_range=config.readString("range");
  if (!block_range.empty())
  {
    std::istringstream parse(block_range);
    parse >> this->block_range.from;
    parse >> this->block_range.to;
  }

  //set this only if you know what you are doing (example visus convert with only one process)
  this->bDisableWriteLocks = 
    config.readBool("disable_write_locks") == true ||
    std::find(ApplicationInfo::args.begin(), ApplicationInfo::args.end(), "--disable-write-locks") != ApplicationInfo::args.end();

  //if (this->bDisableWriteLocks)
  //  VisusInfo() << "IdxDiskAccess::IdxDiskAccess disabling write locsk. be careful";

  this->bDisableIO = config.readBool("disable_io")==true ||
    std::find(ApplicationInfo::args.begin(), ApplicationInfo::args.end(), "--idx-disk-access-disable-io") != ApplicationInfo::args.end();

  // important!number of threads must be <=1 
#if 1
  bool disable_async = config.readBool("disable_async", dataset->bServerMode);
  if (int nthreads = disable_async ? 0 : 1)
  {
    async_tpool = std::make_shared<ThreadPool>("IdxDiskAccess Thread", nthreads);
  }
#endif

  if (bVerbose)
    VisusInfo()<<"IdxDiskAccess created url("<<url.toString()<<") async("<<(async_tpool?"yes":"no")<<")";
}

////////////////////////////////////////////////////////////////////
IdxDiskAccess::~IdxDiskAccess()
{
  if (bVerbose)
    VisusInfo()<<"IdxDiskAccess destroyed";

  VisusReleaseAssert(!isReading() && !isWriting());

  if (async_tpool)
  {
    async_tpool->waitAll();
    async_tpool.reset();
  }
}

////////////////////////////////////////////////////////////////////
String IdxDiskAccess::getFilename(Field field,double time,BigInt blockid) const 
{
  return sync->getFilename(field, time, blockid);
}

////////////////////////////////////////////////////////////////////
void IdxDiskAccess::beginIO(String mode) {
  Access::beginIO(mode);

  bool bAsync = !isWriting() && async_tpool;

  ThreadPool::push(bAsync ? async_tpool : SharedPtr<ThreadPool>(), [this, bAsync, mode]() {
    auto pimpl = bAsync ? async.get() : sync.get();
    pimpl->beginIO(mode);
  });
  
}

////////////////////////////////////////////////////////////////////
void IdxDiskAccess::endIO() {

  bool bAsync = !isWriting() && async_tpool;

  ThreadPool::push(bAsync? async_tpool : SharedPtr<ThreadPool>(),[this, bAsync]() {
    auto pimpl = bAsync ? async.get() : sync.get();
    pimpl->endIO();
  });

  Access::endIO();
}

////////////////////////////////////////////////////////////////////
void IdxDiskAccess::readBlock(SharedPtr<BlockQuery> query)
{
  VisusAssert(isReading());

  BigInt blockid = query->getBlockNumber(bitsperblock);

  if (bVerbose)
    VisusInfo() << "got request to read block blockid(" << blockid << ")";

  //check that the current block and file descriptor is correct
  if (blockid < 0)
  {
    if (bVerbose)
      VisusInfo() << "IdxDiskAccess::read blockid(" << blockid << ") failed blockid is wrong(" << blockid << ")";

    return readFailed(query);
  }

  VisusAssert(query->start_address <= query->end_address);

  if (block_range.to>0 )
  {    
    if (bVerbose)
      VisusInfo() << "IdxDiskAccess::read blockid(" << blockid << ") failed because out of range";

    if (!(blockid >= block_range.from && blockid < block_range.to))
      return readFailed(query);
    else
    {
      query->buffer.fillWithValue(query->field.default_value);
      return readOk(query);
    }
  }

  if (bDisableIO)
  {
    query->buffer.fillWithValue(query->field.default_value);
    query->buffer.layout = query->field.default_layout;
    return readOk(query);
  }

  bool bAsync = !isWriting() && async_tpool;
  ThreadPool::push(bAsync? async_tpool : SharedPtr<ThreadPool>(),[this, query, bAsync]() {

    auto pimpl = bAsync? async.get() : sync.get();
    return pimpl->readBlock(query);
  });

}


////////////////////////////////////////////////////////////////////
void IdxDiskAccess::writeBlock(SharedPtr<BlockQuery> query)
{
  VisusAssert(isWriting());

  BigInt blockid = query->getBlockNumber(bitsperblock);

  if (bVerbose)
    VisusInfo() << "got request to write block blockid(" << blockid << ")";

  if (block_range.to>0 && !(blockid >= block_range.from && blockid < block_range.to))
  {
    if (bVerbose)
      VisusInfo() << "IdxDiskAccess::write blockid(" << blockid << ") failed because out of range";

    return writeFailed(query);
  }

  if (bDisableIO)
    return writeOk(query);

  acquireWriteLock(query);
  sync->writeBlock(query);
  releaseWriteLock(query);
}


///////////////////////////////////////////////////////
void IdxDiskAccess::acquireWriteLock(SharedPtr<BlockQuery> query)
{
  if (bDisableWriteLocks)
    return;

  VisusAssert(isWriting());
  sync->acquireWriteLock(query);
}

///////////////////////////////////////////////////////
void IdxDiskAccess::releaseWriteLock(SharedPtr<BlockQuery> query)
{
  if (bDisableWriteLocks)
    return;

  VisusAssert(isWriting());
  sync->releaseWriteLock(query);
}

} //namespace Visus







  

