/* Functions that implement file/chunk/brick lookup logic */
#include "idx2Lookup.h"
#include "Format.h"
#include "idx2Common.h"


namespace idx2
{


/* Interleave the bits of the brick according to the indexing template */
u64
GetLinearBrick(const idx2_file& Idx2, int Level, v3i Brick3)
{
  u64 LinearBrick = 0;
  int Size = Idx2.BricksOrderStr[Level].Len;
  for (int I = Size - 1; I >= 0; --I)
  {
    int D = Idx2.BricksOrderStr[Level][I] - 'X';
    LinearBrick |= (Brick3[D] & u64(1)) << (Size - I - 1);
    Brick3[D] >>= 1;
  }
  return LinearBrick;
}


v3i
GetSpatialBrick(const idx2_file& Idx2, int Level, u64 LinearBrick)
{
  int Size = Idx2.BricksOrderStr[Level].Len;
  v3i Brick3(0);
  for (int I = 0; I < Size; ++I)
  {
    int D = Idx2.BricksOrderStr[Level][I] - 'X';
    int J = Size - I - 1;
    Brick3[D] |= (LinearBrick & (u64(1) << J)) >> J;
    Brick3[D] <<= 1;
  }
  return Brick3 >> 1;
}


/* From a chunk address, return its spatial extent.
The extent can be "decoded" using From(extent) and Dims(extent), which returns
the coordinates of the first sample, and the size of the extent in each dimension.
*/
extent
ChunkAddressToSpatial(const idx2_file& Idx2, u64 ChunkAddress)
{
  u64 Brick;
  i8 Level;
  i8 Subband;
  i16 BitPlane;
  UnpackChunkAddress(Idx2, ChunkAddress, &Brick, &Level, &Subband, &BitPlane);
  v3i GroupBrick3 = Pow(Idx2.GroupBrick3, Level);
  v3i BrickDims3 = Idx2.BrickDims3 * GroupBrick3;
  v3i Brick3 = GetSpatialBrick(Idx2, Level, Brick);
  v3i ChunkFrom3 = Brick3 * BrickDims3;
  v3i ChunkDims3 = BrickDims3 * Idx2.BricksPerChunk3s[Level];
  extent ChunkExtent = extent(ChunkFrom3, ChunkDims3);
  return extent(Crop(ChunkExtent, extent(Idx2.Dims3)));
}


file_id
ConstructFilePath(const idx2_file& Idx2, u64 BrickAddress)
{
  u64 Brick;
  i16 BitPlane;
  i8 Level;
  i8 Subband;
  UnpackFileAddress(Idx2, BrickAddress, &Brick, &Level, &Subband, &BitPlane);
  return ConstructFilePath(Idx2, Brick, Level, Subband, BitPlane);
}


file_id
ConstructFilePath(const idx2_file& Idx2, u64 Brick, i8 Level, i8 Subband, i16 BpKey)
{
  //i16 BpFileKey = (BpKey * Idx2.BitPlanesPerChunk) / Idx2.BitPlanesPerFile;
#define idx2_PrintLevel idx2_Print(&Pr, "/L%02x", Level);
#define idx2_PrintBrick                                                                            \
  for (int Depth = 0; Depth + 1 < Idx2.FilesDirsDepth[Level].Len; ++Depth)                         \
  {                                                                                                \
    int BitLen =                                                                                   \
      idx2_BitSizeOf(u64) - Idx2.BricksOrderStr[Level].Len + Idx2.FilesDirsDepth[Level][Depth];    \
    idx2_Print(&Pr, "/B%" PRIx64, TakeFirstBits(Brick, BitLen));                                   \
    Brick <<= Idx2.FilesDirsDepth[Level][Depth];                                                   \
    Shift += Idx2.FilesDirsDepth[Level][Depth];                                                    \
  }
#define idx2_PrintExtension idx2_Print(&Pr, ".bin");
  u64 BrickBackup = Brick;
  int Shift = 0;
  thread_local static char FilePath[256];
  printer Pr(FilePath, sizeof(FilePath));
  idx2_Print(&Pr, "%.*s/%s/%s/", Idx2.Dir.Size, Idx2.Dir.ConstPtr, Idx2.Name, Idx2.Field);
  idx2_PrintLevel;
  idx2_PrintBrick;
  idx2_PrintExtension;
  u64 FileId = GetFileAddress(Idx2, BrickBackup, Level, Subband, BpKey);
  return file_id{ stref{ FilePath, Pr.Size }, FileId };
#undef idx2_PrintLevel
#undef idx2_PrintBrick
#undef idx2_PrintSubband
#undef idx2_PrintBitPlane
#undef idx2_PrintExtension
}


} // namespace idx2
