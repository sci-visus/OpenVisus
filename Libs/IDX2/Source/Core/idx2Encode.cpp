#include "idx2Encode.h"
#include "FileSystem.h"
#include "Function.h"
#include "InputOutput.h"
#include "Statistics.h"
#include "Timer.h"
#include "VarInt.h"
#include "Zfp.h"
#include "idx2Common.h"
#include "idx2Lookup.h"
#include "idx2SparseBricks.h"
#include "idx2Write.h"
#include "sexpr.h"
#include "zstd/zstd.h"
#include <algorithm>


namespace idx2
{


void
CompressBufZstd(const buffer& Input, bitstream* Output)
{
  if (Size(Input) == 0)
    return;
  size_t const MaxDstSize = ZSTD_compressBound(Size(Input));
  GrowToAccomodate(Output, MaxDstSize - Size(*Output));
  size_t const CpresSize =
    ZSTD_compress(Output->Stream.Data, MaxDstSize, Input.Data, Size(Input), 1);
  if (CpresSize <= 0)
  {
    fprintf(stderr, "CompressBufZstd failed\n");
    exit(1);
  }
  Output->BitPtr = CpresSize + Output->Stream.Data;
}


static void
EncodeBrickSubbandExponents(idx2_file* Idx2,
                            encode_data* E,
                            const v3i& NBlocks3,
                            const u64 Brick)
{

  /* query the right sub channel for the block exponents */
  const u64 SubChanKey = GetAddress(0, 0, E->Level, E->Subband, ExponentBitPlane_);
  auto ScIt = Lookup(E->SubChannels, SubChanKey);
  if (!ScIt)
  {
    sub_channel SubChan;
    Init(&SubChan);
    Insert(&ScIt, SubChanKey, SubChan);
  }
  idx2_Assert(ScIt);
  sub_channel* Sc = ScIt.Val;

  /* write the last chunk exponents if this is the first brick of the new chunk */
  bool NewChunk = Brick >= (Sc->LastChunk + 1) * Idx2->BricksPerChunk[E->Level];
  if (NewChunk)
  {
    WriteChunkExponents(*Idx2, E, Sc, E->Level, E->Subband);
    Sc->LastChunk = Brick >> Log2Ceil(Idx2->BricksPerChunk[E->Level]);
  }
  /* write the min exponent */
  GrowToAccomodate(&Sc->BlockExpStream, 2 * Size(E->SubbandExps));
  idx2_For (int, I, 0, Size(E->SubbandExps))
  {
    i16 S = E->SubbandExps[I] + (SizeOf(Idx2->DType) > 4 ? traits<f64>::ExpBias : traits<f32>::ExpBias);
    // we use 16 bits for f64 exponents (instead of 11) so that zstd compression works later
    Write(&Sc->BlockExpStream, S, SizeOf(Idx2->DType) > 4 ? 16 : traits<f32>::ExpBits);
  }
  /* write brick exponent size */
  GrowToAccomodate(&Sc->BrickExpStream, Size(Sc->BlockExpStream));
  WriteStream(&Sc->BrickExpStream, &Sc->BlockExpStream);
  //BlockEMaxStat.Add((f64)Size(Sc->BlockEMaxesStream));
  Rewind(&Sc->BlockExpStream);
  Sc->LastBrick = Brick;
}


static void
EncodeBrickSubbandMetadata(idx2_file* Idx2,
                           encode_data* E,
                           const v3i& NBlocks3,
                           const v3i& SbDims3,
                           const u64 Brick,
                           const u32 LastBlock)
{
  /* pass 2: encode the brick meta info */
  idx2_InclusiveFor (u32, Block, 0, LastBlock)
  {
    v3i Z3(DecodeMorton3(Block));
    idx2_NextMorton(Block, Z3, NBlocks3);
    v3i D3 = Z3 * Idx2->BlockDims3;
    v3i BlockDims3 = Min(Idx2->BlockDims3, SbDims3 - D3);
    bool CodedInNextLevel =
      E->Subband == 0 && E->Level + 1 < Idx2->NLevels && BlockDims3 == Idx2->BlockDims3;
    if (CodedInNextLevel)
      continue;
    /* done at most once per brick, by the last significant block */
    idx2_For (int, I, 0, Size(E->LastSigBlock))
    {
      i16 BpKey = E->LastSigBlock[I].BitPlane;
      if (Block != E->LastSigBlock[I].Block)
        continue;

      u32 ChannelKey = GetChannelKey(BpKey, E->Level, E->Subband);
      auto ChannelIt = Lookup(E->Channels, ChannelKey);
      idx2_Assert(ChannelIt);
      channel* C = ChannelIt.Val;
      /* write brick delta */
      if (C->NBricks == 0)
      { // start of a chunk
        GrowToAccomodate(&C->BrickDeltasStream, 8);
        WriteVarByte(&C->BrickDeltasStream, Brick);
      }
      else
      {
        GrowToAccomodate(&C->BrickDeltasStream, (Brick - C->LastBrick - 1 + 8) / 8);
        WriteUnary(&C->BrickDeltasStream, u32(Brick - C->LastBrick - 1));
      }
      /* write brick size */
      i64 BrickSize = Size(C->BlockStream);
      GrowToAccomodate(&C->BrickSizeStream, 4);
      WriteVarByte(&C->BrickSizeStream, BrickSize);
      /* write brick data */
      GrowToAccomodate(&C->BrickStream, BrickSize);
      WriteStream(&C->BrickStream, &C->BlockStream);
      //BlockStat.Add((f64)Size(C->BlockStream));
      Rewind(&C->BlockStream);
      ++C->NBricks;
      C->LastBrick = Brick;
    } // end bit plane loop
  }   // end zfp block loop
}


static void
EncodeSubbandBlocks(idx2_file* Idx2,
                    encode_data* E,
                    const grid& SbGrid,
                    const v3i& SbDims3,
                    const v3i& NBlocks3,
                    const u64 Brick,
                    const u32 LastBlock,
                    volume* BrickVol)
{
  const i8 NBitPlanes = idx2_BitSizeOf(u64);
  Clear(&E->LastSigBlock);
  Reserve(&E->LastSigBlock, NBitPlanes);
  Clear(&E->SubbandExps);
  Reserve(&E->SubbandExps, Prod(NBlocks3));

  /* pass 1: compress the blocks */
  idx2_InclusiveFor (u32, Block, 0, LastBlock)
  { // zfp block loop
    v3i Z3(DecodeMorton3(Block));
    idx2_NextMorton(Block, Z3, NBlocks3);
    v3i D3 = Z3 * Idx2->BlockDims3;
    v3i BlockDims3 = Min(Idx2->BlockDims3, SbDims3 - D3);
    const i8 NDims = (i8)NumDims(BlockDims3);
    const int NVals = 1 << (2 * NDims);
    const i8 Prec = NBitPlanes - 1 - NDims;
    f64 BlockFloats[4 * 4 * 4];
    buffer_t BufFloats(BlockFloats, NVals);
    buffer_t BufInts((i64*)BlockFloats, NVals);
    u64 BlockUInts[4 * 4 * 4];
    buffer_t BufUInts(BlockUInts, NVals);
    bool CodedInNextLevel =
      E->Subband == 0 && E->Level + 1 < Idx2->NLevels && BlockDims3 == Idx2->BlockDims3;
    if (CodedInNextLevel)
      continue;

    /* copy the samples to the local buffer and zfp transform them */
    v3i S3;
    int J = 0;
    v3i From3 = From(SbGrid), Strd3 = Strd(SbGrid);
    idx2_BeginFor3 (S3, v3i(0), BlockDims3, v3i(1))
    { // sample loop
      idx2_Assert(D3 + S3 < SbDims3);
      BlockFloats[J++] = BrickVol->At<f64>(From3, Strd3, D3 + S3);
    }
    idx2_EndFor3; // end sample loop
    /* zfp transform and shuffle */
    const i16 EMax = SizeOf(Idx2->DType) > 4 ? (i16)QuantizeF64(Prec, BufFloats, &BufInts)
                                             : (i16)QuantizeF32(Prec, BufFloats, &BufInts);
    PushBack(&E->SubbandExps, EMax);
    ForwardZfp((i64*)BlockFloats, NDims);
    ForwardShuffle((i64*)BlockFloats, BlockUInts, NDims);

    /* zfp encode */
    i8 N = 0; // number of significant coefficients in the block so far
    i8 EndBitPlane = Min(i8(BitSizeOf(Idx2->DType)), NBitPlanes);
    int Bpc = Idx2->BitPlanesPerChunk;
    idx2_InclusiveForBackward (i8, Bp, NBitPlanes - 1, NBitPlanes - EndBitPlane)
    { // bit plane loop
      i16 RealBp = Bp + EMax;
      i16 BpKey = (RealBp + BitPlaneKeyBias_) / Bpc; // make it so that the BpKey is positive
      bool TooHighPrecision = NBitPlanes - 6 > RealBp - Exponent(Idx2->Tolerance) + 1;
      if (TooHighPrecision)
      {
        if ((RealBp + BitPlaneKeyBias_) % Bpc == 0) // make sure we encode full "block" of BpKey
          break;
      }

      u32 ChannelKey = GetChannelKey(BpKey, E->Level, E->Subband);
      auto ChannelIt = Lookup(E->Channels, ChannelKey);
      if (!ChannelIt)
      {
        channel Channel;
        Init(&Channel);
        Insert(&ChannelIt, ChannelKey, Channel);
      }
      idx2_Assert(ChannelIt);
      channel* C = ChannelIt.Val;

      /* record the last significant block on this bit plane */
      // u32 BlockDelta = Block;
      int I = 0;
      for (; I < Size(E->LastSigBlock); ++I)
      {
        if (E->LastSigBlock[I].BitPlane == BpKey)
        {
          idx2_Assert(Block > E->LastSigBlock[I].Block);
          // BlockDelta = Block - E->BlockSigs[I].Block - 1;
          E->LastSigBlock[I].Block = Block;
          break;
        }
      }

      /* write chunk if this brick is after the last chunk */
      // first block that becomes significant on this bit plane
      // TODO: need to think about the logic of LastSigBlock in conjunction with BpKey
      bool FirstSigBlock = (I == Size(E->LastSigBlock));
      bool BrickNotEmpty = Size(C->BrickStream) > 0;
      bool NewChunk = Brick >= (C->LastChunk + 1) * Idx2->BricksPerChunk[E->Level];
      if (FirstSigBlock)
      {
        if (NewChunk)
        {
          if (BrickNotEmpty)
            WriteChunk(*Idx2, E, C, E->Level, E->Subband, BpKey);
          C->NBricks = 0;
          C->LastChunk = Brick >> Log2Ceil(Idx2->BricksPerChunk[E->Level]);
        }
        PushBack(&E->LastSigBlock, block_sig{ Block, BpKey });
      }

      /* encode the block */
      GrowIfTooFull(&C->BlockStream);
      Encode(BlockUInts, NVals, Bp, N, &C->BlockStream);
    } // end bit plane loop
  } // end zfp block loop
}


  // TODO: return an error code
static void
EncodeSubband(idx2_file* Idx2, encode_data* E, const grid& SbGrid, volume* BrickVol)
{
  const u64 Brick = E->Brick[E->Level];
  const v3i SbDims3 = Dims(SbGrid);
  const v3i NBlocks3 = (SbDims3 + Idx2->BlockDims3 - 1) / Idx2->BlockDims3;
  const u32 LastBlock = EncodeMorton3(v3<u32>(NBlocks3 - 1));

  EncodeSubbandBlocks(Idx2, E, SbGrid, SbDims3, NBlocks3, Brick, LastBlock, BrickVol);
  EncodeBrickSubbandExponents(Idx2, E, NBlocks3, Brick);
  EncodeBrickSubbandMetadata(Idx2, E, NBlocks3, SbDims3, Brick, LastBlock);

}


static void
EncodeBrick(idx2_file* Idx2, const params& P, encode_data* E, bool IncrementLevel = false)
{
  idx2_Assert(Idx2->NLevels <= idx2_file::MaxLevels);

  i8 Level = E->Level += IncrementLevel;

  u64 Brick = E->Brick[Level];
  //printf(
  //  "level %d brick " idx2_PrStrV3i " %" PRIu64 "\n", Iter, idx2_PrV3i(E->Bricks3[Iter]), Brick);
  auto BIt = Lookup(E->BrickPool, GetBrickKey(Level, Brick));
  idx2_Assert(BIt);
  volume& BVol = BIt.Val->Vol;
  idx2_Assert(BVol.Buffer);

  // TODO: we do not need to pre-extrapolate, instead just compute and store the extrapolated values
  ExtrapolateCdf53(Dims(BIt.Val->ExtentLocal), Idx2->TransformOrder, &BVol);

  /* do wavelet transform */
  bool CoarsestLevel = Level + 1 == Idx2->NLevels; // only normalize
  ForwardCdf53(Idx2->BrickDimsExt3, E->Level, Idx2->Subbands, Idx2->TransformDetails, &BVol, CoarsestLevel);

  /* recursively encode the brick, one subband at a time */
  idx2_For (i8, Sb, 0, Size(Idx2->Subbands))
  { // subband loop
    const subband& S = Idx2->Subbands[Sb];
    v3i SbDimsNonExt3 = idx2_NonExtDims(Dims(S.Grid));
    i8 NextLevel = Level + 1;
    if (Sb == 0 && NextLevel < Idx2->NLevels)
    { // need to encode the parent brick
      /* find the parent brick and create it if not found */
      v3i Brick3 = E->Bricks3[Level];
      v3i PBrick3 = (E->Bricks3[NextLevel] = Brick3 / Idx2->GroupBrick3);
      u64 PBrick = (E->Brick[NextLevel] = GetLinearBrick(*Idx2, NextLevel, PBrick3));
      u64 PKey = GetBrickKey(NextLevel, PBrick);
      auto PbIt = Lookup(E->BrickPool, PKey);
      if (!PbIt)
      { // instantiate the parent brick in the hash table
        brick_volume PBrickVol;
        Resize(&PBrickVol.Vol, Idx2->BrickDimsExt3, dtype::float64, E->Alloc);
        Fill(idx2_Range(f64, PBrickVol.Vol), 0.0);
        v3i From3 = (Brick3 / Idx2->GroupBrick3) * Idx2->GroupBrick3;
        v3i NChildren3 =
          Dims(Crop(extent(From3, Idx2->GroupBrick3), extent(Idx2->NBricks3[Level])));
        PBrickVol.NChildrenMax = (i8)Prod(NChildren3);
        PBrickVol.ExtentLocal = extent(NChildren3 * SbDimsNonExt3);
        Insert(&PbIt, PKey, PBrickVol);
      }
      /* copy data to the parent brick and (optionally) encode it */
      v3i LocalBrickPos3 = Brick3 % Idx2->GroupBrick3;
      grid SbGridNonExt = S.Grid;
      SetDims(&SbGridNonExt, SbDimsNonExt3);
      extent ToGrid(LocalBrickPos3 * SbDimsNonExt3, SbDimsNonExt3);
      CopyGridExtent<f64, f64>(SbGridNonExt, BVol, ToGrid, &PbIt.Val->Vol);
      //      Copy(SbGridNonExt, BVol, ToGrid, &PbIt.Val->Vol);
      bool LastChild = ++PbIt.Val->NChildrenDecoded == PbIt.Val->NChildrenMax;
      if (LastChild)
        EncodeBrick(Idx2, P, E, true);
    } // end Sb == 0 && NextIteration < Idx2->NLevels
    E->Subband = Sb;
    EncodeSubband(Idx2, E, S.Grid, &BVol);
  } // end subband loop
  Dealloc(&BVol);
  Delete(&E->BrickPool, GetBrickKey(Level, Brick));
  E->Level -= IncrementLevel;
}


// TODO: return true error code
struct channel_ptr
{
  i8 Iteration = 0;
  i8 Level = 0;
  i16 BitPlane = 0;
  channel* ChunkPtr = nullptr;
  idx2_Inline bool operator<(const channel_ptr& Other) const
  {
    if (Iteration == Other.Iteration)
    {
      if (BitPlane == Other.BitPlane)
        return Level < Other.Level;
      return BitPlane > Other.BitPlane;
    }
    return Iteration < Other.Iteration;
  }
};


f64 TotalTime_ = 0; // TODO: move to somewhere


/*
By default, copy brick data from a volume to a local brick buffer.
Can be extended polymorphically to provide other ways of copying.
*/
brick_copier::brick_copier(const volume* InputVolume)
{
  Volume = InputVolume;
}


v2d
brick_copier::Copy(const extent& ExtentGlobal, const extent& ExtentLocal, brick_volume* Brick)
{
  v2d MinMax;
  if (Volume->Type == dtype::float32)
    MinMax = (CopyExtentExtentMinMax<f32, f64>(ExtentGlobal, *Volume, ExtentLocal, &Brick->Vol));
  else if (Volume->Type == dtype::float64)
    MinMax = (CopyExtentExtentMinMax<f64, f64>(ExtentGlobal, *Volume, ExtentLocal, &Brick->Vol));

  return MinMax;
}


error<idx2_err_code>
Encode(idx2_file* Idx2, const params& P, brick_copier& Copier)
{
  const int BrickBytes = Prod(Idx2->BrickDimsExt3) * sizeof(f64);
  BrickAlloc_ = free_list_allocator(BrickBytes);
  idx2_RAII(encode_data, E, Init(&E));
  idx2_BrickTraverse(
    timer Timer; StartTimer(&Timer);
    //    idx2_Assert(GetLinearBrick(*Idx2, 0, Top.BrickFrom3) == Top.Address);
    //    idx2_Assert(GetSpatialBrick(*Idx2, 0, Top.Address) == Top.BrickFrom3);
    // BVol = local brick storage (we will copy brick data from the input to this)
    brick_volume BVol;
    Resize(&BVol.Vol, Idx2->BrickDimsExt3, dtype::float64, E.Alloc);
    Fill(idx2_Range(f64, BVol.Vol), 0.0);
    extent BrickExtent(Top.BrickFrom3 * Idx2->BrickDims3, Idx2->BrickDims3);
    // BrickExtentCrop = the true extent of the brick (boundary bricks are cropped)
    extent BrickExtentCrop = Crop(BrickExtent, extent(Idx2->Dims3));
    BVol.ExtentLocal = Relative(BrickExtentCrop, BrickExtent);
    v2d MinMax = Copier.Copy(BrickExtentCrop, BVol.ExtentLocal, &BVol);
    Idx2->ValueRange.Min = Min(Idx2->ValueRange.Min, MinMax.Min);
    Idx2->ValueRange.Max = Max(Idx2->ValueRange.Max, MinMax.Max);
    //    Copy(BrickExtentCrop, Vol, BVol.ExtentLocal, &BVol.Vol);
    E.Level = 0;
    E.Bricks3[E.Level] = Top.BrickFrom3;
    E.Brick[E.Level] = GetLinearBrick(*Idx2, E.Level, E.Bricks3[E.Level]);
    idx2_Assert(E.Brick[E.Level] == Top.Address);
    u64 BrickKey = GetBrickKey(E.Level, E.Brick[E.Level]);
    Insert(&E.BrickPool, BrickKey, BVol);
    EncodeBrick(Idx2, P, &E);
    TotalTime_ += Seconds(ElapsedTime(&Timer));
    ,
    128,
    Idx2->BricksOrder[E.Level],
    v3i(0),
    Idx2->NBricks3[E.Level],
    extent(Idx2->NBricks3[E.Level]),
    extent(Idx2->NBricks3[E.Level])
  );

  /* dump the bit streams to files */
  timer Timer;
  StartTimer(&Timer);
  idx2_PropagateIfError(FlushChunks(*Idx2, &E));
  idx2_PropagateIfError(FlushChunkExponents(*Idx2, &E));
  TotalTime_ += Seconds(ElapsedTime(&Timer));

  cstr MetaFileName = idx2_PrintScratch("%s/%s/%s.idx2", P.OutDir, P.Meta.Name, P.Meta.Field);
  WriteMetaFile(*Idx2, P, MetaFileName);
  printf("num channels            = %" PRIi64 "\n", Size(E.Channels));
  printf("num sub channels        = %" PRIi64 "\n", Size(E.SubChannels));
  MetaFileName = idx2_PrintScratch("%s/%s/%s.idx2", P.OutDir, P.Meta.Name, P.Meta.Field);
  PrintStats(MetaFileName);
  printf("total time              = %f seconds\n", TotalTime_);
  //  _ASSERTE( _CrtCheckMemory( ) );
  return idx2_Error(idx2_err_code::NoError);
}


// TODO: make sure the wavelet normalization works across levels
// TODO: add a mode that treats the chunks like a row in a table


void
Init(encode_data* E, allocator* Alloc)
{
  Init(&E->BrickPool, 9);
  Init(&E->Channels, 10);
  Init(&E->SubChannels, 5);
  E->Alloc = Alloc ? Alloc : &BrickAlloc_;
  Init(&E->ChunkMeta, 8);
  Init(&E->ChunkExponents, 5);
  //InitWrite(&E->CompressedExps, 32768);
  InitWrite(&E->CompressedChunkAddresses, 16384);
  InitWrite(&E->ChunkStream, 16384);
  InitWrite(&E->ChunkExpStream, 32768);
}


void
Dealloc(encode_data* E)
{
  E->Alloc->DeallocAll();
  Dealloc(&E->BrickPool);
  idx2_ForEach (ChannelIt, E->Channels)
    Dealloc(ChannelIt.Val);
  Dealloc(&E->Channels);
  idx2_ForEach (SubChannelIt, E->SubChannels)
    Dealloc(SubChannelIt.Val);
  Dealloc(&E->SubChannels);
  Dealloc(&E->SortedChannels);
  Dealloc(&E->SortedSubChannels);
  idx2_ForEach (ChunkMetaIt, E->ChunkMeta)
    Dealloc(ChunkMetaIt.Val);
  idx2_ForEach (ChunkExpIt, E->ChunkExponents)
    Dealloc(ChunkExpIt.Val);
  Dealloc(&E->ChunkMeta);
  //Dealloc(&E->CompressedExps);
  Dealloc(&E->CompressedChunkAddresses);
  Dealloc(&E->ChunkStream);
  Dealloc(&E->ChunkExpStream);
  Dealloc(&E->LastSigBlock);
  Dealloc(&E->SubbandExps);
  //Dealloc(&E->BlockStream);
}


void
Init(channel* C)
{
  InitWrite(&C->BrickStream, 16384);
  InitWrite(&C->BrickDeltasStream, 32);
  InitWrite(&C->BrickSizeStream, 256);
  InitWrite(&C->BlockStream, 256);
}


void
Dealloc(channel* C)
{
  Dealloc(&C->BrickDeltasStream);
  Dealloc(&C->BrickSizeStream);
  Dealloc(&C->BrickStream);
  Dealloc(&C->BlockStream);
}


void
Init(sub_channel* Sc)
{
  InitWrite(&Sc->BlockExpStream, 64);
  InitWrite(&Sc->BrickExpStream, 8192);
}


void
Dealloc(sub_channel* Sc)
{
  Dealloc(&Sc->BlockExpStream);
  Dealloc(&Sc->BrickExpStream);
}


void
Dealloc(chunk_meta_info* Cm)
{
  Dealloc(&Cm->Addrs);
  Dealloc(&Cm->Sizes);
}

void
Dealloc(chunk_exp_info* Ce)
{
  Dealloc(&Ce->Addrs);
  Dealloc(&Ce->ExpSizes);
  Dealloc(&Ce->FileExpBuffer);
}


} // namespace idx2

