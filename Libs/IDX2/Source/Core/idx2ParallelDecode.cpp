#include "Array.h"
#include "BitStream.h"
#include "Expected.h"
#include "Function.h"
#include "HashTable.h"
#include "Memory.h"
#include "Timer.h"
#include "VarInt.h"
#include "Zfp.h"
#include "idx2Decode.h"
#include "idx2Lookup.h"
#include "idx2Read.h"
#include "idx2SparseBricks.h"
#include "sexpr.h"
#include "zstd/zstd.h"
#include "thread-pool/BS_thread_pool.hpp"
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>


namespace idx2
{

error<idx2_err_code>
ParallelDecode(const idx2_file& Idx2, const params& P, buffer* OutBuf);


/* decode the subband of a brick */
// TODO: we can detect the precision and switch to the avx2 version that uses float for better
// performance
// TODO: if a block does not decode any bit plane, no need to copy data afterwards
static expected<bool, idx2_err_code>
ParallelDecodeSubband(const idx2_file& Idx2,
                      decode_data* D,
                      decode_state Ds,
                      f64 Tolerance,      // TODO: move to decode_state
                      const grid& SbGrid, // TODO: move to decode_state
                      hash_table<i16, bitstream>* StreamsPtr,
                      brick_volume* BrickVol)       // TODO: move to decode_states
{
  u64 Brick = Ds.Brick;
  v3i SbDims3 = Dims(SbGrid);
  v3i NBlocks3 = (SbDims3 + Idx2.BlockDims3 - 1) / Idx2.BlockDims3;
  int BlockCount = Prod(NBlocks3);
  // the following subtracts blocks on subband 0 that are coded in the next level
  // meaning, we only code subband-0 blocks that are on the boundary (extrapolated)
  if (Ds.Subband == 0 && Ds.Level + 1 < Idx2.NLevels)
    BlockCount -= Prod(SbDims3 / Idx2.BlockDims3);

  /* first, read the block exponents */
  auto ReadChunkExpResult = ParallelReadChunkExponents(Idx2, D, Brick, Ds.Level, Ds.Subband);
  if (!ReadChunkExpResult)
    return Error(ReadChunkExpResult);

  chunk_exp_cache ChunkExpCache = Value(ReadChunkExpResult); // make a copy to avoid data race
  i32 BrickExpOffset = (Ds.BrickInChunk * BlockCount) * (SizeOf(Idx2.DType) > 4 ? 2 : 1);
  bitstream BrickExpsStream = ChunkExpCache.ChunkExpStream;
  SeekToByte(&BrickExpsStream, BrickExpOffset);
  u32 LastBlock = EncodeMorton3(v3<u32>(NBlocks3 - 1));
  const i8 NBitPlanes = idx2_BitSizeOf(u64);
  Clear(StreamsPtr);

  bool SubbandSignificant = false; // whether there is any significant block on this subband
  idx2_InclusiveFor (u32, Block, 0, LastBlock)
  { // zfp block loop
    v3i Z3(DecodeMorton3(Block));
    idx2_NextMorton(Block, Z3, NBlocks3);
    v3i D3 = Z3 * Idx2.BlockDims3;
    v3i BlockDims3 = Min(Idx2.BlockDims3, SbDims3 - D3);
    const int NDims = NumDims(BlockDims3);
    const int NVals = 1 << (2 * NDims);
    const int Prec = NBitPlanes - 1 - NDims;
    f64 BlockFloats[4 * 4 * 4];
    buffer_t BufFloats(BlockFloats, NVals);
    buffer_t BufInts((i64*)BlockFloats, NVals);
    u64 BlockUInts[4 * 4 * 4] = {};
    buffer_t BufUInts(BlockUInts, Prod(BlockDims3));

    bool CodedInNextLevel =
      Ds.Subband == 0 && Ds.Level + 1 < Idx2.NLevels && BlockDims3 == Idx2.BlockDims3;
    if (CodedInNextLevel)
      continue;

    // we read the exponent for the block
    i16 EMax = SizeOf(Idx2.DType) > 4
                 ? (i16)Read(&BrickExpsStream, 16) - traits<f64>::ExpBias
                 : (i16)Read(&BrickExpsStream, traits<f32>::ExpBits) - traits<f32>::ExpBias;
    i8 N = 0;
    i8 EndBitPlane = Min(i8(BitSizeOf(Idx2.DType)), NBitPlanes);
    int NBitPlanesDecoded = Exponent(Tolerance) - 6 - EMax + 1;
    i8 NBps = 0;
    int Bpc = Idx2.BitPlanesPerChunk;
    idx2_InclusiveForBackward (i8, Bp, NBitPlanes - 1, NBitPlanes - EndBitPlane)
    { // bit plane loop
      i16 RealBp = Bp + EMax;
      i16 BpKey = (RealBp + BitPlaneKeyBias_) / Bpc; // make it so that the BpKey is positive
      // TODO: always decode extra 6 bit planes?
      bool TooHighPrecision = NBitPlanes - 6 > RealBp - Exponent(Tolerance) + 1;
      if (TooHighPrecision)
      {
        if ((RealBp + BitPlaneKeyBias_) % Bpc == 0) // make sure we encode full "block" of BpKey
          break;
      }

      auto StreamIt = Lookup(*StreamsPtr, BpKey);
      bitstream* Stream = nullptr;
      if (!StreamIt)
      { // first block in the brick
        auto ReadChunkResult = ParallelReadChunk(Idx2, D, Brick, Ds.Level, Ds.Subband, BpKey);
        if (!ReadChunkResult)
          return Error(ReadChunkResult);

        chunk_cache ChunkCache = Value(ReadChunkResult); // making a copy to avoid data race
        u64* BrickPos = BinarySearch(idx2_Range(ChunkCache.Bricks), Brick);
        idx2_Assert(BrickPos != End(ChunkCache.Bricks));
        //idx2_Assert(*BrickPos == Brick);
        i64 BrickInChunk = BrickPos - Begin(ChunkCache.Bricks);
        idx2_Assert(BrickInChunk < Size(ChunkCache.BrickOffsets));
        i64 BrickOffset = ChunkCache.BrickOffsets[BrickInChunk];
        //BrickOffset += Size(ChunkCache.ChunkStream);
        Insert(&StreamIt, BpKey, ChunkCache.ChunkStream);
        Stream = StreamIt.Val;
        // seek to the correct byte offset of the brick in the chunk
        SeekToByte(Stream, BrickOffset);
      }
      else // if the stream already exists
      {
        Stream = StreamIt.Val;
      }
      /* zfp decode */
      if (!TooHighPrecision)
        ++NBps;
      auto SizeBegin = BitSize(*Stream);
      if (NBitPlanesDecoded <= 8)
        Decode(BlockUInts, NVals, Bp, N, Stream, false); // use AVX2
      else                                        // delay the transpose of bits to later
        DecodeTest(&BlockUInts[NBitPlanes - 1 - Bp], NVals, N, Stream);
      auto SizeEnd = BitSize(*Stream);
      D->BytesDecoded_ += SizeEnd - SizeBegin;
    } // end bit plane loop

    /* do inverse zfp transform but only if any bit plane is decoded */
    if (NBps > 0)
    {
      if (NBitPlanesDecoded > 8)
        TransposeRecursive(BlockUInts, NBps);

      // if the subband is not 0 or if this is the last level, we count this block
      // as significant, otherwise it is not significant
      bool CurrBlockSignificant = (Ds.Subband > 0 || Ds.Level + 1 == Idx2.NLevels);
      SubbandSignificant = SubbandSignificant || CurrBlockSignificant;
      ++D->NSignificantBlocks;
      InverseShuffle(BlockUInts, (i64*)BlockFloats, NDims);
      InverseZfp((i64*)BlockFloats, NDims);
      Dequantize(EMax, Prec, BufInts, &BufFloats);
      v3i S3;
      int J = 0;
      v3i From3 = From(SbGrid), Strd3 = Strd(SbGrid);
      timer DataTimer;
      StartTimer(&DataTimer);
      volume& Vol = BrickVol->Vol;
      idx2_Assert(Vol.Buffer);
      idx2_BeginFor3 (S3, v3i(0), BlockDims3, v3i(1))
      { // sample loop
        idx2_Assert(D3 + S3 < SbDims3);
        Vol.At<f64>(From3, Strd3, D3 + S3) = BlockFloats[J++];
      }
      idx2_EndFor3; // end sample loop
      D->DataMovementTime_ += ElapsedTime(&DataTimer);
    }
  }
  D->NInsignificantSubbands += (SubbandSignificant == false);
  // printf("%d\n", AnyBlockDecoded);

  return SubbandSignificant;
}



static expected<brick_volume, idx2_err_code>
ParallelDecodeBrick(const idx2_file& Idx2,
                    const params& P,
                    decode_data* D,
                    decode_state Ds,
                    f64 Tolerance)
{
  i8 Level = Ds.Level;
  brick_volume BrickVol;
  volume& Vol = BrickVol.Vol;
  Resize(&Vol, Idx2.BrickDimsExt3, dtype::float64, D->Alloc);
  Fill(idx2_Range(f64, Vol), 0.0); // TODO: use memset

  idx2_Assert(Size(Idx2.Subbands) <= 8);

  bool Significant = false;
  using stream_cache = hash_table<i16, bitstream>;
  idx2_RAII(stream_cache, Streams, Init(&Streams, 7), Dealloc(&Streams));
  idx2_For (i8, Sb, 0, (i8)Size(Idx2.Subbands))
  {
    if (!BitSet(Idx2.DecodeSubbandMasks[Level], Sb))
      continue;

    const subband& S = Idx2.Subbands[Sb];
    v3i SbDimsNonExt3 = idx2_NonExtDims(Dims(S.Grid));
    i8 NextLevel = Level + 1;

    /* we first copy data from the parent brick to the current brick if subband is 0 */
    if (Sb == 0 && NextLevel < Idx2.NLevels)
    {
      /* find the parent */
      v3i Brick3 = Ds.Brick3;
      v3i PBrick3 = Brick3 / Idx2.GroupBrick3;
      u64 PBrick = GetLinearBrick(Idx2, NextLevel, PBrick3);
      u64 PKey = GetBrickKey(NextLevel, PBrick);
      brick_volume& Pb = Ds.ParentBrick;
      /* copy data from the parent to the current brick for subband 0 */
      v3i LocalBrickPos3 = Brick3 % Idx2.GroupBrick3;
      grid SbGridNonExt = S.Grid;
      SetDims(&SbGridNonExt, SbDimsNonExt3);
      extent PGrid(LocalBrickPos3 * SbDimsNonExt3, SbDimsNonExt3); // parent grid
      CopyExtentGrid<f64, f64>(PGrid, Pb.Vol, SbGridNonExt, &Vol);
      // if last child, delete the parent if needed
      if (Brick3 == v3i(0)) // last child (the stack traversal goes backward)
      {
        bool DeleteBrick = true;
        if (P.OutMode == params::out_mode::HashMap)
          DeleteBrick = !Pb.Significant;
        if (DeleteBrick)
        { // TODO: cannot delete the brick because we don't know if the last child is run last
          // need a "cleanup" function to delete the brick later
          Dealloc(&Pb.Vol);
        }
      } // end last child
    } // end subband 0

    /* now we decode the subband */
    Ds.Subband = Sb;
    auto Result = ParallelDecodeSubband(Idx2, D, Ds, Tolerance, S.Grid, &Streams, &BrickVol);
    if (!Result)
      return Error(Result);
    Significant = Significant || Value(Result);
  } // end subband loop

  bool CoarsestLevel = Level + 1 == Idx2.NLevels;
  InverseCdf53(Idx2.BrickDimsExt3, Ds.Level, Idx2.Subbands, Idx2.TransformDetails, &Vol, CoarsestLevel);

  BrickVol.DoneDecoding = true;
  BrickVol.Significant = Significant;

  return BrickVol;
}


expected<brick_volume, idx2_err_code>
DecodeTask(const idx2_file& Idx2,
           const params& P,
           decode_data* D,
           decode_state Ds,
           grid OutGrid,
           f64 Tolerance,
           mmap_volume* OutVol,
           volume* OutVolMem)
{
  v3i B3 = Idx2.BrickDims3 * Pow(Idx2.GroupBrick3, Ds.Level);
  /* --------------- Decode the brick --------------- */
  auto Result = ParallelDecodeBrick(Idx2, P, D, Ds, Tolerance);
  if (!Result)
    return Error(Result);
  // Copy the samples out to the output buffer (or file)
  // The Idx2.DecodeSubbandMasks[Level - 1] == 0 means that no subbands on the next level
  // will be decoded, so we can now just copy the result out
  brick_volume& BVol = Value(Result);
  if (Ds.Level == 0 || Idx2.DecodeSubbandMasks[Ds.Level - 1] == 0)
  {
    grid BrickGrid(Ds.Brick3 * B3, Idx2.BrickDims3, v3i(1 << Ds.Level));
    grid OutBrickGrid = Crop(OutGrid, BrickGrid);
    grid BrickGridLocal = Relative(OutBrickGrid, BrickGrid);
    if (P.OutMode == params::out_mode::RegularGridFile ||
        P.OutMode == params::out_mode::RegularGridMem)
    {
      auto OutputVol =
        P.OutMode == params::out_mode::RegularGridFile ? &OutVol->Vol : OutVolMem;
      auto CopyFunc = OutputVol->Type == dtype::float32 ? (CopyGridGrid<f64, f32>)
                                                        : (CopyGridGrid<f64, f64>);
      CopyFunc(BrickGridLocal, BVol.Vol, Relative(OutBrickGrid, OutGrid), OutputVol);
      Dealloc(&BVol); // TODO: dealloc here or outside?
    }
    else if (P.OutMode == params::out_mode::HashMap)
    {
      if (!BVol.Significant)
      {
        Dealloc(&BVol);
      }
      else
      {
        u64 BrickKey = GetBrickKey(Ds.Level, Ds.Brick);
        std::unique_lock<std::mutex> Lock(D->BrickPoolMutex);
        Insert(&D->BrickPool.BrickTable, BrickKey, BVol);
      }
    }
    else if (P.OutMode == params::out_mode::NoOutput)
    {
      Dealloc(&BVol);
    }
  }
  else // not the finest level
  {
    if (P.OutMode == params::out_mode::HashMap)
    {
      if (BVol.Significant)
      {
        u64 BrickKey = GetBrickKey(Ds.Level, Ds.Brick);
        std::unique_lock<std::mutex> Lock(D->BrickPoolMutex);
        Insert(&D->BrickPool.BrickTable, BrickKey, BVol);
      }
    }
  }

  return Value(Result);
}


error<idx2_err_code>
TraverseSecondLevel(const idx2_file& Idx2,
                    const params& P,
                    decode_data* D,
                    decode_state First,
                    const extent& Extent,
                    const grid& OutGrid,
                    mmap_volume* OutVolFile,
                    volume* OutVolMem)
{
  // then decode the subtree
  f64 Tolerance = Max(Idx2.Tolerance, P.DecodeTolerance);
  int LastIndex = 0;
  idx2_RAII(array<decode_state>, BrickStack, Reserve(&BrickStack, 128), Dealloc(&BrickStack));
  PushBack(&BrickStack, First);
  expected<brick_volume, idx2_err_code> Result;
  while (Size(BrickStack) > 0)
  {
    decode_state Current = Back(BrickStack);
    //printf("level %d current " idx2_PrStrV3i "\n", Current.Level, idx2_PrV3i(Current.Brick3));
    PopBack(&BrickStack);
    i8 NextLevel = Current.Level - 1;
    // TODO: check for error
    v3i B3 = Idx2.BrickDims3 * Pow(Idx2.GroupBrick3, Current.Level);
    extent ExtentInBricks, ExtentInChunks, ExtentInFiles;
    extent VolExtentInBricks, VolExtentInChunks, VolExtentInFiles;
    extent CurrentExtent = extent(Current.Brick3 * B3, B3);
    extent CurrentExtentCrop = Crop(CurrentExtent, P.DecodeExtent);
    //printf("  extent     " idx2_PrStrExt "\n", idx2_PrExt(CurrentExtent));
    //printf("  extent crop" idx2_PrStrExt "\n", idx2_PrExt(CurrentExtentCrop));
    if (Prod<i64>(Dims(CurrentExtentCrop)) == 0)
      continue;
    Result = DecodeTask(Idx2, P, D, Current, OutGrid, Tolerance, OutVolFile, OutVolMem);
    if (!Result)
      goto EXIT;

    if (NextLevel < 0 || Idx2.DecodeSubbandMasks[NextLevel] == 0)
      continue;

    ComputeExtentsForTraversal(Idx2,
                               CurrentExtent,
                               NextLevel,
                               &ExtentInBricks,
                               &ExtentInChunks,
                               &ExtentInFiles,
                               &VolExtentInBricks,
                               &VolExtentInChunks,
                               &VolExtentInFiles);
    idx2_FileTraverse(
      idx2_ChunkTraverse(
        idx2_BrickTraverse(
          decode_state Ds;
          Ds.BrickInChunk = Top.BrickInChunk;
          Ds.Level = NextLevel;
          Ds.Brick3 = Top.BrickFrom3;
          Ds.Brick = GetLinearBrick(Idx2, NextLevel, Top.BrickFrom3);
          Ds.ParentBrick = Value(Result);
          PushBack(&BrickStack, Ds);
          ,
          64,
          Idx2.BricksOrderInChunk[NextLevel],
          ChunkTop.ChunkFrom3 * Idx2.BricksPerChunk3s[NextLevel],
          Idx2.BricksPerChunk3s[NextLevel],
          ExtentInBricks,
          VolExtentInBricks);
        ,
        64,
        Idx2.ChunksOrderInFile[NextLevel],
        FileTop.FileFrom3 * Idx2.ChunksPerFile3s[NextLevel],
        Idx2.ChunksPerFile3s[NextLevel],
        ExtentInChunks,
        VolExtentInChunks);
      , 64, Idx2.FilesOrder[NextLevel], v3i(0), Idx2.NFiles3[NextLevel], ExtentInFiles, VolExtentInFiles);


  }

  EXIT:
  {
    std::unique_lock<std::mutex> Lock(D->Mutex);
    --D->NTasks;
  }
  if (D->NTasks == 0)
    D->AllTasksDone.notify_all();

  if (!Result)
    return Error(Result);

  return idx2_Error(idx2_err_code::NoError);
}


error<idx2_err_code>
TraverseFirstLevel(const idx2_file& Idx2,
                   const params& P,
                   decode_data* D,
                   const grid& OutGrid,
                   mmap_volume* OutVolFile,
                   volume* OutVolMem)
{
  i8 Level = Idx2.NLevels - 1; // coarsest level
  extent ExtentInBricks, ExtentInChunks, ExtentInFiles;
  extent VolExtentInBricks, VolExtentInChunks, VolExtentInFiles;
  ComputeExtentsForTraversal(Idx2,
                             P.DecodeExtent,
                             Level,
                             &ExtentInBricks,
                             &ExtentInChunks,
                             &ExtentInFiles,
                             &VolExtentInBricks,
                             &VolExtentInChunks,
                             &VolExtentInFiles);
  v3i B3 = Idx2.BrickDims3 * Pow(Idx2.GroupBrick3, Level); // dimension of bricks on this level
  decode_state Ds;
  idx2_FileTraverse(
    idx2_ChunkTraverse(
      idx2_BrickTraverse(
        extent BrickExtent = extent(Top.BrickFrom3 * B3, B3);
        extent BrickExtentCrop = Crop(BrickExtent, P.DecodeExtent);
        {
          std::unique_lock<std::mutex> Lock(D->Mutex);
          ++D->NTasks;
        }
        decode_state First;
        First.BrickInChunk = Top.BrickInChunk;
        First.Level = Level;
        First.Brick3 = Top.BrickFrom3;
        First.Brick = GetLinearBrick(Idx2, Level, Top.BrickFrom3);
        D->ThreadPool.push_task([&, First, BrickExtentCrop]() {
          TraverseSecondLevel(Idx2, P, D, First, BrickExtentCrop, OutGrid, OutVolFile, OutVolMem);
        });
        //auto Task = stlab::async(stlab::default_executor,
        //  [&, First, BrickExtentCrop]()
        //  {
        //    TraverseSecondLevel(Idx2, P, D, First, BrickExtentCrop, OutGrid, OutVolFile, OutVolMem);
        //  });
        //Task.detach();
        ,
        64,
        Idx2.BricksOrderInChunk[Level],
        ChunkTop.ChunkFrom3 * Idx2.BricksPerChunk3s[Level],
        Idx2.BricksPerChunk3s[Level],
        ExtentInBricks,
        VolExtentInBricks);
      ,
      64,
      Idx2.ChunksOrderInFile[Level],
      FileTop.FileFrom3 * Idx2.ChunksPerFile3s[Level],
      Idx2.ChunksPerFile3s[Level],
      ExtentInChunks,
      VolExtentInChunks);
    , 64, Idx2.FilesOrder[Level], v3i(0), Idx2.NFiles3[Level], ExtentInFiles, VolExtentInFiles);

  return idx2_Error(idx2_err_code::NoError);
}


/* TODO: dealloc chunks after we are done with them */
error<idx2_err_code>
ParallelDecode(const idx2_file& Idx2, const params& P, buffer* OutBuf)
{
  timer DecodeTimer;
  StartTimer(&DecodeTimer);
  // TODO: we should add a --effective-mask
  grid OutGrid = GetGrid(Idx2, P.DecodeExtent);
  // printf("output grid = " idx2_PrStrGrid "\n", idx2_PrGrid(OutGrid));
  mmap_volume OutVolFile;
  volume OutVolMem;
  idx2_CleanUp(if (P.OutMode == params::out_mode::RegularGridFile) { Unmap(&OutVolFile); });

  if (P.OutMode == params::out_mode::RegularGridFile)
  {
    metadata Met;
    memcpy(Met.Name, Idx2.Name, sizeof(Met.Name));
    memcpy(Met.Field, Idx2.Field, sizeof(Met.Field));
    Met.Dims3 = Dims(OutGrid);
    Met.DType = Idx2.DType;
    //  printf("zfp decode time = %f\n", DecodeTime_);
    cstr OutFile = P.OutFile
                     ? idx2_PrintScratch("%s/%s", P.OutDir, P.OutFile)
                     : idx2_PrintScratch(
                         "%s/%s-tolerance-%f.raw", P.OutDir, ToRawFileName(Met), P.DecodeTolerance);
    //    idx2_RAII(mmap_volume, OutVol, (void)OutVol, Unmap(&OutVol));
    MapVolume(OutFile, Met.Dims3, Met.DType, &OutVolFile, map_mode::Write);
    printf("writing output volume to %s\n", OutFile);
  }
  else if (P.OutMode == params::out_mode::RegularGridMem)
  {
    OutVolMem.Buffer = *OutBuf;
    SetDims(&OutVolMem, Dims(OutGrid));
    OutVolMem.Type = Idx2.DType;
  }

  const int BrickBytes = Prod(Idx2.BrickDimsExt3) * sizeof(f64);
  // for now the allocator seems not a bottleneck
  idx2_RAII(decode_data, D, Init(&D, &Idx2, &Mallocator()));

  TraverseFirstLevel(Idx2, P, &D, OutGrid, &OutVolFile, &OutVolMem);

  std::unique_lock<std::mutex> Lock(D.Mutex);
  D.AllTasksDone.wait(Lock, [&D]{ return D.NTasks == 0; });
  //stlab::pre_exit();

  if (P.OutMode == params::out_mode::HashMap)
  {
    PrintStatistics(&D.BrickPool);
    ComputeBrickResolution(&D.BrickPool);
    WriteBricks(&D.BrickPool, "bricks");
  }

  printf("total decode time   = %f\n", Seconds(ElapsedTime(&DecodeTimer)));
  printf("io time             = %f\n", Seconds(D.DecodeIOTime_.load()));
  printf("data movement time  = %f\n", Seconds(D.DataMovementTime_.load()));
  printf("exp   bytes read    = %" PRIi64 "\n", D.BytesExps_.load());
  printf("data  bytes read    = %" PRIi64 "\n", D.BytesData_.load());
  printf("total bytes read    = %" PRIi64 "\n", D.BytesExps_.load() + D.BytesData_.load());
  printf("total bytes decoded = %" PRIi64 "\n", D.BytesDecoded_.load() / 8);
  printf("final size of brick hashmap = %" PRIi64 "\n", Size(D.BrickPool.BrickTable));
  printf("number of significant blocks = %" PRIi64 "\n", D.NSignificantBlocks.load());
  printf("number of insignificant subbands = %" PRIi64 "\n", D.NInsignificantSubbands.load());

  return idx2_Error(err_code::NoError);
}


} // namespace idx2

