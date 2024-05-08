#include "idx2Common.h"
#include "idx2Encode.h"
#include "idx2Lookup.h"
#include "InputOutput.h"
#include "FileSystem.h"
#include "Statistics.h"
#include "VarInt.h"

namespace idx2
{


/* book-keeping stuffs */
static stat BlockStat;
static stat BlockEMaxStat;
static stat UncompressedExpChunksStat;
static stat CompressedExpChunksStat;
static stat ExpChunkSizesStat;
static stat CompressedExpChunkAddressesStat;
static stat UncompressedExpChunkAddressesStat;
static stat BrickDeltasStat;
static stat BrickSizesStat;
//static stat BrickStreamStat;
static stat BitPlaneChunksStat;
static stat CompressedChunkAddressesStat;
static stat UncompressedChunkAddressesStat;
static stat ChunkSizesStat;


/* Write an exponent chunk to a file (we actually write to a buffer then later flush to a file).
* The structure of a chunk:
* A = (zstd compressed) exponents for each brick
* (we don't need to encode which bricks are present since all bricks are present)
* (also, since the exponents are written originally with fixed number of bytes, we don't need
* to encode the size for each brick)
*/
void
WriteChunkExponents(const idx2_file& Idx2, encode_data* E, sub_channel* Sc, i8 Level, i8 Subband)
{
#if VISUS_IDX2
  if (Idx2.external_write){

    /* brick exponents */
    Flush(&Sc->BrickExpStream);
    UncompressedExpChunksStat.Add((f64)Size(Sc->BrickExpStream));
    Rewind(&E->ChunkExpStream);
    CompressBufZstd(ToBuffer(Sc->BrickExpStream), &E->ChunkExpStream);
    CompressedExpChunksStat.Add((f64)Size(E->ChunkExpStream));
    Rewind(&Sc->BrickExpStream);
    u64 ChunkExpAddress = GetChunkAddress(Idx2, Sc->LastBrick, Level, Subband, ExponentBitPlane_);
    buffer Buf = ToBuffer(E->ChunkExpStream);
    if (!Idx2.external_write(Idx2, Buf, ChunkExpAddress))
      idx2_ExitIf(true, "Write chunk exponent failed\n");
    Rewind(&E->ChunkExpStream);
    return;
  }
#endif

  /* brick exponents */
  Flush(&Sc->BrickExpStream);
  UncompressedExpChunksStat.Add((f64)Size(Sc->BrickExpStream));
  Rewind(&E->ChunkExpStream);
  CompressBufZstd(ToBuffer(Sc->BrickExpStream), &E->ChunkExpStream);
  CompressedExpChunksStat.Add((f64)Size(E->ChunkExpStream));

  /* rewind */
  Rewind(&Sc->BrickExpStream);

  /* write to file */
  file_id FileId = ConstructFilePath(Idx2, Sc->LastBrick, Level, Subband, ExponentBitPlane_);
  /* keep track of the chunk sizes */
  auto CemIt = Lookup(E->ChunkExponents, FileId.Id);
  if (!CemIt)
  {
    chunk_exp_info ChunkExpInfo;
    InitWrite(&ChunkExpInfo.ExpSizes, 128);
    //Init(&ChunkEMaxInfo.FileEMaxBuffer, 128);
    Insert(&CemIt, FileId.Id, ChunkExpInfo);
  }
  chunk_exp_info* Ce = CemIt.Val;
  bitstream* ChunkEMaxSzs = &Ce->ExpSizes;
  GrowToAccomodate(ChunkEMaxSzs, 4);
  // write the size of the exponent stream for current chunk
  WriteVarByte(ChunkEMaxSzs, Size(E->ChunkExpStream));
  array<u8>* ExpBuffer = &Ce->FileExpBuffer;
  // write the exponents to the exponent buffer for the whole file
  PushBack(ExpBuffer, E->ChunkExpStream.Stream.Data, Size(E->ChunkExpStream));
  //printf("%lld %lld\n", Size(E->ChunkExpStream), Size(*EMaxBuffer));
  u64 ChunkExpAddress = GetChunkAddress(Idx2, Sc->LastBrick, Level, Subband, ExponentBitPlane_);
  PushBack(&Ce->Addrs, ChunkExpAddress);
  Rewind(&E->ChunkExpStream);
}


// TODO: check the error path
/* Write the buffered exponent chunks for each file.
Write also the metadata for the exponent chunks at the end of each file. */
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
* A : int32     = number of bytes for the exponent information
*               = A + B + C + D + E + F + G
* B : int32     = number of exponent chunks
* C : int32     = size (in bytes) of D
* D : buffer    = (zstd compressed) exponent chunk addresses
* E : int32     = size (in bytes) of F
* F : buffer    = (varint compressed) sizes of the exponent chunks
* G : B buffers, whose sizes are encoded in F, each being one exponent chunk
*/
error<idx2_err_code>
FlushChunkExponents(const idx2_file& Idx2, encode_data* E)
{
#if VISUS_IDX2
  if (Idx2.external_write)
  {
    Reserve(&E->SortedSubChannels, Size(E->SubChannels));
    Clear(&E->SortedSubChannels);
    idx2_ForEach (Sch, E->SubChannels)
    {
      sub_channel_info ScInfo;
      ScInfo.SubChannel = &*Sch;
      u64 Brick;
      i16 BitPlane;
      UnpackFileAddress(Idx2, *Sch.Key, &Brick, &ScInfo.Level, &ScInfo.Subband, &BitPlane);
      PushBack(&E->SortedSubChannels, ScInfo);
    }
    InsertionSort(Begin(E->SortedSubChannels), End(E->SortedSubChannels));

    idx2_ForEach (Sch, E->SortedSubChannels)
      WriteChunkExponents(Idx2, E, Sch->SubChannel, Sch->Level, Sch->Subband); //just call  WriteChunkExponents

    return idx2_Error(idx2_err_code::NoError);
  }
#endif

  Reserve(&E->SortedSubChannels, Size(E->SubChannels));
  Clear(&E->SortedSubChannels);
  idx2_ForEach (Sch, E->SubChannels)
  {
    sub_channel_info ScInfo;
    ScInfo.SubChannel = &*Sch;
    u64 Brick;
    i16 BitPlane;
    UnpackFileAddress(Idx2, *Sch.Key, &Brick, &ScInfo.Level, &ScInfo.Subband, &BitPlane);
    PushBack(&E->SortedSubChannels, ScInfo);
  }
  InsertionSort(Begin(E->SortedSubChannels), End(E->SortedSubChannels));

  idx2_ForEach (Sch, E->SortedSubChannels)
    WriteChunkExponents(Idx2, E, Sch->SubChannel, Sch->Level, Sch->Subband);
  // TODO: deallocate the file emax buffer after it is flushed to a file
  // TODO: need to "interleave" this with FlushChunk
  // TODO: detect that we are done with a file to flush it as soon as possible instead of at the end (maybe count the number of chunks in a file)
  // TODO: as soon as we get out of a spatial domain for a file, flush every files in the buffer
  // (since we know that all files in the buffer cannot be traversed again in the spatial DFS order)
  idx2_ForEach (CeIt, E->ChunkExponents) // one CeIt for each file
  {
    chunk_exp_info* Ce = CeIt.Val;
    bitstream* ChunkExpSizes = &Ce->ExpSizes;
    file_id FileId = ConstructFilePath(Idx2, *CeIt.Key);
    idx2_Assert(FileId.Id == *CeIt.Key);
    /* write chunk emax sizes */
    idx2_OpenMaybeExistingFile(Fp, FileId.Name.ConstPtr, "ab");
    Flush(ChunkExpSizes);
    ExpChunkSizesStat.Add((f64)Size(*ChunkExpSizes));
    int TotalExpBytes = 0;
    // write the exponent buffer
    buffer Buf = ToBuffer(Ce->FileExpBuffer);
    WriteBuffer(Fp, Buf);
    TotalExpBytes += int(Buf.Bytes);
    // write the (compressed) sizes of the exponents
    Buf = ToBuffer(*ChunkExpSizes);
    WriteBuffer(Fp, Buf);
    WritePOD(Fp, (int)Buf.Bytes);
    TotalExpBytes += int(Buf.Bytes) + sizeof(int);
    // write compressed chunk addresses
    UncompressedExpChunkAddressesStat.Add((f64)Size(ToBuffer(Ce->Addrs)));
    CompressBufZstd(ToBuffer(Ce->Addrs), &E->CompressedChunkAddresses);
    CompressedExpChunkAddressesStat.Add((f64)Size(E->CompressedChunkAddresses));
    Buf = ToBuffer(E->CompressedChunkAddresses);
    WriteBuffer(Fp, Buf);
    WritePOD(Fp, (int)Buf.Bytes);
    TotalExpBytes += int(Buf.Bytes) + sizeof(int);
    // write number of chunks
    WritePOD(Fp, (int)Size(Ce->Addrs));
    TotalExpBytes += sizeof(int);
    // write the total number of bytes used for storing the exponents
    TotalExpBytes += sizeof(int);
    WritePOD(Fp, (int)TotalExpBytes);
    Dealloc(&CeIt.Val->FileExpBuffer);
  }

  return idx2_Error(idx2_err_code::NoError);
}


// TODO: return error
/* Write a chunk to disk.
* The structure of a chunk:
* A
* B
* C
* D
*
* A : varint = number of bricks in the chunk
* B : buffer = (unary encoded) delta stream that encodes which bricks are present
* C : buffer = (varint encoded) size (in bytes) of each brick
* D : buffer = (compressed) data for each brick
*/
void
WriteChunk(const idx2_file& Idx2, encode_data* E, channel* C, i8 Level, i8 Subband, i16 BitPlane)
{
#if VISUS_IDX2
  if (Idx2.external_write)
  {
    BrickDeltasStat.Add((f64)Size(C->BrickDeltasStream)); // brick deltas
    BrickSizesStat.Add((f64)Size(C->BrickSizeStream));      // brick sizes
    i64 ChunkSize =
      Size(C->BrickDeltasStream) + Size(C->BrickSizeStream) + Size(C->BrickStream) + 64;
    Rewind(&E->ChunkStream);
    GrowToAccomodate(&E->ChunkStream, ChunkSize);
    WriteVarByte(&E->ChunkStream, C->NBricks);
    WriteStream(&E->ChunkStream, &C->BrickDeltasStream);
    WriteStream(&E->ChunkStream, &C->BrickSizeStream);
    WriteStream(&E->ChunkStream, &C->BrickStream);
    Flush(&E->ChunkStream);
    BitPlaneChunksStat.Add((f64)Size(E->ChunkStream));

    /* we are done with these, rewind */
    Rewind(&C->BrickDeltasStream);
    Rewind(&C->BrickSizeStream);
    Rewind(&C->BrickStream);

    u64 ChunkAddress = GetChunkAddress(Idx2, C->LastBrick, Level, Subband, BitPlane);
    buffer Buf = ToBuffer(E->ChunkStream);
    if (!Idx2.external_write(Idx2, Buf, ChunkAddress))
      idx2_ExitIf(true, "Write chunk failed\n");
    Rewind(&E->ChunkStream);
    return;
  }
#endif

  BrickDeltasStat.Add((f64)Size(C->BrickDeltasStream)); // brick deltas
  BrickSizesStat.Add((f64)Size(C->BrickSizeStream));       // brick sizes
  i64 ChunkSize = Size(C->BrickDeltasStream) + Size(C->BrickSizeStream) + Size(C->BrickStream) + 64;
  Rewind(&E->ChunkStream);
  GrowToAccomodate(&E->ChunkStream, ChunkSize);
  WriteVarByte(&E->ChunkStream, C->NBricks);
  WriteStream(&E->ChunkStream, &C->BrickDeltasStream);
  WriteStream(&E->ChunkStream, &C->BrickSizeStream);
  WriteStream(&E->ChunkStream, &C->BrickStream);
  Flush(&E->ChunkStream);
  BitPlaneChunksStat.Add((f64)Size(E->ChunkStream));

  /* we are done with these, rewind */
  Rewind(&C->BrickDeltasStream);
  Rewind(&C->BrickSizeStream);
  Rewind(&C->BrickStream);

  /* write to file */
  file_id FileId = ConstructFilePath(Idx2, C->LastBrick, Level, Subband, BitPlane);
  idx2_OpenMaybeExistingFile(Fp, FileId.Name.ConstPtr, "ab");
  WriteBuffer(Fp, ToBuffer(E->ChunkStream));
  /* keep track of the chunk addresses and sizes */
  auto ChunkMetaIt = Lookup(E->ChunkMeta, FileId.Id);
  if (!ChunkMetaIt)
  {
    chunk_meta_info Cm;
    InitWrite(&Cm.Sizes, 128);
    Insert(&ChunkMetaIt, FileId.Id, Cm);
  }
  idx2_Assert(ChunkMetaIt);
  chunk_meta_info* ChunkMeta = ChunkMetaIt.Val;
  GrowToAccomodate(&ChunkMeta->Sizes, 4);
  // Write the size of the chunk stream
  WriteVarByte(&ChunkMeta->Sizes, Size(E->ChunkStream));
  u64 ChunkAddress = GetChunkAddress(Idx2, C->LastBrick, Level, Subband, BitPlane);
  PushBack(&ChunkMeta->Addrs, ChunkAddress);
  Rewind(&E->ChunkStream);
}


// TODO: check the error path
/* We need to "flush" chunks because each chunk is written to disk when a new chunk is encountered,
however, at the end of the volume, there is no new chunk, so we write the last few chunks by "flushing"
them. Also, for each file, the FlushChunks function writes the metadata for all chunks in the file. */
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
* H : int32  = number of bit plane chunks
* I : int32  = size of J
* J : buffer = (zstd compressed) bit plane chunk addresses
* K : int32  = size (in bytes) of L
* L : buffer = (varint compressed) sizes of the bit plane chunks
* M : H buffers, whose sizes are encoded in L, each being one bit plane chunk
*/
error<idx2_err_code>
FlushChunks(const idx2_file& Idx2, encode_data* E)
{
#if VISUS_IDX2
  if (Idx2.external_write)
  {
    Reserve(&E->SortedChannels, Size(E->Channels));
    Clear(&E->SortedChannels);
    idx2_ForEach (Ch, E->Channels)
    {
      PushBack(&E->SortedChannels, t2<u32, channel*>{ *Ch.Key, Ch.Val });
    }
    InsertionSort(Begin(E->SortedChannels), End(E->SortedChannels));
    idx2_ForEach (Ch, E->SortedChannels)
    {
      i8 Level = GetLevelFromChannelKey(Ch->First);
      i8 Subband = GetSubbandFromChannelKey(Ch->First);
      i16 BitPlane = BitPlaneFromChannelKey(Ch->First);
      // printf("key %llu level %d subband %d bitplane %d\n", Ch->First, Level, Subband, BitPlane);
      WriteChunk(Idx2, E, Ch->Second, Level, Subband, BitPlane); //just call WriteChunk
    }
    return idx2_Error(idx2_err_code::NoError);
  }
#endif
  /* write the chunks */
  Reserve(&E->SortedChannels, Size(E->Channels));
  Clear(&E->SortedChannels);
  idx2_ForEach (Ch, E->Channels)
  {
    PushBack(&E->SortedChannels, t2<u32, channel*>{ *Ch.Key, Ch.Val });
  }
  InsertionSort(Begin(E->SortedChannels), End(E->SortedChannels));
  idx2_ForEach (Ch, E->SortedChannels)
  {
    i8 Level = GetLevelFromChannelKey(Ch->First);
    i8 Subband = GetSubbandFromChannelKey(Ch->First);
    i16 BitPlane = BitPlaneFromChannelKey(Ch->First);
    //printf("key %llu level %d subband %d bitplane %d\n", Ch->First, Level, Subband, BitPlane);
    WriteChunk(Idx2, E, Ch->Second, Level, Subband, BitPlane);
  }

  /* write the chunk metadata */
  idx2_ForEach (CmIt, E->ChunkMeta)
  {
    chunk_meta_info* Cm = CmIt.Val;
    file_id FileId = ConstructFilePath(Idx2, *CmIt.Key);
    if (FileId.Id != *CmIt.Key)
    {
      FileId = ConstructFilePath(Idx2, *CmIt.Key);
    }
    //printf("%llu %s\n", FileId.Id, FileId.Name.ConstPtr);
    idx2_Assert(FileId.Id == *CmIt.Key);
    /* compress and write chunk sizes */
    idx2_OpenMaybeExistingFile(Fp, FileId.Name.ConstPtr, "ab");
    Flush(&Cm->Sizes);
    WriteBuffer(Fp, ToBuffer(Cm->Sizes));
    ChunkSizesStat.Add((f64)Size(Cm->Sizes));
    WritePOD(Fp, (int)Size(Cm->Sizes));
    /* compress and write chunk addresses */
    CompressBufZstd(ToBuffer(Cm->Addrs), &E->CompressedChunkAddresses);
    WriteBuffer(Fp, ToBuffer(E->CompressedChunkAddresses));
    // write size of the compressed chunk addresses
    WritePOD(Fp, (int)Size(E->CompressedChunkAddresses));
    WritePOD(Fp, (int)Size(Cm->Addrs)); // number of chunks
    UncompressedChunkAddressesStat.Add((f64)Size(Cm->Addrs) * sizeof(Cm->Addrs[0]));
    CompressedChunkAddressesStat.Add((f64)Size(E->CompressedChunkAddresses));
  }

  return idx2_Error(idx2_err_code::NoError);
}


void
PrintStats(cstr MetaFileName)
{
  FILE* Fp = fopen(MetaFileName, "a");
  fprintf(Fp, "\n\n---------------- Statistics ----------------\n\n");
  fprintf(Fp, "num chunks = %" PRIi64 " (bit plane) and %" PRIi64 " (exponent) \n", BitPlaneChunksStat.Count(), CompressedExpChunksStat.Count());
  fprintf(Fp, "bit plane chunk: total = %d avg = %d stddev = %d bytes\n",
         (int)BitPlaneChunksStat.Sum(),
         (int)BitPlaneChunksStat.Avg(),
         (int)BitPlaneChunksStat.StdDev());
  fprintf(Fp, "  (metadata) brick deltas = %d bytes\n", (int)BrickDeltasStat.Sum());
  fprintf(Fp, "  (metadata) brick sizes = %d bytes\n", (int)BrickSizesStat.Sum());
  fprintf(Fp, "  (metadata) chunk sizes = %d bytes\n", (int)ChunkSizesStat.Sum());
  fprintf(Fp, "  (metadata) chunk addresses: uncompressed = %d, compressed = %d bytes\n",
         (int)UncompressedChunkAddressesStat.Sum(), (int)CompressedChunkAddressesStat.Sum());
  fprintf(Fp, "exponent chunk:\n");
  fprintf(Fp, "  uncompressed: total = %d bytes\n", (int)UncompressedExpChunksStat.Sum());
  fprintf(Fp, "  compressed: total = %d avg = %d stddev = %d bytes\n",
         (int) CompressedExpChunksStat.Sum(),
         (int) CompressedExpChunksStat.Avg(),
         (int) CompressedExpChunksStat.StdDev());
  fprintf(Fp, "  (metadata) chunk sizes = %d bytes\n", (int)ExpChunkSizesStat.Sum());
  fprintf(Fp, "  (metadata) chunk addresses: uncompressed = %d, compressed = %d bytes\n",
         (int)UncompressedExpChunkAddressesStat.Sum(), (int)CompressedExpChunkAddressesStat.Sum());
  fclose(Fp);
}


} // namespace idx2

