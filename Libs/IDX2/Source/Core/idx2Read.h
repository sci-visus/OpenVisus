#pragma once


#include "Expected.h"
#include "HashTable.h"
#include "idx2Common.h"


namespace idx2
{


struct chunk_cache
{
  i32 ChunkPos; // chunk position in the offset array (also chunk order in the file)
  array<u64> Bricks;
  array<i32> BrickOffsets;
  bitstream ChunkStream;
  bool Ready = false;
};

void
Dealloc(chunk_cache* ChunkCache);


struct chunk_exp_cache
{
  i32 ChunkPos; // chunk position in the offset array
  bitstream ChunkExpStream;
  bool Ready = false;
};

void
Dealloc(chunk_exp_cache* ChunkExpCache);

// TODO: we just need a single cache table addressed by chunk id
struct file_cache
{
  array<i64> ChunkOffsets;                  // TODO: 32-bit to store chunk sizes?
  hash_table<u64, chunk_cache> ChunkCaches; // [chunk address] -> chunk cache
  hash_table<u64, chunk_exp_cache> ChunkExpCaches;
  array<i32> ChunkExpOffsets;
  i64 ExponentBeginOffset = 0;              // where in the file the exponent information begins
  bool ExpCached = false;
  bool DataCached = false;
};


void
Init(file_cache* FileCache);

void
Dealloc(file_cache* FileCache);

// [file address] -> file cache
using file_cache_table = hash_table<u64, file_cache>;


struct decode_data;


void
DeallocFileCacheTable(file_cache_table* FileCacheTable);


// TODO: not quite exhaustive
idx2_Inline i64
Size(const chunk_cache& C)
{
  return Size(C.Bricks) * sizeof(C.Bricks[0]) + Size(C.BrickOffsets) * sizeof(C.BrickOffsets[0]) +
         sizeof(C.ChunkPos) +
         Size(C.ChunkStream.Stream);
}


idx2_Inline i64
Size(const chunk_exp_cache& C)
{ return Size(C.ChunkExpStream.Stream) + sizeof(C.ChunkPos); }


idx2_Inline i64
Size(const file_cache& F)
{
  i64 Result = 0;
  Result += Size(F.ChunkOffsets) * sizeof(i64);
  idx2_ForEach (It, F.ChunkCaches)
    Result += Size(*It.Val);
  idx2_ForEach (It, F.ChunkExpCaches)
    Result += Size(*It.Val);
  Result += Size(F.ChunkExpOffsets) * sizeof(i32);
  return Result;
}


expected<const chunk_exp_cache*, idx2_err_code>
ReadChunkExponents(const idx2_file& Idx2, decode_data* D, u64 Brick, i8 Level, i8 Subband);


expected<const chunk_cache*, idx2_err_code>
ReadChunk(const idx2_file& Idx2, decode_data* D, u64 Brick, i8 Level, i8 Subband, i16 BitPlane);

expected<chunk_cache, idx2_err_code>
ParallelReadChunk(const idx2_file& Idx2, decode_data* D, u64 Brick, i8 Level, i8 Subband, i16 BpKey);

expected<chunk_exp_cache, idx2_err_code>
ParallelReadChunkExponents(const idx2_file& Idx2, decode_data* D, u64 Brick, i8 Level, i8 Subband);

} // namespace idx2

