//#define _CRTDBG_MAP_ALLOC
//#include <stdlib.h>
//#include <crtdbg.h>

#if VISUS_IDX2
#include <Visus/IdxDataset2.h>
#include <Visus/Access.h>
#include <Visus/File.h>
#endif

#include "../idx2.h"

using namespace idx2;

/* Parse the decode options */
static void
ParseDecodeOptions(int Argc, cstr* Argv, params* P)
{
  idx2_ExitIf(!OptVal(Argc, Argv, "--decode", &P->InputFile), "Provide input file after --decode\n"); // TODO: we actually cannot detect if --decode is not followed by a file name
  v3i First3, Last3;
  if (OptExists(Argc, Argv, "--first") || OptExists(Argc, Argv, "--last"))
  {
    // Parse the first extent coordinates (--first)
    idx2_ExitIf(!OptVal(Argc, Argv, "--first", &First3),
                "Provide --first (the first sample of the box to decode)\n"
                "Example: --first 0 400 0\n");

    // Parse the last extent coordinates (--last)
    idx2_ExitIf(!OptVal(Argc, Argv, "--last", &Last3),
                "Provide --last (the last sample of the box to decode)\n"
                "Example: --first 919 655 719\n");
    P->DecodeExtent = extent(First3, Last3 - First3 + 1);
  }

  // Parse the downsampling factor
  OptVal(Argc, Argv, "--downsampling", &P->DownsamplingFactor3);

  // Parse the decode accuracy (--accuracy)
  OptVal(Argc, Argv, "--tolerance", &P->DecodeTolerance);

  // Parse the input directory (--in_dir)
  OptVal(Argc, Argv, "--in_dir", &P->InDir);
  if (!OptExists(Argc, Argv, "--in_dir"))
  { // try to parse the input directory from the input file
    auto Parent = GetParentPath(P->InputFile);
    P->InDir = GetParentPath(Parent);
    if (P->InDir == Parent)
      P->InDir = "./";
  }

  if (OptExists(Argc, Argv, "--brick_map"))
  {
    P->OutMode = params::out_mode::HashMap;
  }

  P->ParallelDecode = OptExists(Argc, Argv, "--parallel");
}


/* Parse the metadata (name, field, dims, dtype) from the command line */
static void
ParseMetaData(int Argc, cstr* Argv, params* P)
{
  // Parse the name
  idx2_ExitIf(!OptVal(Argc, Argv, "--name", P->Meta.Name),
              "Provide --name\n"
              "Example: --name Miranda\n");

  // Parse the field name (--field)
  idx2_ExitIf(!OptVal(Argc, Argv, "--field", P->Meta.Field),
              "Provide --field\n"
              "Example: --field Density\n");

  // Parse the dimensions
  idx2_ExitIf(!OptVal(Argc, Argv, "--dims", &P->Meta.Dims3),
              "Provide --dims\n"
              "Example: --dims 384 384 256\n");

  // Parse the data type (--type)
  char DType[8];
  idx2_ExitIf(!OptVal(Argc, Argv, "--type", DType),
              "Provide --type (float32 or float64)\n"
              "Example: --type float32\n");
  P->Meta.DType = StringTo<dtype>()(stref(DType));
  idx2_ExitIf(P->Meta.DType != dtype::float32 && P->Meta.DType != dtype::float64,
              "Unsupported type\n");
}


/* If the files in P->InputFiles have different sizes, return false */
static bool
CheckFileSizes(params* P)
{
  i64 ConstantSize = 0;
  idx2_For (int, I, 0, Size(P->InputFiles))
  {
    i64 S = GetFileSize(stref(P->InputFiles[I].Arr));
    if (I == 0)
      ConstantSize = S;
    if (S != ConstantSize)
      return false;
  }

  int DTypeSize = SizeOf(P->Meta.DType);
  idx2_ExitIf(ConstantSize % DTypeSize != 0, "File size not multiple of dtype size\n");
  //P->NSamplesInFile = ConstantSize / DTypeSize;

  return true;
}


/* Parse the options specific to encoding */
static void
ParseEncodeOptions(int Argc, cstr* Argv, params* P)
{
  idx2_ExitIf(!OptVal(Argc, Argv, "--encode", &P->InputFile), "Provide input file after --encode\n"); // TODO: we actually cannot currently detect if --encode is not followed by a file names
  // First, try to parse the metadata from the file name
  auto ParseOk = StrToMetaData(P->InputFile, &P->Meta);
  // if the previous parse fails, parse metadata from the command line
  if (!ParseOk)
  {
    ParseMetaData(Argc, Argv, P);

    // If the input file is a .txt file, read all the file names in the txt into an array
    if (GetExtension(P->InputFile) == idx2_StRef("txt"))
    {
      // Parse the file names
      idx2_RAII(FILE*, Fp = fopen(P->InputFile, "rb"), , if (Fp) fclose(Fp));
      ReadLines(Fp, &P->InputFiles);
      idx2_ExitIf(!CheckFileSizes(P), "Input files have to be of the same size\n");
    }
  }

  // Check the data type
  idx2_ExitIf(P->Meta.DType == dtype::__Invalid__, "Data type not supported\n");

  // Parse the brick dimensions (--brick_size)
  OptVal(Argc, Argv, "--brick_size", &P->BrickDims3);
  //idx2_ExitIf(!OptVal(Argc, Argv, "--brick_size", &P->BrickDims3),
  //            "Provide --brick_size\n"
  //            "Example: --brick_size 32 32 32\n");

  // Parse the number of levels (--num_levels)
  OptVal(Argc, Argv, "--num_levels", &P->NLevels);

  // Parse the tolerance (--tolerance)
  OptVal(Argc, Argv, "--tolerance", &P->Tolerance);
  if (P->Tolerance <= 0)
  {
    if (P->Meta.DType == dtype::float32)
      P->Tolerance = GetMachineEpsilon<f32>();
    else if (P->Meta.DType == dtype::float64)
      P->Tolerance = GetMachineEpsilon<f64>();
    printf("tolerance = %.16f (machine epsilon)\n", P->Tolerance);
  }

  OptVal(Argc, Argv, "--bit_planes_per_chunk", &P->BitPlanesPerChunk);
  OptVal(Argc, Argv, "--bit_planes_per_file", &P->BitPlanesPerFile);
  OptVal(Argc, Argv, "--bricks_per_chunk", &P->BricksPerChunk);
  OptVal(Argc, Argv, "--chunks_per_file", &P->ChunksPerFile);
  OptVal(Argc, Argv, "--files_per_dir", &P->FilesPerDir);

  // Parse the optional version (--version)
  OptVal(Argc, Argv, "--version", &P->Version);
  // Parse the optional output directory (--out_dir)
  OptVal(Argc, Argv, "--out_dir", &P->OutDir);

}


/* Parse the parameters to the program from the command line */
params
ParseParams(int Argc, cstr* Argv)
{
  params P;

  P.Action = OptExists(Argc, Argv, "--encode")   ? action::Encode
             : OptExists(Argc, Argv, "--decode") ? P.Action = action::Decode
                                                 : action::__Invalid__;

  idx2_ExitIf(P.Action == action::__Invalid__,
              "Provide either --encode or --decode\n"
              "Example 1: --encode /Users/abc/Miranda-Density-[384-384-256]-Float64.raw\n"
              "Example 2: --encode Miranda-Density-[384-384-256]-Float64.raw\n"
              "Example 3: --decode /Users/abc/Miranda/Density.idx2\n"
              "Example 4: --decode Miranda/Density.idx2\n");

  //// Parse the input file (--input)
  //idx2_ExitIf(!OptVal(Argc, Argv, "--input", &P.InputFile),
  //            "Provide --input\n"
  //            "Example: --input /Users/abc/MIRANDA-DENSITY-[384-384-256]-Float64.raw\n");

  // Parse the pause option (--pause): wait for keyboard input at the end
  P.Pause = OptExists(Argc, Argv, "--pause");

  // Parse the optional output directory (--out_dir)
  OptVal(Argc, Argv, "--out_dir", &P.OutDir);
  // Parse the optional output file (--out_file)
  OptVal(Argc, Argv, "--out_file", &P.OutFile);

  // Parse the dry run option (--dry): if enabled, skip writing the output file
  P.OutMode =
    OptExists(Argc, Argv, "--dry") ? params::out_mode::NoOutput : params::out_mode::RegularGridFile;

  // Perform parsing depending on the action
  if (P.Action == action::Encode)
    ParseEncodeOptions(Argc, Argv, &P);
  else if (P.Action == action::Decode)
    ParseDecodeOptions(Argc, Argv, &P);

  return P;
}


/* "Copy" the parameters from the command line to the internal idx2_file struct */
static error<idx2_err_code>
SetParams(idx2_file* Idx2, params* P)
{
  SetName(Idx2, P->Meta.Name);
  SetField(Idx2, P->Meta.Field);
  SetVersion(Idx2, P->Version);
  SetDimensions(Idx2, P->Meta.Dims3);
  SetDataType(Idx2, P->Meta.DType);
  SetBrickSize(Idx2, P->BrickDims3);
  SetBricksPerChunk(Idx2, P->BricksPerChunk);
  SetChunksPerFile(Idx2, P->ChunksPerFile);
  SetBitPlanesPerChunk(Idx2, P->BitPlanesPerChunk);
  SetNumLevels(Idx2, (i8)P->NLevels);
  SetTolerance(Idx2, P->Tolerance);
  SetFilesPerDirectory(Idx2, P->FilesPerDir);
  SetDir(Idx2, P->OutDir);
  return Finalize(Idx2, P);
}

int
Idx2App(int Argc, const char* Argv[])
{
  SetHandleAbortSignals();

  idx2_RAII(params, P, P = ParseParams(Argc, Argv));

  idx2_RAII(timer,
            Timer,
            StartTimer(&Timer),
            printf("Total time: %f seconds\n", Seconds(ElapsedTime(&Timer))));

  /* Perform the action */
  idx2_RAII(idx2_file, Idx2);

  if (P.Action == action::Encode)
  {
    RemoveDir(idx2_PrintScratch("%s/%s/%s", P.OutDir, P.Meta.Name, P.Meta.Field));
    idx2_ExitIfError(SetParams(&Idx2, &P));

#if VISUS_IDX2
    //make sure these instances are alive for encoding/decoding operations
    Visus::SharedPtr<Visus::IdxDataset2> dataset;
    Visus::SharedPtr<Visus::Access> access;

    //by default inside OpenVisus I am switching to IDX1 data access
    if (!Visus::IdxDataset2::VISUS_USE_IDX2_FILE_FORMAT()) 
    {
      // need to have the *.idx2 file written in advance (otherwise I cannot load the IdxDataset2 inside OpenVisus)
      std::string url = idx2_PrintScratch("%s/%s/%s.idx2", P.OutDir, P.Meta.Name, P.Meta.Field);
      {
        auto Min = 0.0;
        std::swap(Idx2.ValueRange.Min, Min);
        auto Max = 1.0;
        std::swap(Idx2.ValueRange.Max, Max);
        Visus::FileUtils::createDirectory(Visus::Path(url).getParent());
        WriteMetaFile(Idx2, P, url.c_str());
        std::swap(Idx2.ValueRange.Min, Min);
        std::swap(Idx2.ValueRange.Max, Max);
      }

      dataset = std::dynamic_pointer_cast<Visus::IdxDataset2>(Visus::LoadDataset(url));
      access = dataset->createAccess();
      access->disableWriteLocks();
      access->disableCompression();

      dataset->enableExternalWrite(Idx2, access);
      dataset->enableExternalRead(Idx2, access);
    }
#endif

    if (Size(P.InputFiles) > 0)
    { // the input contains multiple files
      idx2_ExitIf(true, "File list input not supported at the moment\n");
    }
    else if (Size(P.InputFiles) == 0)
    { // a single raw volume is provided
      idx2_RAII(mmap_volume, Vol, (void)Vol, Unmap(&Vol));
      //      error Result = ReadVolume(P.Meta.File, P.Meta.Dims3, P.Meta.DType, &Vol.Vol);
      idx2_ExitIfError(MapVolume(P.InputFile, P.Meta.Dims3, P.Meta.DType, &Vol, map_mode::Read));
      brick_copier Copier(&Vol.Vol);
      if (P.Version == v2i(1, 0))
      {
        idx2_ExitIfError(Encode(&Idx2, P, Copier));
      }
      //else if (P.Version == v2i(2, 0))
      //{
      //  idx2_ExitIfError(Encode_v2(&Idx2, P, Copier));
      //}
    }
  }
  else if (P.Action == action::Decode)
  {
    idx2_ExitIfError(Init(&Idx2, P));

#if VISUS_IDX2
    // make sure these instances are alive for encoding/decoding operations
    Visus::SharedPtr<Visus::IdxDataset2> dataset;
    Visus::SharedPtr<Visus::Access> access;

    // by default inside OpenVisus I am switching to IDX1 data access
    if (!Visus::IdxDataset2::VISUS_USE_IDX2_FILE_FORMAT())
    { 
      std::string url = idx2_PrintScratch("%s%s/%s.idx2", Idx2.Dir.ConstPtr, Idx2.Name, Idx2.Field);
      dataset = std::dynamic_pointer_cast<Visus::IdxDataset2>(Visus::LoadDataset(url));
      access = dataset->createAccess();
      dataset->enableExternalRead(Idx2, access);
    }
#endif

    if (P.ParallelDecode)
    {
      idx2_ExitIfError(ParallelDecode(Idx2, P));
    }
    else
    {
      idx2_ExitIfError(Decode(Idx2, P));
    }
  }

  if (P.Pause)
  {
    printf("Press any key to end...\n");
    getchar();
  }

  //_CrtDumpMemoryLeaks();
  return 0;
}

#if !VISUS_IDX2 // visus will have a different entry point
int main(int Argc, const char* Argv[]) {
  return Idx2App(Argc, Argv);
}
#endif

