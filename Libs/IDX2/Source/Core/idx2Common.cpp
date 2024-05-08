#include "idx2Common.h"
#include "InputOutput.h"
#include "Math.h"


#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wsign-compare"
#if defined(__clang__)
#pragma GCC diagnostic ignored "-Wnested-anon-types"
#endif
#endif
#define SEXPR_IMPLEMENTATION
#include "sexpr.h"
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#endif
#include "zstd/zstd.c"
#include "zstd/zstd.h"
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

namespace idx2
{

free_list_allocator BrickAlloc_;


void
Dealloc(params* P)
{
}


void
SetName(idx2_file* Idx2, cstr Name)
{
  snprintf(Idx2->Name, sizeof(Idx2->Name), "%s", Name);
}


void
SetField(idx2_file* Idx2, cstr Field)
{
  snprintf(Idx2->Field, sizeof(Idx2->Field), "%s", Field);
}


void
SetVersion(idx2_file* Idx2, const v2i& Ver)
{
  Idx2->Version = Ver;
}


void
SetDimensions(idx2_file* Idx2, const v3i& Dims3)
{
  Idx2->Dims3 = Dims3;
}


void
SetDataType(idx2_file* Idx2, dtype DType)
{
  Idx2->DType = DType;
}


void
SetBrickSize(idx2_file* Idx2, const v3i& BrickDims3)
{
  Idx2->BrickDims3 = BrickDims3;
}


void
SetNumLevels(idx2_file* Idx2, i8 NLevels)
{
  Idx2->NLevels = NLevels;
}


void
SetTolerance(idx2_file* Idx2, f64 Tolerance)
{
  Idx2->Tolerance = Tolerance;
}


void
SetChunksPerFile(idx2_file* Idx2, int ChunksPerFile)
{
  Idx2->ChunksPerFileIn = ChunksPerFile;
}


void
SetBricksPerChunk(idx2_file* Idx2, int BricksPerChunk)
{
  Idx2->BricksPerChunkIn = BricksPerChunk;
}


void
SetFilesPerDirectory(idx2_file* Idx2, int FilesPerDir)
{
  Idx2->FilesPerDir = FilesPerDir;
}


void
SetBitPlanesPerChunk(idx2_file* Idx2, int BitPlanesPerChunk)
{
  Idx2->BitPlanesPerChunk = BitPlanesPerChunk;
}


void
SetBitPlanesPerFile(idx2_file* Idx2, int BitPlanesPerFile)
{
  Idx2->BitPlanesPerFile = BitPlanesPerFile;
}


void
SetDir(idx2_file* Idx2, stref Dir)
{
  Idx2->Dir = Dir;
}


void
SetDownsamplingFactor(idx2_file* Idx2, const v3i& DownsamplingFactor3)
{
  Idx2->DownsamplingFactor3 = DownsamplingFactor3;
}


/* Write the metadata file (idx) */
// TODO: return error type
void
WriteMetaFile(const idx2_file& Idx2, const params& P, cstr FileName)
{
  FILE* Fp = fopen(FileName, "w");
  fprintf(Fp, "(\n"); // begin (
  fprintf(Fp, "  (common\n");
  fprintf(Fp, "    (type \"Simulation\")\n"); // TODO: add this config to Idx2
  fprintf(Fp, "    (name \"%s\")\n", P.Meta.Name);
  fprintf(Fp, "    (field \"%s\")\n", P.Meta.Field);
  fprintf(Fp, "    (dimensions %d %d %d)\n", idx2_PrV3i(Idx2.Dims3));
  stref DType = ToString(Idx2.DType);
  fprintf(Fp, "    (data-type \"%s\")\n", idx2_PrintScratchN(Size(DType), "%s", DType.ConstPtr));
  fprintf(Fp, "    (min-max %.20f %.20f)\n", Idx2.ValueRange.Min, Idx2.ValueRange.Max);
  fprintf(Fp, "    (accuracy %.20f)\n", Idx2.Tolerance);
  fprintf(Fp, "  )\n"); // end common)
  fprintf(Fp, "  (format\n");
  fprintf(Fp, "    (version %d %d)\n", Idx2.Version[0], Idx2.Version[1]);
  fprintf(Fp, "    (brick-size %d %d %d)\n", idx2_PrV3i(Idx2.BrickDims3));
  char TransformOrder[128];
  DecodeTransformOrder(Idx2.TransformOrder, TransformOrder);
  fprintf(Fp, "    (transform-order \"%s\")\n", TransformOrder);
  fprintf(Fp, "    (num-levels %d)\n", Idx2.NLevels);
  fprintf(Fp, "    (transform-passes-per-levels %d)\n", Idx2.NTformPasses);
  fprintf(Fp, "    (bricks-per-chunk %d)\n", Idx2.BricksPerChunkIn);
  fprintf(Fp, "    (chunks-per-file %d)\n", Idx2.ChunksPerFileIn);
  fprintf(Fp, "    (files-per-directory %d)\n", Idx2.FilesPerDir);
  fprintf(Fp, "    (bit-planes-per-chunk %d)\n", Idx2.BitPlanesPerChunk);
  fprintf(Fp, "  )\n"); // end format)
  fprintf(Fp, ")\n");   // end )
  fclose(Fp);
}


error<idx2_err_code>
ReadMetaFileFromBuffer(idx2_file* Idx2, buffer& Buf)
{
  SExprResult Result = ParseSExpr((cstr)Buf.Data, Size(Buf), nullptr);
  if (Result.type == SE_SYNTAX_ERROR)
  {
    fprintf(stderr, "Error(%d): %s.\n", Result.syntaxError.lineNumber, Result.syntaxError.message);
    return idx2_Error(idx2_err_code::SyntaxError);
  }
  else
  {
    SExpr* Data = (SExpr*)malloc(sizeof(SExpr) * Result.count);
    idx2_CleanUp(free(Data));
    array<SExpr*> Stack;
    Reserve(&Stack, Result.count);
    idx2_CleanUp(Dealloc(&Stack));
    // This time we supply the pool
    SExprPool Pool = { Result.count, Data };
    Result = ParseSExpr((cstr)Buf.Data, Size(Buf), &Pool);
    // result.expr contains the successfully parsed SExpr
    //    printf("parse .idx2 file successfully\n");
    PushBack(&Stack, Result.expr);
    bool GotId = false;
    SExpr* LastExpr = nullptr;
    while (Size(Stack) > 0)
    {
      SExpr* Expr = Back(Stack);
      PopBack(&Stack);
      if (Expr->next)
        PushBack(&Stack, Expr->next);
      if (GotId)
      {
        if (SExprStringEqual((cstr)Buf.Data, &(LastExpr->s), "version"))
        {
          idx2_Assert(Expr->type == SE_INT);
          Idx2->Version[0] = Expr->i;
          idx2_Assert(Expr->next);
          Expr = Expr->next;
          idx2_Assert(Expr->type == SE_INT);
          Idx2->Version[1] = Expr->i;
          //          printf("Version = %d.%d\n", Idx2->Version[0], Idx2->Version[1]);
        }
        else if (SExprStringEqual((cstr)Buf.Data, &(LastExpr->s), "name"))
        {
          idx2_Assert(Expr->type == SE_STRING);
          memcpy(Idx2->Name, Buf.Data + Expr->s.start, Expr->s.len);
          Idx2->Name[Expr->s.len] = 0;
          //          printf("Name = %s\n", Idx2->Name);
        }
        else if (SExprStringEqual((cstr)Buf.Data, &(LastExpr->s), "field"))
        {
          idx2_Assert(Expr->type == SE_STRING);
          memcpy(Idx2->Field, Buf.Data + Expr->s.start, Expr->s.len);
          Idx2->Field[Expr->s.len] = 0;
          //          printf("Field = %s\n", Idx2->Field);
        }
        else if (SExprStringEqual((cstr)Buf.Data, &(LastExpr->s), "dimensions"))
        {
          idx2_Assert(Expr->type == SE_INT);
          Idx2->Dims3.X = Expr->i;
          idx2_Assert(Expr->next);
          Expr = Expr->next;
          idx2_Assert(Expr->type == SE_INT);
          Idx2->Dims3.Y = Expr->i;
          idx2_Assert(Expr->next);
          Expr = Expr->next;
          idx2_Assert(Expr->type == SE_INT);
          Idx2->Dims3.Z = Expr->i;
          //          printf("Dims = %d %d %d\n", idx2_PrV3i(Idx2->Dims3));
        }
        if (SExprStringEqual((cstr)Buf.Data, &(LastExpr->s), "accuracy"))
        {
          idx2_Assert(Expr->type == SE_FLOAT);
          Idx2->Tolerance = Expr->f;
          //          printf("Accuracy = %.17g\n", Idx2->Accuracy);
        }
        else if (SExprStringEqual((cstr)Buf.Data, &(LastExpr->s), "data-type"))
        {
          idx2_Assert(Expr->type == SE_STRING);
          Idx2->DType = StringTo<dtype>()(stref((cstr)Buf.Data + Expr->s.start, Expr->s.len));
          //          printf("Data type = %.*s\n", ToString(Idx2->DType).Size,
          //          ToString(Idx2->DType).ConstPtr);
        }
        else if (SExprStringEqual((cstr)Buf.Data, &(LastExpr->s), "min-max"))
        {
          idx2_Assert(Expr->type == SE_FLOAT || Expr->type == SE_INT);
          Idx2->ValueRange.Min = Expr->i;
          idx2_Assert(Expr->next);
          Expr = Expr->next;
          idx2_Assert(Expr->type == SE_FLOAT || Expr->type == SE_INT);
          Idx2->ValueRange.Max = Expr->i;
        }
        else if (SExprStringEqual((cstr)Buf.Data, &(LastExpr->s), "brick-size"))
        {
          v3i BrickDims3(0);
          idx2_Assert(Expr->type == SE_INT);
          BrickDims3.X = Expr->i;
          idx2_Assert(Expr->next);
          Expr = Expr->next;
          idx2_Assert(Expr->type == SE_INT);
          BrickDims3.Y = Expr->i;
          idx2_Assert(Expr->next);
          Expr = Expr->next;
          idx2_Assert(Expr->type == SE_INT);
          BrickDims3.Z = Expr->i;
          SetBrickSize(Idx2, BrickDims3);
          //          printf("Brick size %d %d %d\n", idx2_PrV3i(Idx2->BrickDims3));
        }
        else if (SExprStringEqual((cstr)Buf.Data, &(LastExpr->s), "transform-order"))
        {
          idx2_Assert(Expr->type == SE_STRING);
          Idx2->TransformOrder =
            EncodeTransformOrder(stref((cstr)Buf.Data + Expr->s.start, Expr->s.len));
          char TransformOrder[128];
          DecodeTransformOrder(Idx2->TransformOrder, TransformOrder);
          //          printf("Transform order = %s\n", TransformOrder);
        }
        else if (SExprStringEqual((cstr)Buf.Data, &(LastExpr->s), "num-levels"))
        {
          idx2_Assert(Expr->type == SE_INT);
          Idx2->NLevels = i8(Expr->i);
          //          printf("Num levels = %d\n", Idx2->NLevels);
        }
        else if (SExprStringEqual((cstr)Buf.Data, &(LastExpr->s), "bricks-per-chunk"))
        {
          idx2_Assert(Expr->type == SE_INT);
          Idx2->BricksPerChunkIn = Expr->i;
          //          printf("Bricks per chunk = %d\n", Idx2->BricksPerChunks[0]);
        }
        else if (SExprStringEqual((cstr)Buf.Data, &(LastExpr->s), "chunks-per-file"))
        {
          idx2_Assert(Expr->type == SE_INT);
          Idx2->ChunksPerFileIn = Expr->i;
          //          printf("Chunks per file = %d\n", Idx2->ChunksPerFiles[0]);
        }
        else if (SExprStringEqual((cstr)Buf.Data, &(LastExpr->s), "files-per-directory"))
        {
          idx2_Assert(Expr->type == SE_INT);
          Idx2->FilesPerDir = Expr->i;
          //          printf("Files per directory = %d\n", Idx2->FilesPerDir);
        }
        else if (SExprStringEqual((cstr)Buf.Data, &(LastExpr->s), "bit-planes-per-chunk"))
        {
          idx2_Assert(Expr->type == SE_INT);
          Idx2->BitPlanesPerChunk = Expr->i;
        }
      }
      if (Expr->type == SE_ID)
      {
        LastExpr = Expr;
        GotId = true;
      }
      else if (Expr->type == SE_LIST)
      {
        PushBack(&Stack, Expr->head);
        GotId = false;
      }
      else
      {
        GotId = false;
      }
    }
  }
  return idx2_Error(idx2_err_code::NoError);
}

error<idx2_err_code>
ReadMetaFile(idx2_file* Idx2, cstr FileName)
{
  buffer Buf;
  idx2_CleanUp(DeallocBuf(&Buf));
  idx2_PropagateIfError(ReadFile(FileName, &Buf));
  return ReadMetaFileFromBuffer(Idx2, Buf);
}


static error<idx2_err_code>
CheckBrickSize(idx2_file* Idx2, const params& P)
{
  if (!(IsPow2(Idx2->BrickDims3.X) && IsPow2(Idx2->BrickDims3.Y) && IsPow2(Idx2->BrickDims3.Z)))
    return idx2_Error(
      idx2_err_code::BrickSizeNotPowerOfTwo, idx2_PrStrV3i "\n", idx2_PrV3i(Idx2->BrickDims3));
  if (!(Idx2->Dims3 >= Idx2->BrickDims3))
    return idx2_Error(idx2_err_code::BrickSizeTooBig,
                      " total dims: " idx2_PrStrV3i ", brick dims: " idx2_PrStrV3i "\n",
                      idx2_PrV3i(Idx2->Dims3),
                      idx2_PrV3i(Idx2->BrickDims3));
  return idx2_Error(idx2_err_code::NoError);
}


static void
ComputeTransformOrder(idx2_file* Idx2, const params& P, char* TformOrder)
{ /* try to repeat XYZ+, depending on the BrickDims */
  int J = 0;
  idx2_For (int, D, 0, 3)
  {
    if (Idx2->BrickDims3[D] > 1)
      TformOrder[J++] = char('X' + D);
  }
  TformOrder[J++] = '+';
  TformOrder[J++] = '+';
  Idx2->TransformOrder = EncodeTransformOrder(TformOrder);
  Idx2->TransformOrderFull.Len =
    DecodeTransformOrder(Idx2->TransformOrder, Idx2->NTformPasses, Idx2->TransformOrderFull.Data);
}

const char* bit_rep[16] = { "0000", "0001", "0010", "0011",
                            "0100", "0101", "0110", "0111",
                            "1000", "1001", "1010", "1011",
                            "1100", "1101", "1110", "1111" };

void
print_byte(uint8_t byte)
{
  printf("%s%s", bit_rep[byte >> 4], bit_rep[byte & 0x0F]);
}


/* Build the subbands, including which subbands to decode, depending on P.DownsamplingFactor3*/
static void
BuildSubbands(idx2_file* Idx2, const params& P)
{
  Idx2->BrickDimsExt3 = idx2_ExtDims(Idx2->BrickDims3);
  BuildSubbands(Idx2->BrickDimsExt3, Idx2->NTformPasses, Idx2->TransformOrder, &Idx2->Subbands);
  BuildSubbands(Idx2->BrickDims3, Idx2->NTformPasses, Idx2->TransformOrder, &Idx2->SubbandsNonExt);


  /* compute the list of subbands to decode given the downsampling factor */
  v3i Spacing3 = v3i(1) << P.DownsamplingFactor3;
  //printf("target spacing " idx2_PrStrV3i "\n", idx2_PrV3i(Spacing3));
  array<array<v3i>> SubbandFroms3;
  array<array<v3i>> SubbandSpacings3;
  Resize(&SubbandFroms3, Idx2->NLevels);
  Resize(&SubbandSpacings3, Idx2->NLevels);
  idx2_For (int, I, 0, Idx2->NLevels)
  {
    Resize(&SubbandSpacings3[I], Size(Idx2->Subbands));
    Resize(&SubbandFroms3[I], Size(Idx2->Subbands));
    u8 Mask = 0xFF;
    //printf("level %d\n", I);
    Resize(&Idx2->DecodeSubbandSpacings[I], Size(Idx2->Subbands));
    idx2_For (int, Sb, 0, Size(SubbandSpacings3[I]))
    {
      v3i F3 = SubbandFroms3[I][Sb] = From(Idx2->Subbands[Sb].Grid) * (I > 0 ? SubbandSpacings3[I - 1][0] : v3i(1));
      v3i S3 = SubbandSpacings3[I][Sb] = Strd(Idx2->Subbands[Sb].Grid) * (I > 0 ? SubbandSpacings3[I - 1][0] : v3i(1));
      //printf("subband %d from " idx2_PrStrV3i "\n", Sb, idx2_PrV3i(F3));
      //printf("subband %d spacing " idx2_PrStrV3i "\n", Sb, idx2_PrV3i(S3));
      Idx2->DecodeSubbandSpacings[I][Sb] = v3i(1);
      for (int D = 0; D < 3; ++D)
      {
        // check if a grid1 that starts at F3 with spacing S3 is a subgrid
        // of a grid2 that starts at 0 with spacing Spacing3
        if ((F3[D] % Spacing3[D]) == 0)
        {
          if ((S3[D] % Spacing3[D]) != 0)
            Idx2->DecodeSubbandSpacings[I][Sb][D] = Spacing3[D] / S3[D];
        }
        else // skip the subband
        {
          Mask = UnsetBit(Mask, Sb);
        }
      }
      //printf("subband decode spacing = " idx2_PrStrV3i "\n", idx2_PrV3i(Idx2->DecodeSubbandSpacings[I][Sb]));
    }
    if (I + 1 < Idx2->NLevels && Mask == 1)
      Mask = 0; // explicitly disable subband 0 if it is the only subband to decode
    Idx2->DecodeSubbandMasks[I] = Mask;
    //print_byte(Mask);
    //printf("\n");
  }

  idx2_ForEach (Elem, SubbandSpacings3)
    Dealloc(Elem);
  Dealloc(&SubbandSpacings3);
  idx2_ForEach (Elem, SubbandFroms3)
    Dealloc(Elem);
  Dealloc(&SubbandFroms3);
}


static void
ComputeNumBricksPerLevel(idx2_file* Idx2, const params& P)
{
  Idx2->GroupBrick3 = Idx2->BrickDims3 / Dims(Idx2->SubbandsNonExt[0].Grid);
  v3i NBricks3 = (Idx2->Dims3 + Idx2->BrickDims3 - 1) / Idx2->BrickDims3;
  v3i NBricksPerLevel3 = NBricks3;
  idx2_For (int, I, 0, Idx2->NLevels)
  {
    Idx2->NBricks3[I] = NBricksPerLevel3;
    NBricksPerLevel3 = (NBricksPerLevel3 + Idx2->GroupBrick3 - 1) / Idx2->GroupBrick3;
  }
}


/* compute the brick order, by repeating the (per brick) transform order */
static error<idx2_err_code>
ComputeGlobalBricksOrder(idx2_file* Idx2, const params& P, const char* TformOrder)
{
  Resize(&Idx2->BricksOrderStr, Idx2->NLevels);
  idx2_For (int, I, 0, Idx2->NLevels)
  {
    v3i N3 = Idx2->NBricks3[I];
    v3i LogN3 = v3i(Log2Ceil(N3.X), Log2Ceil(N3.Y), Log2Ceil(N3.Z));
    int MinLogN3 = Min(LogN3.X, LogN3.Y, LogN3.Z);
    v3i LeftOver3 =
      LogN3 -
      v3i(Idx2->BrickDims3.X > 1, Idx2->BrickDims3.Y > 1, Idx2->BrickDims3.Z > 1) * MinLogN3;
    char BrickOrder[128];
    int J = 0;
    idx2_For (int, D, 0, 3)
    {
      if (Idx2->BrickDims3[D] == 1)
      {
        while (LeftOver3[D]-- > 0)
          BrickOrder[J++] = char('X' + D);
      }
    }
    while (!(LeftOver3 <= 0))
    {
      idx2_For (int, D, 0, 3)
      {
        if (LeftOver3[D]-- > 0)
          BrickOrder[J++] = char('X' + D);
      }
    }
    if (J > 0)
      BrickOrder[J++] = '+';
    idx2_For (size_t, K, 0, sizeof(TformOrder))
      BrickOrder[J++] = TformOrder[K];
    Idx2->BricksOrder[I] = EncodeTransformOrder(BrickOrder);
    Idx2->BricksOrderStr[I].Len =
      DecodeTransformOrder(Idx2->BricksOrder[I], N3, Idx2->BricksOrderStr[I].Data);

    if (Idx2->BricksOrderStr[I].Len < Idx2->TransformOrderFull.Len)
      return idx2_Error(idx2_err_code::TooManyLevels);
  }

  return idx2_Error(idx2_err_code::NoError);
}


static error<idx2_err_code>
ComputeLocalBricksChunksFilesOrders(idx2_file* Idx2, const params& P)
{
  Idx2->BricksPerChunk[0] = Idx2->BricksPerChunkIn;
  Idx2->ChunksPerFile[0] = Idx2->ChunksPerFileIn;

  if (!(Idx2->BricksPerChunk[0] <= idx2_file::MaxBricksPerChunk))
    return idx2_Error(idx2_err_code::TooManyBricksPerChunk);
  if (!IsPow2(Idx2->BricksPerChunk[0]))
    return idx2_Error(idx2_err_code::BricksPerChunkNotPowerOf2);
  if (!(Idx2->ChunksPerFile[0] <= idx2_file::MaxChunksPerFile))
    return idx2_Error(idx2_err_code::TooManyChunksPerFile);
  if (!IsPow2(Idx2->ChunksPerFile[0]))
    return idx2_Error(idx2_err_code::ChunksPerFileNotPowerOf2);

  idx2_For (int, I, 0, Idx2->NLevels)
  {
    stack_string<64> BricksOrderInChunk;
    stack_string<64> ChunksOrderInFile;
    stack_string<64> FilesOrder;

    /* bricks order in chunk */
    {
      Idx2->BricksPerChunk[I] =
        1 << Min((u8)Log2Ceil(Idx2->BricksPerChunk[0]), Idx2->BricksOrderStr[I].Len);
      BricksOrderInChunk.Len = Log2Ceil(Idx2->BricksPerChunk[I]);
      Idx2->BricksPerChunk3s[I] = v3i(1);
      idx2_For (int, J, 0, BricksOrderInChunk.Len)
      {
        char C = Idx2->BricksOrderStr[I][Idx2->BricksOrderStr[I].Len - J - 1];
        Idx2->BricksPerChunk3s[I][C - 'X'] *= 2;
        BricksOrderInChunk[BricksOrderInChunk.Len - J - 1] = C;
      }
      Idx2->BricksOrderInChunk[I] =
        EncodeTransformOrder(stref(BricksOrderInChunk.Data, BricksOrderInChunk.Len));
      idx2_Assert(Idx2->BricksPerChunk[I] = Prod(Idx2->BricksPerChunk3s[I]));
    }

    /* chunks order in file */
    {
      Idx2->NChunks3[I] =
        (Idx2->NBricks3[I] + Idx2->BricksPerChunk3s[I] - 1) / Idx2->BricksPerChunk3s[I];
      Idx2->ChunksPerFile[I] = 1 << Min((u8)Log2Ceil(Idx2->ChunksPerFile[0]),
                                         (u8)(Idx2->BricksOrderStr[I].Len - BricksOrderInChunk.Len));
      idx2_Assert(Idx2->BricksOrderStr[I].Len >= BricksOrderInChunk.Len);
      ChunksOrderInFile.Len = Log2Ceil(Idx2->ChunksPerFile[I]);
      Idx2->ChunksPerFile3s[I] = v3i(1);
      idx2_For (int, J, 0, ChunksOrderInFile.Len)
      {
        char C = Idx2->BricksOrderStr[I][Idx2->BricksOrderStr[I].Len - BricksOrderInChunk.Len - J - 1];
        Idx2->ChunksPerFile3s[I][C - 'X'] *= 2;
        ChunksOrderInFile[ChunksOrderInFile.Len - J - 1] = C;
      }
      Idx2->ChunksOrderInFile[I] =
        EncodeTransformOrder(stref(ChunksOrderInFile.Data, ChunksOrderInFile.Len));
      idx2_Assert(Idx2->ChunksPerFile[I] == Prod(Idx2->ChunksPerFile3s[I]));
      Idx2->NFiles3[I] =
        (Idx2->NChunks3[I] + Idx2->ChunksPerFile3s[I] - 1) / Idx2->ChunksPerFile3s[I];
    }

    /* global chunk orders (not being used for now) */
    {
      stack_string<64> ChunksOrder;
      int ChunksPerVol = 1 << (Idx2->BricksOrderStr[I].Len - BricksOrderInChunk.Len);
      idx2_Assert(Idx2->BricksOrderStr[I].Len >= BricksOrderInChunk.Len);
      ChunksOrder.Len = Log2Ceil(ChunksPerVol);
      idx2_For (int, J, 0, ChunksOrder.Len)
      {
        char C = Idx2->BricksOrderStr[I][Idx2->BricksOrderStr[I].Len - BricksOrderInChunk.Len - J - 1];
        ChunksOrder[ChunksOrder.Len - J - 1] = C;
      }
      Idx2->ChunksOrder[I] = EncodeTransformOrder(stref(ChunksOrder.Data, ChunksOrder.Len));
      Resize(&Idx2->ChunksOrderStr, Idx2->NLevels);
      Idx2->ChunksOrderStr[I].Len = DecodeTransformOrder(
        Idx2->ChunksOrder[I], Idx2->NChunks3[I], Idx2->ChunksOrderStr[I].Data);
    }

    /* files order */
    {
      int FilesPerVol =
        1 << (Idx2->BricksOrderStr[I].Len - BricksOrderInChunk.Len - ChunksOrderInFile.Len);
      // TODO: the following check may fail if the brick size is too close to the size of the
      // volume, and we set NLevels too high
      idx2_Assert(Idx2->BricksOrderStr[I].Len >= BricksOrderInChunk.Len + ChunksOrderInFile.Len);
      FilesOrder.Len = Log2Ceil(FilesPerVol);
      idx2_For (int, J, 0, FilesOrder.Len)
      {
        char C = Idx2->BricksOrderStr[I][Idx2->BricksOrderStr[I].Len - BricksOrderInChunk.Len -
                                         ChunksOrderInFile.Len - J - 1];
        FilesOrder[FilesOrder.Len - J - 1] = C;
      }
      Idx2->FilesOrder[I] = EncodeTransformOrder(stref(FilesOrder.Data, FilesOrder.Len));
      Resize(&Idx2->FilesOrderStr, Idx2->NLevels);
      Idx2->FilesOrderStr[I].Len =
        DecodeTransformOrder(Idx2->FilesOrder[I], Idx2->NFiles3[I], Idx2->FilesOrderStr[I].Data);
    }
  }

  return idx2_Error(idx2_err_code::NoError);
}


static error<idx2_err_code>
ComputeFileDirDepths(idx2_file* Idx2, const params& P)
{
  if (!(Idx2->FilesPerDir <= idx2_file::MaxFilesPerDir))
    return idx2_Error(idx2_err_code::TooManyFilesPerDir, "%d", Idx2->FilesPerDir);

  idx2_For (int, I, 0, Idx2->NLevels)
  {
    Idx2->BricksPerFile[I] = Idx2->BricksPerChunk[I] * Idx2->ChunksPerFile[I];
    Idx2->FilesDirsDepth[I].Len = 0;
    i8 DepthAccum = Idx2->FilesDirsDepth[I][Idx2->FilesDirsDepth[I].Len++] =
      Log2Ceil(Idx2->BricksPerFile[I]);
    i8 Len = Idx2->BricksOrderStr[I].Len /* - Idx2->TformOrderFull.Len*/;
    while (DepthAccum < Len)
    {
      i8 Inc = Min(i8(Len - DepthAccum), Log2Ceil(Idx2->FilesPerDir));
      DepthAccum += (Idx2->FilesDirsDepth[I][Idx2->FilesDirsDepth[I].Len++] = Inc);
    }
    if (Idx2->FilesDirsDepth[I].Len > idx2_file::MaxSpatialDepth)
      return idx2_Error(idx2_err_code::TooManyFilesPerDir);
    Reverse(Begin(Idx2->FilesDirsDepth[I]),
            Begin(Idx2->FilesDirsDepth[I]) + Idx2->FilesDirsDepth[I].Len);
  }

  return idx2_Error(idx2_err_code::NoError);
}


/* compute the transform details, for both the normal transform and for extrapolation */
static void
ComputeWaveletTransformDetails(idx2_file* Idx2)
{
  ComputeTransformDetails(&Idx2->TransformDetails, Idx2->BrickDimsExt3, Idx2->NTformPasses, Idx2->TransformOrder);
  int NLevels = Log2Floor(Max(Max(Idx2->BrickDims3.X, Idx2->BrickDims3.Y), Idx2->BrickDims3.Z));
  ComputeTransformDetails(&Idx2->TransformDetailsExtrapolate, Idx2->BrickDims3, NLevels, Idx2->TransformOrder);
}


/* TODO NEXT: this function needs revision */
static void
GuessNumLevelsIfNeeded(idx2_file* Idx2)
{
  if (Idx2->NLevels == 0)
  {
    v3i BrickDims3 = Idx2->BrickDims3;
    while (BrickDims3 <= Idx2->Dims3)
    {
      BrickDims3.X = BrickDims3.X == 1 ? BrickDims3.X : BrickDims3.X * 2;
      BrickDims3.Y = BrickDims3.Y == 1 ? BrickDims3.Y : BrickDims3.Y * 2;
      BrickDims3.Z = BrickDims3.Z == 1 ? BrickDims3.Z : BrickDims3.Z * 2;
      if (BrickDims3 <= Idx2->Dims3)
        ++Idx2->NLevels;
    }
  }
}


void
ComputeExtentsForTraversal(const idx2_file& Idx2,
                           const extent& Ext,
                           i8 Level,
                           extent* ExtentInBricks,
                           extent* ExtentInChunks,
                           extent* ExtentInFiles,
                           extent* VolExtentInBricks,
                           extent* VolExtentInChunks,
                           extent* VolExtentInFiles)
{
  //extent Ext = Extent;                  // this is in unit of samples
  v3i B3, Bf3, Bl3, C3, Cf3, Cl3, F3, Ff3, Fl3; // Brick dimensions, brick first, brick last
  B3 = Idx2.BrickDims3 * Pow(Idx2.GroupBrick3, Level);
  C3 = Idx2.BricksPerChunk3s[Level] * B3;
  F3 = C3 * Idx2.ChunksPerFile3s[Level];

  Bf3 = From(Ext) / B3;
  Bl3 = Last(Ext) / B3;
  Cf3 = From(Ext) / C3;
  Cl3 = Last(Ext) / C3;
  Ff3 = From(Ext) / F3;
  Fl3 = Last(Ext) / F3;

  *ExtentInBricks = extent(Bf3, Bl3 - Bf3 + 1);
  *ExtentInChunks = extent(Cf3, Cl3 - Cf3 + 1);
  *ExtentInFiles  = extent(Ff3, Fl3 - Ff3 + 1);

  extent VolExt(Idx2.Dims3);
  v3i Vbf3, Vbl3, Vcf3, Vcl3, Vff3, Vfl3; // VolBrickFirst, VolBrickLast
  Vbf3 = From(VolExt) / B3;
  Vbl3 = Last(VolExt) / B3;
  Vcf3 = From(VolExt) / C3;
  Vcl3 = Last(VolExt) / C3;
  Vff3 = From(VolExt) / F3;
  Vfl3 = Last(VolExt) / F3;

  *VolExtentInBricks = extent(Vbf3, Vbl3 - Vbf3 + 1);
  *VolExtentInChunks = extent(Vcf3, Vcl3 - Vcf3 + 1);
  *VolExtentInFiles  = extent(Vff3, Vfl3 - Vff3 + 1);
}

error<idx2_err_code>
Finalize(idx2_file* Idx2, params* P)
{
  idx2_PropagateIfError(CheckBrickSize(Idx2, *P));
  P->Tolerance = Max(fabs(P->Tolerance), Idx2->Tolerance);
  //printf("tolerance = %.16f\n", P->Tolerance);
  GuessNumLevelsIfNeeded(Idx2);
  if (!(Idx2->NLevels <= idx2_file::MaxLevels))
    return idx2_Error(idx2_err_code::TooManyLevels, "Max # of levels = %d\n", Idx2->MaxLevels);
  if ((Idx2->BitPlanesPerChunk > Idx2->BitPlanesPerFile) ||
      (Idx2->BitPlanesPerFile % Idx2->BitPlanesPerChunk) != 0)
    return idx2_Error(idx2_err_code::SizeMismatched, "BitPlanesPerFile not multiple of BitPlanesPerChunk\n");


  char TformOrder[8] = {};
  ComputeTransformOrder(Idx2, *P, TformOrder);

  BuildSubbands(Idx2, *P);

  ComputeNumBricksPerLevel(Idx2, *P);

  idx2_PropagateIfError(ComputeGlobalBricksOrder(Idx2, *P, TformOrder));
  idx2_PropagateIfError(ComputeLocalBricksChunksFilesOrders(Idx2, *P));

  idx2_PropagateIfError(ComputeFileDirDepths(Idx2, *P));

  ComputeWaveletTransformDetails(Idx2);

  return idx2_Error(idx2_err_code::NoError);
}


void
Dealloc(idx2_file* Idx2)
{
  Dealloc(&Idx2->BricksOrderStr);
  Dealloc(&Idx2->ChunksOrderStr);
  Dealloc(&Idx2->FilesOrderStr);
  Dealloc(&Idx2->Subbands);
  Dealloc(&Idx2->SubbandsNonExt);
  idx2_ForEach (Elem ,Idx2->DecodeSubbandSpacings)
    Dealloc(Elem);
}


// TODO: handle the case where the query extent is larger than the domain itself
grid
GetGrid(const idx2_file& Idx2, const extent& Ext)
{
  auto CroppedExt = Crop(Ext, extent(Idx2.Dims3));
  v3i Strd3(1); // start with stride (1, 1, 1)
  idx2_For (int, D, 0, 3)
    Strd3[D] <<= Idx2.DownsamplingFactor3[D];

  v3i First3 = From(CroppedExt);
  v3i Last3 = Last(CroppedExt);
  Last3 = ((Last3 + Strd3 - 1) / Strd3) * Strd3; // move last to the right
  First3 = (First3 / Strd3) * Strd3; // move first to the left

  return grid(First3, (Last3 - First3) / Strd3 + 1, Strd3);
}


static void
ComputeExtentsForTraversal(const idx2_file& Idx2,
                           const extent& Ext,
                           i8 Level,
                           extent* ExtentInFiles,
                           extent* VolExtentInFiles)
{
  v3i B3, C3, F3, Ff3, Fl3;
  B3 = Idx2.BrickDims3 * Pow(Idx2.GroupBrick3, Level);
  C3 = Idx2.BricksPerChunk3s[Level] * B3;
  F3 = C3 * Idx2.ChunksPerFile3s[Level];

  Ff3 = From(Ext) / F3;
  Fl3 = Last(Ext) / F3;
  *ExtentInFiles  = extent(Ff3, Fl3 - Ff3 + 1);

  extent VolExt(Idx2.Dims3);
  v3i Vff3 = From(VolExt) / F3;
  v3i Vfl3 = Last(VolExt) / F3;
  *VolExtentInFiles  = extent(Vff3, Vfl3 - Vff3 + 1);
}

using traverse_callback = error<idx2_err_code>(const traverse_item&);

error<idx2_err_code>
TraverseHierarchy(u64 TraverseOrder,
                  const v3i& From3,
                  const v3i& Dims3,
                  const extent& Extent, // in units of traverse_item
                  const extent& VolExtent, // in units of traverse_item
                  traverse_callback Callback)
{
  idx2_RAII(array<traverse_item>, Stack, Reserve(&Stack, 64), Dealloc(&Stack));
  v3i Dims3Ext((int)NextPow2(Dims3.X), (int)NextPow2(Dims3.Y), (int)NextPow2(Dims3.Z));
  traverse_item Top;
  Top.From3 = From3;
  Top.To3 = From3 + Dims3Ext;
  Top.TraverseOrder = Top.PrevTraverseOrder = TraverseOrder;
  PushBack(&Stack, Top);
  while (Size(Stack) >= 0)
  {
    Top = Back(Stack);
    int FD = Top.TraverseOrder & 0x3;
    Top.TraverseOrder >>= 2;
    if (FD == 3)
    {
      if (Top.TraverseOrder == 3)
        Top.TraverseOrder = Top.PrevTraverseOrder;
      else
        Top.PrevTraverseOrder = Top.TraverseOrder;
      continue;
    }
    PopBack(&Stack);
    if (!(Top.To3 - Top.From3 == 1))
    {
      traverse_item First = Top, Second = Top;
      First.To3[FD] =
        Top.From3[FD] + (Top.To3[FD] - Top.From3[FD]) / 2;
      Second.From3[FD] =
        Top.From3[FD] + (Top.To3[FD] - Top.From3[FD]) / 2;
      extent Skip(First.From3, First.To3 - First.From3);
      //Second.NItemsBefore = First.NItemsBefore + Prod<u64>(Dims(Crop(Skip, Extent)));
      Second.ItemOrder = First.ItemOrder + Prod<i32>(Dims(Crop(Skip, VolExtent)));
      First.Address = Top.Address;
      Second.Address = Top.Address + Prod<u64>(First.To3 - First.From3);
      if (Second.From3 < To(Extent) && From(Extent) < Second.To3)
        PushBack(&Stack, Second);
      if (First.From3 < To(Extent) && From(Extent) < First.To3)
        PushBack(&Stack, First);
    }
    else
    {
      Top.LastItem = Size(Stack) == 0;
      idx2_PropagateIfError(Callback(Top));
    }
  }
}


} // namespace idx2

