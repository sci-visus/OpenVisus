#include "InputOutput.h"
#include "BitStream.h"
#include "Error.h"
#include "Expected.h"
#include "Timer.h"
#include "VarInt.h"
#include "idx2Lookup.h"
#include "idx2Read.h"
#include "idx2Decode.h"

namespace idx2
{

void
Dealloc(chunk_cache* ChunkCache)
{
  Dealloc(&ChunkCache->Bricks);
  Dealloc(&ChunkCache->BrickOffsets);
  Dealloc(&ChunkCache->ChunkStream);
}


void
Dealloc(chunk_exp_cache* ChunkExpCache)
{
  Dealloc(&ChunkExpCache->ChunkExpStream);
}

void
Init(file_cache* FileCache)
{
  Init(&FileCache->ChunkCaches, 10);
  Init(&FileCache->ChunkExpCaches, 10);
}


void
Dealloc(file_cache* FileCache)
{
  Dealloc(&FileCache->ChunkOffsets);
  idx2_ForEach (CIt, FileCache->ChunkCaches)
    Dealloc(&*CIt);
  Dealloc(&FileCache->ChunkCaches);
  idx2_ForEach (CeIt, FileCache->ChunkExpCaches)
    Dealloc(&*CeIt);
  Dealloc(&FileCache->ChunkExpCaches);
  Dealloc(&FileCache->ChunkExpOffsets);
}


void
DeallocFileCacheTable(file_cache_table* FileCacheTable)
{
  idx2_ForEach (FileCacheIt, *FileCacheTable)
    Dealloc(FileCacheIt.Val);
  Dealloc(FileCacheTable);
}


/* Given a brick address, open the file associated with the brick and cache its chunk information */
/* Structure of a file
* -------- beginning of file --------
* M
* L
* K
* J
* I
* H
* -------- exponent information ---------
* see function ReadFileExponents
* -------- end of file --------
*
* To parse the bit plane information, we parse the file backward:
* H : int32  = number of bit plane chunks
* I : int32  = size of J
* J : buffer = (zstd compressed) bit plane chunk addresses
* K : int32  = size (in bytes) of L
* L : buffer = (varint compressed) sizes of the bit plane chunks
* M : H buffers, whose sizes are encoded in L, each being one bit plane chunk
*/
static error<idx2_err_code>
ReadFile(const idx2_file& Idx2,
         decode_data* D,
         file_cache_table::iterator* FileCacheIt,
         const file_id& FileId)
{
#if VISUS_IDX2
  if (Idx2.external_read)
    return idx2_Error(idx2_err_code::NoError);
#endif

  timer IOTimer;
  StartTimer(&IOTimer);

  if (*FileCacheIt && FileCacheIt->Val->DataCached)
    return idx2_Error(idx2_err_code::NoError);

  idx2_RAII(FILE*, Fp = fopen(FileId.Name.ConstPtr, "rb"), , if (Fp) fclose(Fp));
  idx2_ReturnErrorIf(!Fp, idx2::idx2_err_code::FileNotFound, "File: %s", FileId.Name.ConstPtr);
  idx2_FSeek(Fp, 0, SEEK_END);
  i64 FileSize = idx2_FTell(Fp);
  int S = 0; // total number of bytes used to store exponents info
  ReadBackwardPOD(Fp, &S);
  idx2_FSeek(Fp, (FileSize - S), SEEK_SET); // skip the exponents info at the end
  int NChunks = 0;
  ReadBackwardPOD(Fp, &NChunks);
  // TODO: check if there are too many NChunks

  /* read and decompress chunk addresses */
  int ChunkAddrsSz;
  ReadBackwardPOD(Fp, &ChunkAddrsSz);
  idx2_ScopeBuffer(CpresChunkAddrs, ChunkAddrsSz);
  ReadBackwardBuffer(Fp, &CpresChunkAddrs, ChunkAddrsSz);
  D->BytesData_ += ChunkAddrsSz;
  D->DecodeIOTime_ += ElapsedTime(&IOTimer);
  idx2_ScopeBuffer(ChunkAddrsBuf, NChunks * sizeof(u64));
  DecompressBufZstd(CpresChunkAddrs, &ChunkAddrsBuf);

  /* read chunk sizes */
  ResetTimer(&IOTimer);
  int ChunkSizesSz = 0;
  ReadBackwardPOD(Fp, &ChunkSizesSz);
  idx2_ScopeBuffer(ChunkSizesBuf, ChunkSizesSz);
  bitstream ChunkSizeStream;
  ReadBackwardBuffer(Fp, &ChunkSizesBuf, ChunkSizesSz);
  D->BytesData_ += ChunkSizesSz;
  D->DecodeIOTime_ += ElapsedTime(&IOTimer);
  InitRead(&ChunkSizeStream, ChunkSizesBuf);

  /* parse the chunk addresses and cache in memory */
  file_cache FileCache;
  i64 AccumSize = 0;
  Init(&FileCache.ChunkCaches, 10);
  idx2_For (int, I, 0, NChunks)
  {
    i64 ChunkSize = ReadVarByte(&ChunkSizeStream); // TODO: use i32 for chunk size
    u64 ChunkAddr = *((u64*)ChunkAddrsBuf.Data + I);
    chunk_cache ChunkCache;
    ChunkCache.ChunkPos = I;
    Insert(&FileCache.ChunkCaches, ChunkAddr, ChunkCache);
    //printf("chunk %llu size = %lld\n", ChunkAddr, ChunkSize);
    PushBack(&FileCache.ChunkOffsets, AccumSize += ChunkSize);
  }
  idx2_Assert(Size(ChunkSizeStream) == ChunkSizesSz);

  if (!*FileCacheIt) // the file cache does not exist
  { // insert a new file cache
    Insert(FileCacheIt, FileId.Id, FileCache);
  }
  else // the file cache exists
  { // modify the chunk caches portion of the file cache
    idx2_Assert(Size(FileCacheIt->Val->ChunkCaches) == 0);
    FileCacheIt->Val->ChunkCaches = FileCache.ChunkCaches;
    FileCacheIt->Val->ChunkOffsets = FileCache.ChunkOffsets;
  }
  FileCacheIt->Val->DataCached = true;

  return idx2_Error(idx2_err_code::NoError);
}


/* Given a brick address, read the chunk associated with the brick and cache the chunk */
expected<const chunk_cache*, idx2_err_code>
ReadChunk(const idx2_file& Idx2, decode_data* D, u64 Brick, i8 Level, i8 Subband, i16 BpKey)
{
#if VISUS_IDX2
  if (Idx2.external_read)
  {
    u64 ChunkAddress = GetChunkAddress(Idx2, Brick, Level, Subband, BpKey);
    auto ChunkCacheIt = Lookup(D->FileCache.ChunkCaches, ChunkAddress);
    if (ChunkCacheIt)
      return ChunkCacheIt.Val;

    bitstream ChunkStream;
    bool Result = Idx2.external_read(Idx2, ChunkStream.Stream, ChunkAddress).get();
    idx2_ReturnErrorIf(!Result, idx2_err_code::ChunkNotFound);

    //decompress part
    chunk_cache ChunkCache;
    DecompressChunk(&ChunkStream, &ChunkCache, ChunkAddress, Log2Ceil(Idx2.BricksPerChunk[Level]));
    Insert(&ChunkCacheIt, ChunkAddress, ChunkCache);
    return ChunkCacheIt.Val;
  }
#endif

  file_id FileId = ConstructFilePath(Idx2, Brick, Level, Subband, BpKey);
  auto FileCacheIt = Lookup(D->FileCacheTable, FileId.Id);
  idx2_PropagateIfError(ReadFile(Idx2, D, &FileCacheIt, FileId));
  if (!FileCacheIt)
    return idx2_Error(idx2_err_code::FileNotFound, "File: %s\n", FileId.Name.ConstPtr);

  /* find the appropriate chunk */
  u64 ChunkAddress = GetChunkAddress(Idx2, Brick, Level, Subband, BpKey);
  //printf("chunk %llu\n", ChunkAddress);
  const file_cache* FileCache = FileCacheIt.Val;
  decltype(FileCache->ChunkCaches)::iterator ChunkCacheIt;
  ChunkCacheIt = Lookup(FileCache->ChunkCaches, ChunkAddress);
  if (!ChunkCacheIt)
    return idx2_Error(idx2_err_code::ChunkNotFound);
  chunk_cache* ChunkCache = ChunkCacheIt.Val;
  if (Size(ChunkCache->ChunkStream.Stream) == 0) // chunk has not been loaded
  {
    timer IOTimer;
    StartTimer(&IOTimer);
    idx2_RAII(FILE*, Fp = fopen(FileId.Name.ConstPtr, "rb"), , if (Fp) fclose(Fp));
    if (!Fp)
      return idx2_Error(idx2_err_code::FileNotFound, "File: %s\n", FileId.Name.ConstPtr);
    i32 ChunkPos = ChunkCache->ChunkPos;
    i64 ChunkOffset = ChunkPos > 0 ? FileCache->ChunkOffsets[ChunkPos - 1] : 0;
    i64 ChunkSize = FileCache->ChunkOffsets[ChunkPos] - ChunkOffset;
    idx2_FSeek(Fp, ChunkOffset, SEEK_SET);
    bitstream ChunkStream;
    // NOTE: not a memory leak since we will keep track of this in ChunkCache
    InitWrite(&ChunkStream, ChunkSize);
    ReadBuffer(Fp, &ChunkStream.Stream);
    D->BytesData_ += Size(ChunkStream.Stream);
    D->DecodeIOTime_ += ElapsedTime(&IOTimer);
    // TODO: check for error
    DecompressChunk(&ChunkStream, ChunkCache, ChunkAddress, Log2Ceil(Idx2.BricksPerChunk[Level]));
  }

  return ChunkCacheIt.Val;
}


/* Read and decode the sizes of the compressed exponent chunks in a file */
/* Structure of a file
* -------- beginning of file --------
* bit plane information (see function ReadFile)
* -------- exponent information --------
* G
* F
* E
* D
* C
* B
* A
* -------- end of file --------
*
* To parse the exponent information, we parse the file backward:
*
* A : int32     = number of bytes for the exponent information
*               = A + B + C + D + E + F + G
* B : int32     = number of exponent chunks
* C : int32     = size (in bytes) of D
* D : buffer    = (zstd compressed) exponent chunk addresses
* E : int32     = size (in bytes) of F
* F : buffer    = (varint compressed) sizes of the exponent chunks
* G : B buffers, whose sizes are encoded in F, each being one exponent chunk
*/
static error<idx2_err_code>
ReadFileExponents(const idx2_file& Idx2,
                  decode_data* D,
                  i8 Level,
                  file_cache_table::iterator* FileCacheIt,
                  const file_id& FileId)
{
#if VISUS_IDX2
  if (Idx2.external_read)
    return idx2_Error(idx2_err_code::NoError);
#endif

  timer IOTimer;
  StartTimer(&IOTimer);

  if (*FileCacheIt && FileCacheIt->Val->ExpCached)
    return idx2_Error(idx2_err_code::NoError);

  idx2_RAII(FILE*, Fp = fopen(FileId.Name.ConstPtr, "rb"), , if (Fp) fclose(Fp));
  idx2_ReturnErrorIf(!Fp, idx2::idx2_err_code::FileNotFound, "File: %s", FileId.Name.ConstPtr);
  idx2_FSeek(Fp, 0, SEEK_END);
  i64 FileSize = idx2_FTell(Fp);
  int ExponentSize = 0; // total bytes of the encoded chunk sizes
  ReadBackwardPOD(Fp, &ExponentSize); // total size of the exponent info


  /* read addresses of the exponent chunks */
  int NChunks = 0;
  ReadBackwardPOD(Fp, &NChunks);
  int ChunkAddrsSz;
  ReadBackwardPOD(Fp, &ChunkAddrsSz);
  idx2_ScopeBuffer(CpresChunkAddrs, ChunkAddrsSz);
  ReadBackwardBuffer(Fp, &CpresChunkAddrs, ChunkAddrsSz);
  D->BytesData_ += ChunkAddrsSz;
  idx2_ScopeBuffer(ChunkAddrsBuf, NChunks * sizeof(u64));
  DecompressBufZstd(CpresChunkAddrs, &ChunkAddrsBuf);

  // TODO: the exponent sizes can be compressed further
  int S = 0; // size (in bytes) of the compressed exponent sizes
  ReadBackwardPOD(Fp, &S);
  idx2_ScopeBuffer(ChunkExpSizesBuf, S);
  ReadBackwardBuffer(Fp, &ChunkExpSizesBuf, S);
  D->BytesExps_ += sizeof(int) + S;
  D->DecodeIOTime_ += ElapsedTime(&IOTimer);
  bitstream ChunkExpSizesStream;
  InitRead(&ChunkExpSizesStream, ChunkExpSizesBuf);

  file_cache FileCache;
  Init(&FileCache.ChunkExpCaches, 10);
  FileCache.ExponentBeginOffset = FileSize - ExponentSize;
  Reserve(&FileCache.ChunkExpOffsets, S);
  i32 CeSz = 0;
  // we compute a "prefix sum" of the sizes to get the offsets
  int NChunks2 = 0;
  while (Size(ChunkExpSizesStream) < S)
  {
    PushBack(&FileCache.ChunkExpOffsets, CeSz += (i32)ReadVarByte(&ChunkExpSizesStream));
    u64 ChunkAddr = *((u64*)ChunkAddrsBuf.Data + NChunks2);
    chunk_exp_cache ChunkExpCache;
    ChunkExpCache.ChunkPos = NChunks2;
    // NOTE: here we rely on the fact that the exponent chunks are sorted by increasing subband in each file
    Insert(&FileCache.ChunkExpCaches, ChunkAddr, ChunkExpCache);
    ++NChunks2;
  }

  if (NChunks != NChunks2)
    return idx2_Error(idx2_err_code::SizeMismatched,
                      "number of chunks is either %d or %d\n", NChunks, NChunks2);

  // NOTE: this should no longer be true if a file stores more than one level
  if (NChunks % Size(Idx2.Subbands) != 0)
    return idx2_Error(idx2_err_code::SizeMismatched,
                      "number of chunks = %d is not divisible by number of subbands which is %d\n", NChunks, (int)Size(Idx2.Subbands));

  //Resize(&FileCache.ChunkCaches, Size(FileCache.ChunkExpOffsets));
  idx2_Assert(Size(ChunkExpSizesStream) == S);

  if (!*FileCacheIt) // file cache exists
  {
    Insert(FileCacheIt, FileId.Id, FileCache);
  }
  else // file cache does not exist
  {
    idx2_Assert(Size(FileCacheIt->Val->ChunkExpCaches) == 0);
    FileCacheIt->Val->ExponentBeginOffset = FileCache.ExponentBeginOffset;
    FileCacheIt->Val->ChunkExpOffsets = FileCache.ChunkExpOffsets;
    FileCacheIt->Val->ChunkExpCaches = FileCache.ChunkExpCaches;
  }
  FileCacheIt->Val->ExpCached = true;

  return idx2_Error(idx2_err_code::NoError);
}


/* Given a brick address, read the exponent chunk associated with the brick and cache it */
// TODO: remove the last two params (already stored in D)
expected<const chunk_exp_cache*, idx2_err_code>
ReadChunkExponents(const idx2_file& Idx2, decode_data* D, u64 Brick, i8 Level, i8 Subband)
{
#if VISUS_IDX2
  if (Idx2.external_read)
  {
    u64 ChunkAddress = GetChunkAddress(Idx2, Brick, Level, Subband, ExponentBitPlane_);
    auto ChunkExpCacheIt = Lookup(D->FileCache.ChunkExpCaches, ChunkAddress);
    if (ChunkExpCacheIt)
      return ChunkExpCacheIt.Val;

    //read the block
    // TOOD: who manages the memory for buff? (when do I deallocate it?)
    buffer buff;
    bool Result = Idx2.external_read(Idx2, buff, ChunkAddress).get();
    if (!Result)
      idx2_ReturnErrorIf(!Result, idx2_err_code::ChunkNotFound);

    //decompress the block
    chunk_exp_cache ChunkExpCache;
    bitstream& ChunkExpStream = ChunkExpCache.ChunkExpStream;
    DecompressBufZstd(buff, &ChunkExpStream);
    DeallocBuf(&buff);
    InitRead(&ChunkExpCache.ChunkExpStream, ChunkExpStream.Stream);
    Insert(&ChunkExpCacheIt, ChunkAddress, ChunkExpCache);
    return ChunkExpCacheIt.Val;
  }
#endif

  file_id FileId = ConstructFilePath(Idx2, Brick, Level, Subband, ExponentBitPlane_);
  auto FileCacheIt = Lookup(D->FileCacheTable, FileId.Id);
  idx2_PropagateIfError(ReadFileExponents(Idx2, D, Level, &FileCacheIt, FileId));
  if (!FileCacheIt)
    return idx2_Error(idx2_err_code::FileNotFound, "File: %s\n", FileId.Name.ConstPtr);

  /* find the appropriate chunk */
  u64 ChunkAddress = GetChunkAddress(Idx2, Brick, Level, Subband, ExponentBitPlane_);
  file_cache* FileCache = FileCacheIt.Val;
  decltype(FileCache->ChunkExpCaches)::iterator ChunkCacheIt;
  ChunkCacheIt = Lookup(FileCache->ChunkExpCaches, ChunkAddress);
  if (!ChunkCacheIt)
    return idx2_Error(idx2_err_code::ChunkNotFound);

  chunk_exp_cache* ChunkExpCache = ChunkCacheIt.Val;
  if (Size(ChunkExpCache->ChunkExpStream.Stream) == 0) // chunk has not been loaded
  {
    timer IOTimer;
    StartTimer(&IOTimer);
    idx2_RAII(FILE*, Fp = fopen(FileId.Name.ConstPtr, "rb"), , if (Fp) fclose(Fp));
    if (!Fp)
      return idx2_Error(idx2_err_code::FileNotFound, "File: %s\n", FileId.Name.ConstPtr);
    i32 ChunkPos = ChunkExpCache->ChunkPos;
    i64 ChunkExpOffset = FileCache->ExponentBeginOffset;
    i32 ChunkExpSize = FileCache->ChunkExpOffsets[ChunkPos];
    if (ChunkPos > 0)
    {
      i32 PrevChunkOffset = FileCache->ChunkExpOffsets[ChunkPos - 1];
      ChunkExpOffset += PrevChunkOffset;
      ChunkExpSize -= PrevChunkOffset;
    }
    idx2_FSeek(Fp, ChunkExpOffset, SEEK_SET);
    bitstream& ChunkExpStream = ChunkExpCache->ChunkExpStream;
    idx2_ScopeBuffer(CompressedChunkExpsBuf, ChunkExpSize);
    ReadBuffer(Fp, &CompressedChunkExpsBuf, ChunkExpSize);
    DecompressBufZstd(CompressedChunkExpsBuf, &ChunkExpStream);
    D->BytesDecoded_ += ChunkExpSize;
    D->BytesExps_ += ChunkExpSize;
    D->DecodeIOTime_ += ElapsedTime(&IOTimer);
    InitRead(&ChunkExpStream, ChunkExpStream.Stream);
  }

  return ChunkCacheIt.Val;
}


} // namespace idx2

