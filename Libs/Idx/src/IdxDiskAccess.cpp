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


//////////////////////////////////////////////////////////////////////////////////
/*
version 1
  file header:=empty

  block header:=
  Uint32  offset
  Uint32  len
  Uint32  diskcompression (HZFILE_BUFFER_GZIP 1)

version 2,3,4,5
  file header:
  Uint32  file header size (==sizeof(int)*4)
  Uint32  file version
  Uint32  block header size(==sizeof(int)*4)
  Uint32  always 0

  block header:
  Uint32   offset
  Uint32   len
  Uint32   diskcompression (HZFILE_BUFFER_GZIP 1)


version 6:=

  file header (block_header)*

  Where each block_header (sizeof(int32)*10) has the following structure:

    |----------------------------------------------------------------------------------------------|
    |header (not used)  |   block_offset   |  block_size  |   block_flags   |    not used          |
    |-------------------|------------------|--------------|-----------------|----------------------|
    |    2*int32        |      2*int32     |  int32       |     int32       |       4*int32        |
    |----------------------------------------------------------------------------------------------|


NOTE For Version 5

if the dataset has a bitmask then inside the file you will have

----------------
| block header |
- --------------
|
|--------------------> [block data] (bitmask len::int32) [bitmask data]

where the <int32> is the compressed/uncompressed size of the bitmask

*/

//////////////////////////////////////////////////////////////////////
enum CompressionType
{
  NoCompression = 0,
  ZipCompression = 0x03,
  JpgCompression = 0x04,
  //ExrCompression =0x05,
  PngCompression = 0x06,
  Lz4Compression = 0x07,
  CompressionMask = 0x0f
};

//total 10*int32 (40 bytes per block header)
const int V6BlockHeaderSize = (10 * sizeof(Int32));

//file header size,enough to store 10*sizeof(int32)
const int V6FileHeaderSize = 40;

//row major
const int V6FormatRowMajor = 0x10;

//////////////////////////////////////////////////////////////////////
class BlockHeader
{
public:
  Int64 offset = 0;
  Int32 size   = 0;
  Int32 flags  = 0;
};


////////////////////////////////////////////////////////////////////
bool IdxDiskAccess::FileIO::open(String filename, String mode)
{
  VisusReleaseAssert(!mode.empty());

  //useless code, already opened in the desired mode
  if (filename == this->getFilename() && mode == this->getMode())
    return true;

  if (isOpen())
    close("need to openFile");

  if (owner->bVerbose)
    VisusInfo() << "Opening file(" << filename << ") mode(" << mode << ")";

  bool bWriting = StringUtils::contains(mode, "w");

  //already exist
  if (bool bOpened = bWriting ? File::openReadWriteBinary(filename.c_str()) : File::openReadBinary(filename.c_str()))
  {
    if (!File::read(headers.c_ptr(), headers.c_size()))
    {
      close("cannot read headers");
      return false;
    }

    auto ptr = (Int32*)(headers.c_ptr());
    for (int I = 0, Tot = (int)headers.c_size() / (int)sizeof(Int32); I < Tot; I++)
      ptr[I] = ntohl(ptr[I]);

    return true;
  }

  //cannot read the file
  if (!bWriting)
  {
    close("Cannot open file(" + filename + ")");
    return false;
  }

  //create a new file and fill up the headers
  if (!File::createAndOpenReadWriteBinary(filename))
  {
    //should not fail here!
    VisusAssert(false);
    close("Cannot create file(" + filename + ")");
    FileUtils::removeFile(filename);
    return false;
  }

  //write an empty header
  headers.fill(0);
  if (!File::write(headers.c_ptr(), headers.c_size()))
  {
    //should not fail here!
    VisusAssert(false);
    close("Cannot write zero headers file(" + filename + ")");
    FileUtils::removeFile(filename);
    return false;
  }

  return true;
}



////////////////////////////////////////////////////////////////////
void IdxDiskAccess::FileIO::close(String reason)
{
  if (!isOpen())
    return;

  if (owner->bVerbose)
    VisusInfo() << "Closing file(" << this->getFilename() << ") mode(" << this->getMode() << ") reason(" << reason << ")";

  //need to write the headers
  if (this->canWrite())
  {
    auto ptr = (Int32*)(this->headers.c_ptr());
    for (int I = 0, Tot = (int)headers.c_size() / (int)sizeof(Int32); I < Tot; I++)
      ptr[I] = htonl(ptr[I]);

    if (!this->seekAndWrite(0, headers.c_size(), headers.c_ptr()))
    {
      VisusAssert(false);
      if (owner->bVerbose)
        VisusInfo() << "cannot write headers";
    }
  }

  File::close();
}

////////////////////////////////////////////////////////////////////
IdxDiskAccess::IdxDiskAccess(IdxDataset* dataset,StringTree config) 
  : sync(this),async(this)
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

  int file_header_size = 0;
  if (idxfile.version < 6)
    file_header_size=((idxfile.version == 1) ? 0 : 16) + ((int)idxfile.fields.size()*idxfile.blocksperfile)*(3 * sizeof(Int32));
  else
    file_header_size = V6FileHeaderSize + ((int)idxfile.fields.size())*idxfile.blocksperfile*V6BlockHeaderSize;

  sync.headers.resize(file_header_size, __FILE__, __LINE__);

  // important!number of threads must be <=1 
  bool disable_async = config.readBool("disable_async", dataset->bServerMode);
  if (int nthreads = disable_async ? 0 : 1)
  {
    async.tpool = std::make_shared<ThreadPool>("IdxDiskAccess Thread", nthreads);
    async.headers.resize(file_header_size, __FILE__, __LINE__);
  }

  if (bVerbose)
    VisusInfo()<<"IdxDiskAccess created url("<<url.toString()<<") async("<<(async.tpool?"yes":"no")<<")";
}

////////////////////////////////////////////////////////////////////
IdxDiskAccess::~IdxDiskAccess()
{
  if (bVerbose)
    VisusInfo()<<"IdxDiskAccess destroyed";

  VisusReleaseAssert(!isReading() && !isWriting());
  async.destroy();
  sync.destroy();
}

////////////////////////////////////////////////////////////////////
String IdxDiskAccess::getFilename(Field field,double time,BigInt blockid) const 
{
  //not really a template... one file contains all blocks
  if (StringUtils::find(idxfile.filename_template, "%")<0)
    return idxfile.filename_template;

  //old file naming
  if (idxfile.version < 5)
  {
    char temp[2048] = { 0 };

    if (idxfile.time_template.empty())
    {
      int nwritten = sprintf(temp, idxfile.filename_template.c_str(), (int)cint64(getFirstBlockInFile(blockid)));
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
      int nwritten = sprintf(temp, idxfile.filename_template.c_str() + n, (int)cint64(getFirstBlockInFile(blockid)));
      VisusAssert(nwritten<(sizeof(temp) - 1));
      out << temp;
    }

    return out.str();
  }

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
  BigInt address = getFirstBlockInFile(blockid), partial_address;
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

////////////////////////////////////////////////////////////////////
Int64 IdxDiskAccess::getBlockPositionInFile(BigInt nblock) const
{
  VisusAssert(nblock >= 0);
  return cint64((nblock / std::max(1, idxfile.block_interleaving)) % idxfile.blocksperfile);
}

////////////////////////////////////////////////////////////////////
BigInt IdxDiskAccess::getFirstBlockInFile(BigInt nblock) const
{
  if (nblock < 0) return -1;
  return nblock - std::max(1, idxfile.block_interleaving)*getBlockPositionInFile(nblock);
}


////////////////////////////////////////////////////////////////////
void IdxDiskAccess::beginIO(String mode) {
  Access::beginIO(mode);
}

////////////////////////////////////////////////////////////////////
void IdxDiskAccess::endIO() {

  if (bool bAsyncRead = !isWriting() && async.tpool)
  {
    async.tpool->asyncRun([this](int) {
      async.close("posted endIO"); //go in queue...
    });
  }
  else
  {
    sync.close("endIO");
  }

  Access::endIO();
}

////////////////////////////////////////////////////////////////////
void IdxDiskAccess::readBlock(SharedPtr<BlockQuery> query)
{
  VisusAssert(isReading());

  BigInt blockid = query->getBlockNumber(bitsperblock);

  if (bVerbose)
    VisusInfo() << "got request to read block blockid(" << blockid << ")";

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

  String file_mode = isWriting()? "rw" : "r"; // for writing I need to read headers too

  if (bool bAsyncRead= !isWriting() && async.tpool)
  {
    async.tpool->asyncRun([this, query, file_mode](int) {
      readBlockInCurrentThread(async,query, file_mode);
    });
  }
  else
  {
    readBlockInCurrentThread(sync, query, file_mode);
  }
}


////////////////////////////////////////////////////////////////////
void IdxDiskAccess::readBlockInCurrentThread(FileIO& file,SharedPtr<BlockQuery> query, String file_mode)
{
  BigInt blockid = query->getBlockNumber(bitsperblock);

  auto failed = [&](String reason) {

    if (bVerbose)
      VisusInfo() << "IdxDiskAccess::read blockid(" << blockid << ") failed " << reason;

    return readFailed(query);
  };

  //check that the current block and file descriptor is correct
  if (blockid < 0)
    return failed(StringUtils::format() << "blockid is wrong(" << blockid << ")");

  BigInt     block_from = (blockid << idxfile.bitsperblock);
  BigInt     block_to = (block_from + ((BigInt)1 << idxfile.bitsperblock));
  VisusAssert(block_from <= query->start_address);
  VisusAssert(query->start_address <= query->end_address);
  VisusAssert(query->end_address <= block_to);

  //try to open the existing file
  String filename = getFilename(query->field, query->time, blockid);

  if (!file.open(filename, file_mode))
    return failed("cannot open file");

  //block header
  BlockHeader block_header;

  if (idxfile.version == 6)
  {
    Int32* ptr = (Int32*)(file.headers.c_ptr()
      + V6FileHeaderSize
      + (cint(query->field.index)*idxfile.blocksperfile + getBlockPositionInFile(blockid))*V6BlockHeaderSize);

    //this is the part on the disk 
    block_header.offset = (((Int64)(((Uint32*)ptr)[2])) << 0) | (((Int64)(((Uint32*)ptr)[3])) << 32);
    block_header.size = ptr[4];
    block_header.flags = ptr[5];
  }
  else
  {
    Int32* ptr = (Int32*)(file.headers.c_ptr()
      + ((idxfile.version == 1) ? 0 : 16)
      + (cint(query->field.index)*idxfile.blocksperfile + getBlockPositionInFile(blockid))*(3 * sizeof(Int32)));

    block_header.offset = ptr[0];
    block_header.size = ptr[1];
    block_header.flags = (idxfile.version <= 2) ? (ptr[2] ? 1 : 0) : ptr[2];
  }

  if (bVerbose)
    VisusInfo() << "Block header contains the following: block_offset(" << block_header.offset << ") block_size(" << block_header.size << ") block_flags(" << block_header.flags << ")";

  if (!block_header.offset || !block_header.size)
    return failed("the idx data seeems not stored in the file");

  String compression;
  if (idxfile.version>= 6)
  {
    switch (block_header.flags & CompressionMask)
    {
    case NoCompression:compression = ""; break;
    case Lz4Compression:compression = "lz4"; break;
    case ZipCompression:compression = "zip"; break;
    case JpgCompression:compression = "jpg"; break;
    case PngCompression:compression = "png"; break;
    default:
      VisusAssert(false);
      return failed("unknow compression");
    }
  }
  else
  {
    //old idx version supports only zip
    compression = (block_header.flags & 1) ? "zip" : "";
  }

  SharedPtr<HeapMemory> encoded = std::make_shared<HeapMemory>();
  if (!encoded->resize(block_header.size, __FILE__, __LINE__))
    return failed(StringUtils::format()<< "cannot resize block block_size(" << block_header.size << ")");

  if (bVerbose)
    VisusInfo() << "Reading buffer: file->seekAndRead block_offset(" << block_header.offset << ") encoded->c_size(" << encoded->c_size() << ")";

  if (!file.seekAndRead(block_header.offset, encoded->c_size(), encoded->c_ptr()))
    return failed("cannot seekAndRead encoded buffer");

  if (bVerbose)
    VisusInfo() << "Decoding buffer";

  auto decoded = ArrayUtils::decodeArray(compression, query->nsamples, query->field.dtype, encoded);
  if (!decoded)
    return failed("cannot decode the data");

  if (idxfile.version==6)
    decoded.layout = (block_header.flags & V6FormatRowMajor) ? "" : "hzorder";
  else
    decoded.layout = "hzorder";

  //i'm reading the entire block stored on this
  VisusAssert(block_from == query->start_address && block_to == query->end_address);
  VisusAssert(decoded.dims == query->nsamples);
  query->buffer = decoded;

  //for very old file I need to swap endian notation for FLOAT32
  if (idxfile.version <= 2 && query->field.dtype.isVectorOf(DTypes::FLOAT32))
  {
    if (bVerbose)
      VisusInfo() << "Swapping endian notation for Float32 type";

    auto ptr = query->buffer.c_ptr<Float32*>();

    for (int I = 0, N= (int)(query->buffer.c_size() / sizeof(Float32)); I<N; I++, ptr++)
    {
      u_long temp = ntohl(*((u_long*)ptr));
      *ptr = *((Float32*)(&temp));
    }
  }

  if (bVerbose)
    VisusInfo() << "Read block(" << cstring(blockid) << ") from file(" << filename << ") ok";

  if (bVerbose)
    VisusInfo() << "IdxDiskAccess::read blockid(" << query->getBlockNumber(bitsperblock) << ") ok";

  readOk(query);
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

  writeBlockInCurrentThread(sync, query, "rw"); //I need to read headers too
}

////////////////////////////////////////////////////////////////////
void IdxDiskAccess::writeBlockInCurrentThread(FileIO& file,SharedPtr<BlockQuery> query,String file_mode)
{
  BigInt blockid = query->getBlockNumber(bitsperblock);

  acquireWriteLock(query);

  auto failed = [&](String reason) {

    if (bVerbose)
      VisusInfo() << "IdxDiskAccess::write blockid(" << blockid << ")  failed " << reason;

    releaseWriteLock(query);
    return writeFailed(query);
  };

  if (idxfile.version < 6)
  {
    VisusAssert(false);
    return failed("Writing not supported");
  }

  Int64 blockdim = query->field.dtype.getByteSize(((Int64)1) << idxfile.bitsperblock);

  //safety check (must be exactly block aligned! cannot write partial data)
  BigInt     block_from = (blockid << idxfile.bitsperblock);
  BigInt     block_to = (block_from + ((BigInt)1 << idxfile.bitsperblock));

  VisusAssert(block_from == query->start_address);
  VisusAssert(block_to == query->end_address);
  VisusAssert(query->buffer.c_size() == blockdim);

  Int64 blockinfile = getBlockPositionInFile(blockid);

  //check that the current block and file descriptor is correct
  if (!query->field.valid() || blockid<0 || query->buffer.c_size() != blockdim)
  {
    VisusAssert(false);
    return failed("Failed to write block for version V6, input arguments are wrong");
  }

  //block flags
  int block_flags = 0;
  if (query->buffer.layout.empty())
    block_flags |= V6FormatRowMajor;
  else
    VisusAssert(query->buffer.layout == "hzorder");

  //encode the data
  String compression = query->field.default_compression;
  auto decoded = query->buffer;
  auto encoded = ArrayUtils::encodeArray(compression, decoded);
  if (!encoded)
  {
    VisusAssert(false);
    return failed("Failed to encode the data");
  }

  if      (compression.empty())  block_flags |= NoCompression;
  else if (compression == "lz4") block_flags |= Lz4Compression;
  else if (compression == "zip") block_flags |= ZipCompression;
  else if (compression == "jpg") block_flags |= JpgCompression;
  else if (compression == "png") block_flags |= PngCompression;
  else VisusAssert(false);

  String filename = getFilename(query->field, query->time, blockid);

  if (!file.open(filename, file_mode))
    return failed("cannot open file");

  //block header
  BlockHeader ondisk;

  Int32* ptr = (Int32*)(file.headers.c_ptr()
    + V6FileHeaderSize
    + cint(query->field.index)*idxfile.blocksperfile*V6BlockHeaderSize
    + getBlockPositionInFile(query->getBlockNumber(idxfile.bitsperblock))*V6BlockHeaderSize);

  //this is the part on the disk 
  ondisk.offset = (((Int64)(((Uint32*)ptr)[2])) << 0) | (((Int64)(((Uint32*)ptr)[3])) << 32);
  ondisk.size = ptr[4];
  ondisk.flags = ptr[5];

  Int64 block_offset = 0;
  if (bool bCanOverWrite = (ondisk.offset && ondisk.size) && ((encoded->c_size()) <= (ondisk.size)))
  {
    if (encoded->c_size())
      block_offset = ondisk.offset;
  }
  else
  {
    block_offset = file.seek(0, SEEK_END);
    if (block_offset <= 0)
    {
      VisusAssert(false);
      return failed("Failed to write block for version V6, file.seek(0, SEEK_END) failed");
    }
  }

  //safety check
  VisusAssert(encoded->c_size() && block_offset);

  //finally write to the disk
  if (!file.seekAndWrite(block_offset, encoded->c_size(), encoded->c_ptr()))
  {
    VisusAssert(false);
    return failed("Failed to write block for version V6,file.seekAndWrite failed");
  }

  //write the new header in memory (see close for writing of the headers on disk)
  ptr[2] = (int)(block_offset & 0xffffffff);
  ptr[3] = (int)(block_offset >> 32);
  ptr[4] = (int)encoded->c_size();
  ptr[5] = block_flags;

  if (bVerbose)
    VisusInfo() << "IdxDiskAccess::write blockid(" << query->getBlockNumber(bitsperblock) << ") ok";

  releaseWriteLock(query);

  return writeOk(query);
}


///////////////////////////////////////////////////////
void IdxDiskAccess::acquireWriteLock(SharedPtr<BlockQuery> query)
{
  if (bDisableWriteLocks)
    return;

  auto filename = getFilename(query->field, query->time, query->getBlockNumber(bitsperblock));

  if (++file_locks[filename] == 1)
  {
    FileUtils::lock(filename);

    if (bVerbose)
      VisusInfo() << "Locked file " << filename;
  }
}

///////////////////////////////////////////////////////
void IdxDiskAccess::releaseWriteLock(SharedPtr<BlockQuery> query)
{
  if (bDisableWriteLocks)
    return;

  auto filename = getFilename(query->field, query->time, query->getBlockNumber(bitsperblock));

  if (--file_locks[filename] == 0)
  {
    file_locks.erase(filename);
    FileUtils::unlock(filename);

    if (bVerbose)
      VisusInfo() << "Unlocked file " << filename;
  }
}

} //namespace Visus







  

