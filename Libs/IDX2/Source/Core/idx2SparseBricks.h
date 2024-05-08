#pragma once


#include "Array.h"
#include "HashTable.h"
#include "Volume.h"
#include "idx2Common.h"


namespace idx2
{


// TODO: allow the bricks to have different resolutions depending on the subbands decoded
struct brick_volume
{
  // TODO: volume and extent also contains lots of extra unnecessary bits
  volume Vol;
  extent ExtentLocal; // dimensions of the brick // TODO: we do not need full extent, just dims v3i
  //extent ExtentGlobal; // global extent of the brick
  // TODO: we just need one single i8 to encode NChildrenDecoded, NChildrenMax, and Significant
  i8 NChildrenDecoded = 0;
  i8 NChildrenMax = 0;
  bool Significant = false; // if any (non 0) subband is decoded for this brick
  bool DoneDecoding = false;
};


void
Init(brick_volume* BrickVol);

void
Dealloc(brick_volume* BrickVol);


using brick_table = hash_table<u64, brick_volume>;

struct idx2_file;

struct brick_pool
{
  brick_table BrickTable;
  // We use 4 bits for each brick at the finest resolution to specify the finest resolution
  // at the spatial location of the brick (this depends on how much the refinement is at that
  // location)
  // TODO: to avoid storing a separate ResolutionLevels array, we can also "stuff" the 4 bits into
  // they key for BrickTable
  array<i8> ResolutionLevels;
  const idx2_file* Idx2 = nullptr;
};


void
Init(brick_pool* Bp, const idx2_file* Idx2);


void
Dealloc(brick_pool* Bp);


void
PrintStatistics(const brick_pool* Bp);


/* For all finest-resolution bricks, output the actual resolution */
void
ComputeBrickResolution(brick_pool* Bp);


/* Given a finest resolution brick, return the corresponding brick volume */
brick_volume
GetBrickVolume(brick_pool* Bp, const v3i& Brick3);


/* Upsample a brick to the finest resolution. The function may allocate new memory if the
input brick is of coarser resolution. */
brick_volume
UpsampleToFinest(const brick_volume& BrickVol, i8 InputResLevel);


error<idx2_err_code>
WriteBricks(brick_pool* Bp, cstr FileName);


/* Given a position, return the grid encompassing the point
Also, transforming the input point into a local frame of the brick ready for interpolation */
brick_volume
PointQuery(const brick_pool& Bp, v3i* P3);


/* Interpolate to get the value at a point */
f64
Interpolate(const v3i& P3, const brick_volume& Grid);


idx2_Inline i64
Size(const brick_volume& B)
{
  return Prod(Dims(B.Vol)) * SizeOf(B.Vol.Type);
}


} // namespace idx2

