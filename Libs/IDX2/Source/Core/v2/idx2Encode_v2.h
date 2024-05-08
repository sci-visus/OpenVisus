#pragma once

#include "../Array.h"
#include "../BitStream.h"
#include "../HashTable.h"
#include "../Memory.h"
#include "idx2Common_v2.h"
#include "../idx2SparseBricks.h"


namespace idx2
{


/* ---------------------- TYPES ----------------------*/

struct block_sig
{
  u32 Block = 0;
  i16 BitPlane = 0;
};


struct chunk_meta_info
{
  array<u64> Addrs; // iteration, level, bit plane, chunk id
  bitstream Sizes;  // TODO: do we need to init this?
};


struct chunk_exp_info
{
  bitstream ExpSizes;
  array<u64> Addrs;
  array<u8> FileExpBuffer; // buffer for a whole file
};


// Each channel corresponds to one (iteration, subband, bit plane) tuple
struct channel
{
  /* brick-related streams, to be reset once per chunk */
  bitstream BrickDeltasStream; // store data for many bricks
  bitstream BrickSizeStream;    // store data for many bricks
  bitstream BrickStream;       // store data for many bricks
  /* block-related streams, to be reset once per brick */
  bitstream BlockStream; // store data for many blocks
  u64 LastChunk = 0;     // current chunk
  u64 LastBrick = 0;
  i32 NBricks = 0;
};


struct channel_info
{
  i16 BitPlane = 0;
  i8 Level = 0;
  i8 Subband = 0;
  channel* Channel = nullptr;
};


// Each sub-channel corresponds to one (iteration, subband) tuple
struct sub_channel
{
  bitstream BlockExpStream;
  bitstream BrickExpStream; // at the end of each brick we copy from BlockEMaxesStream to here
  u64 LastChunk = 0;
  u64 LastBrick = 0;
};


struct sub_channel_info
{
  i8 Level = 0;
  i8 Subband = 0;
  sub_channel* SubChannel = nullptr;
  idx2_Inline bool operator<(const sub_channel_info& Other) const
  {
    if (Level == Other.Level)
      return Subband < Other.Subband;
    return Level > Other.Level; // TODO: should we sort by increasing level instead?
  }
};


/* We use this to pass data between different stages of the encoder */
struct encode_data
{
  allocator* Alloc = nullptr;
  brick_table BrickPool;
  // each corresponds to (level, subband, bit plane)
  hash_table<u32, channel> Channels;
  // only consider level and subband
  hash_table<u64, sub_channel> SubChannels;
  i8 Level = 0;
  i8 Subband = 0;
  stack_array<u64, idx2_file::MaxLevels> Brick;
  stack_array<v3i, idx2_file::MaxLevels> Bricks3;
  // map from file address to chunk info
  hash_table<u64, chunk_meta_info> ChunkMeta;
  // map from file address to a stream of chunk emax sizes
  // TODO: merge this with the above
  hash_table<u64, chunk_exp_info> ChunkExponents;
  //bitstream CompressedExps;
  bitstream CompressedChunkAddresses;
  bitstream ChunkStream;
  /* block emaxes related */
  bitstream ChunkExpStream;
  // last significant block on each bit plane on the current subband
  // can be used to do certain things only *once* per bit plane
  array<block_sig> LastSigBlock;
  array<i16> SubbandExps;
  //bitstream BlockStream; // only used by v0.1
  array<t2<u32, channel*>> SortedChannels;
  array<sub_channel_info> SortedSubChannels;
};


/*
By default, copy brick data from a volume to a local brick buffer.
Can be extended polymorphically to provide other ways of copying.
*/
struct brick_volume;
struct brick_copier
{
  const volume* Volume = nullptr;

  brick_copier() {};
  brick_copier(const volume* InputVolume);

  virtual v2d // {Min, Max} values of brick
  Copy(const extent& ExtentGlobal, const extent& ExtentLocal, brick_volume* Brick);
};


/* FUNCTIONS */

void
CompressBufZstd(const buffer& Input, bitstream* Output);

/* Encode a whole volume, assuming the volume is available  */
error<idx2_err_code>
Encode(idx2_file* Idx2, const params& P, brick_copier& Copier);

/* Encode a brick. Use this when the input data is not in the form of a big volume. */
error<idx2_err_code>
EncodeBrick(idx2_file* Idx2, const params& P, const v3i& BrickPos3);

error<idx2_err_code>
Encode_v2(idx2_file* Idx2, const params& P, brick_copier& Copier);

error<idx2_err_code>
EncodeBrick_v2(idx2_file* Idx2, const params& P, const v3i& BrickPos3);

void
Init(encode_data* E, allocator* Alloc = nullptr);

void
Dealloc(encode_data* E);

void
Init(channel* C);

void
Dealloc(channel* C);

void
Init(sub_channel* Sc);

void
Dealloc(sub_channel* Sc);

void
Dealloc(chunk_meta_info* Cm);

void
Dealloc(chunk_exp_info* Ce);


} // namespace idx2

