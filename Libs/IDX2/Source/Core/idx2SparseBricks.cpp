#include "BitOps.h"
#include "InputOutput.h"
#include "idx2SparseBricks.h"
#include "idx2Common.h"
#include "idx2Lookup.h"
//#include <unordered_set>

namespace idx2
{


void
Dealloc(brick_volume* BrickVol)
{
  Dealloc(&BrickVol->Vol);
}


void
Init(brick_pool* Bp, const idx2_file* Idx2)
{
  Bp->Idx2 = Idx2;
  i64 NFinestBricks = 1 + GetLinearBrick(*Idx2, 0, Idx2->NBricks3[0] - 1);
  Init(&Bp->BrickTable, 10);
  Resize(&Bp->ResolutionLevels, NFinestBricks);
}


void
Dealloc(brick_pool* Bp)
{
  Dealloc(&Bp->ResolutionLevels);
  idx2_ForEach (It, Bp->BrickTable)
    Dealloc(&It.Val->Vol);
  Dealloc(&Bp->BrickTable);
}


struct stack_item
{
  v3i Brick3 = v3i(0);
  i8 Level = -1;
  i8 ResolutionToSet = -1;
};


/* Print the percentage of significant bricks on each level */
void
PrintStatistics(const brick_pool* Bp)
{
  const idx2_file* Idx2 = Bp->Idx2;
  i64 Count[idx2_file::MaxLevels] = {};

  idx2_ForEach (BIt, Bp->BrickTable)
  {
    i8 Level = GetLevelFromBrickKey(*BIt.Key);
    ++Count[Level];
  }
  idx2_For (i8, L, 0, Idx2->NLevels)
  {
    i64 NBricks = Prod<i64>(Idx2->NBricks3[L]);
    f64 Percent = f64(Count[L]) * 100 / f64(NBricks);
    printf("level %d: %lld out of %lld bricks significant (%f percent)\n", L, Count[L], NBricks, Percent);
  }
}


void
ComputeBrickResolution(brick_pool* Bp)
{
  const idx2_file* Idx2 = Bp->Idx2;
  stack_item Stack[idx2_file::MaxLevels * 8];
  i8 LastIndex = 0;
  /* push all bricks at the coarsest level */
  v3i CurrCoarsestBrick = v3i(0);
  stack_item& First = Stack[LastIndex];
  First.Brick3 = CurrCoarsestBrick;
  First.Level = First.ResolutionToSet = Idx2->NLevels - 1;
  //i64 Count = 0;
  while (LastIndex >= 0)
  {
    // pop the stack
    stack_item Current = Stack[LastIndex--];
    // get the current brick
    u64 BrickIndex = GetLinearBrick(*Idx2, Current.Level, Current.Brick3);
    u64 BrickKey = GetBrickKey(Current.Level, BrickIndex);
    auto BrickIt = Lookup(Bp->BrickTable, BrickKey);

    if (BrickIt)
    {
      idx2_Assert(BrickIt.Val->Significant);
      Current.ResolutionToSet = Current.Level;
    }
    // push the children if not at the finest level
    if (Current.Level > 0)
    {
      i8 NextLevel = Current.Level - 1;
      for (int I = 0; I < 8; ++I)
      {
        int X = BitSet(I, 0);
        int Y = BitSet(I, 1);
        int Z = BitSet(I, 2);
        v3i ChildBrick3 = Current.Brick3 * Idx2->GroupBrick3 + v3i(X, Y, Z);
        if (ChildBrick3 < Idx2->NBricks3[NextLevel])
        {
          stack_item& Child = Stack[++LastIndex];
          Child.Brick3 = ChildBrick3;
          Child.Level = Current.Level - 1;
          Child.ResolutionToSet = Current.ResolutionToSet;
        }
      }
    }
    else // finest level, set the level if necessary
    {
      // TODO: we should just stuff 4 bits into the brick key instead of using a separate
      // ResolutionStream
      if (Current.ResolutionToSet > 0)
      {// by default, the bit stream is init to 0 so no need to write if ResolutionToSet == 0
        Bp->ResolutionLevels[BrickIndex] = Current.ResolutionToSet;
      }
      //printf("level = %d\n", Current.ResolutionToSet);
      //v3i Brick3 = GetSpatialBrick(*Idx2, Current.Level, BrickIndex);
      //idx2_Assert(Brick3 == Current.Brick3);
      //++Count;
    }

    /* push the next brick at the coarsest resolution if stack is empty */
    if (LastIndex < 0)
    {
      ++CurrCoarsestBrick.X;
      if (CurrCoarsestBrick.X >= Idx2->NBricks3[Idx2->NLevels - 1].X)
      {
        CurrCoarsestBrick.X = 0;
        ++CurrCoarsestBrick.Y;
        if (CurrCoarsestBrick.Y >= Idx2->NBricks3[Idx2->NLevels - 1].Y)
        {
          CurrCoarsestBrick.Y = 0;
          ++CurrCoarsestBrick.Z;
        }
      }
      if (CurrCoarsestBrick < Idx2->NBricks3[Idx2->NLevels - 1])
      {
        stack_item& Next = Stack[++LastIndex];
        Next.Brick3 = CurrCoarsestBrick;
        Next.Level = Next.ResolutionToSet = Idx2->NLevels - 1;
      }
    }
  }

  //printf("num finest bricks = %" PRIi64 "\n", Count);
}


/* If the brick exists, just return its brick_volume, else we compute the brick_volume from
its ancestor (using the ResolutionStream). */
brick_volume
GetBrickVolume(brick_pool* Bp, const v3i& Brick3)
{
  const i8 Level = 0;
  u64 BrickIndex = GetLinearBrick(*Bp->Idx2, Level, Brick3);
  i8 Resolution = Bp->ResolutionLevels[BrickIndex];

  if (Resolution == 0)
  {
    u64 BrickKey = GetBrickKey(Level, BrickIndex);
    auto BrickIt = Lookup(Bp->BrickTable, BrickKey);
    idx2_Assert(BrickIt);
    BrickIt.Val->ExtentLocal = extent(Bp->Idx2->BrickDimsExt3);
    return *BrickIt.Val;
  }

  /* if resolution > 0, we need to find the ancestor */
  v3i GroupBrick3 = Pow(Bp->Idx2->GroupBrick3, Resolution);
  v3i ABrick3 = Brick3 / GroupBrick3;
  u64 ABrick = GetLinearBrick(*Bp->Idx2, Resolution, ABrick3);
  u64 AKey = GetBrickKey(Resolution, ABrick);
  auto AbIt = Lookup(Bp->BrickTable, AKey);
  idx2_Assert(AbIt);
  v3i D3 = Dims(AbIt.Val->Vol);
  v3i E3 = Dims(AbIt.Val->ExtentLocal);

  v3i LocalBrickPos3 = Brick3 % GroupBrick3;
  v3i BrickDims3 = (Bp->Idx2->BrickDims3 / (1 << Resolution));
  v3i BrickDimsExt3 = idx2_ExtDims(BrickDims3);
  brick_volume BrickVol = *AbIt.Val;
  BrickVol.ExtentLocal = extent(LocalBrickPos3 * BrickDims3, BrickDimsExt3);
  BrickVol.Significant = false;
  // BrickVol.NChildrenMax or NChildrenDecoded?

  return BrickVol;
}

/* File structure:
* 3 int32s (Nx Ny Nz): dimensions of the full volume (e.g., 512 512 512)
* 3 int32s (Bx By Bz): finest brick dimensions (e.g., 32 32 32)
*   (these are always power of 2)
* 3 ints32 (NBx, NBy, NBz): number of bricks in each dimension
* For each brick (in row major order):
*   3 int32s (Bi Bj Bk): brick's indices (e.g., 1 0 1) (multiply this by the
*     (Bx By Bz) above to get the brick's location in voxel)
*   3 int32s (Dx Dy Dz): true dimensions of the brick (e.g., 9 9 9)
*     (these are always power of 2 + 1 and are at most Bx+1 By+1 Bz+1)
*   Dx x Dy x Dz float64s: brick sample values in row major order
*     (for simplicity we use float64 regardless of the original type)
*/
error<idx2_err_code>
WriteBricks(brick_pool* Bp, cstr FileName)
{
  idx2_RAII(FILE*, Fp = fopen(FileName, "wb"), , if (Fp) fclose(Fp));
  if (!Fp)
    return idx2_Error(idx2_err_code::FileCreateFailed);

  WritePOD(Fp, Bp->Idx2->Dims3);
  WritePOD(Fp, Bp->Idx2->BrickDims3);
  WritePOD(Fp, Bp->Idx2->NBricks3[0]);

  v3i B3;
  idx2_BeginFor3 (B3, v3i(0), Bp->Idx2->NBricks3[0], v3i(1))
  {
    //u64 BrickIndex = GetLinearBrick(*Bp->Idx2, 0, B3);
    WritePOD(Fp, B3);
    brick_volume BrickVol = GetBrickVolume(Bp, B3);
    v3i D3 = Dims(BrickVol.ExtentLocal);
    WritePOD(Fp, Dims(BrickVol.ExtentLocal));
    v3i F3 = From(BrickVol.ExtentLocal);
    WriteVolume(Fp, BrickVol.Vol, grid(BrickVol.ExtentLocal));
  } idx2_EndFor3

  return idx2_Error(idx2_err_code::NoError);
}


// NOTE: this should not be used in "production", it is very inefficient
// in production, we simply do interpolation straight from coarse samples
// This function is to just verify that the finest bricks we generate are correct
//brick_volume
//UpsampleToFinest(const idx2_file& Idx2, const brick_volume& BrickVol, i8 InputResLevel)
//{
//  if (InputResLevel == 0)
//    return BrickVol;
//
//  // allocate new storage for the brick
//  brick_volume BVolFrom = BrickVol;
//  brick_volume BVolTo;
//  i8 Level = InputResLevel;
//  while (Level > 0)
//  {
//    Resize(&BVolTo.Vol, Idx2.BrickDimsExt3, dtype::float64);
//    ZeroBuf(&BVolTo.Vol.Buffer);
//    // copy the data over
//    grid GridTo;
//    SetFrom(&GridTo, v3i(0)); // TODO: this is wrong, we need to determine this from the extent local
//    SetDims(&GridTo, Dims(BrickVol.ExtentLocal));
//    SetStrd(&GridTo, v3i(1 << InputResLevel)); // TODO: should be dependent on the Idx2.BrickDims
//    // TODO: what about other types?
//    CopyExtentGrid<f64, f64>(BrickVol.ExtentLocal, BrickVol.Vol, GridTo, &BVol.Vol);
//    bool CoarsestLevel = InputResLevel + 1 == Idx2.NLevels;
//    // TODO: the following only inverse once
//    InverseCdf53(Idx2.BrickDimsExt3, InputResLevel, Idx2.Subbands, Idx2.Td, &BVol.Vol, CoarsestLevel);
//    --Level;
//  }
//
//  return BVol;
//}


brick_volume
PointQuery(const brick_pool& Bp, v3i* P3)
{
  return brick_volume();
}


// TODO
f64
Interpolate(const v3i& P3, const grid& Grid)
{
  return 0;
}


} // namespace idx2

