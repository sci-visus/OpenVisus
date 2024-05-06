#include "Volume.h"
#include "Assert.h"
#include "InputOutput.h"
#include "Math.h"
#include "MemoryMap.h"
#include "ScopeGuard.h"


namespace idx2
{


void
Unmap(mmap_volume* Vol)
{
  if (Vol->MMap.Buf)
  {
    if (Vol->MMap.Mode == map_mode::Write)
      FlushFile(&Vol->MMap);
    SyncFile(&Vol->MMap);
    UnmapFile(&Vol->MMap);
    CloseFile(&Vol->MMap);
  }
}


// TODO: add more return types to warn the user about the size of the volume loaded
// for example, the size of the volume may not agree with the input Dims3 and Type
// TODO: maybe add a mode where if the file size disagree with the inputs, then do
// not load the file
error<>
ReadVolume(cstr FileName, const v3i& Dims3, dtype Type, volume* Volume)
{
  error Ok;
  if (Size(Volume->Buffer) <= Prod(Dims3) * SizeOf(Type))
  {
    Ok = ReadFile(FileName, &Volume->Buffer);
    if (Ok.Code != err_code::NoError)
      return Ok;
    *Volume = volume(Volume->Buffer, Dims3, Type);
  }
  else
  { // volume's buffer is larger than the input
    idx2_Assert(Dims3 <= Dims(*Volume));
    idx2_Assert(Volume->Type != dtype::__Invalid__);
    volume Temp(Dims3, Volume->Type);
    idx2_CleanUp(Dealloc(&Temp));
    Ok = ReadFile(FileName, &Temp.Buffer);
    if (Ok.Code != err_code::NoError)
      return Ok;
    if (Volume->Type == dtype::float32)
      Copy<f32>(grid(Dims3), Temp, Volume);
    else if (Volume->Type == dtype::float64)
      Copy<f64>(grid(Dims3), Temp, Volume);
    return idx2_Error(err_code::NoError);
  }
  return idx2_Error(err_code::NoError);
}


error<mmap_err_code>
MapVolume(cstr FileName, const v3i& Dims3, dtype DType, mmap_volume* Vol, map_mode Mode)
{
  idx2_ReturnIfError(OpenFile(&Vol->MMap, FileName, Mode));
  if (Mode == map_mode::Write)
  {
    idx2_ReturnIfError(MapFile(&Vol->MMap, Prod<u64>(Dims3) * SizeOf(DType)));
  }
  else
  {
    idx2_ReturnIfError(MapFile(&Vol->MMap));
  }
  Vol->Vol = volume(Vol->MMap.Buf, Dims3, DType);
  return idx2_Error(mmap_err_code::NoError);
}


error<>
WriteVolume(FILE* Fp, const volume& Vol, const grid& Grid)
{
#define Body(type)                                                                                 \
  auto EndIt = End<type>(Grid, Vol);                                                               \
  for (auto It = Begin<type>(Grid, Vol); It != EndIt; ++It)                                        \
  {                                                                                                \
    if (fwrite(&(*It), sizeof(*It), 1, Fp) != 1)                                                   \
      return idx2_Error(err_code::FileWriteFailed);                                                \
  }
  idx2_DispatchOnType(Vol.Type) return idx2_Error(err_code::NoError);
#undef Body
}


error<>
WriteVolume(cstr FileName, const volume& Vol)
{
  return WriteBuffer(FileName, Vol.Buffer);
}


error<>
WriteVolume(cstr FileName, const volume& Vol, const extent& Ext)
{
  FILE* Fp = fopen(FileName, "wb");
  idx2_CleanUp(fclose(Fp));
#define Body(type)                                                                                 \
  auto EndIt = End<type>(Ext, Vol);                                                                \
  for (auto It = Begin<type>(Ext, Vol); It != EndIt; ++It)                                         \
  {                                                                                                \
    if (fwrite(&(*It), sizeof(*It), 1, Fp) != 1)                                                   \
      return idx2_Error(err_code::FileWriteFailed);                                                \
  }
  idx2_DispatchOnType(Vol.Type) return idx2_Error(err_code::NoError);
#undef Body
}


void
Resize(volume* Vol, const v3i& Dims3, allocator* Alloc)
{
  idx2_Assert(Vol->Type != dtype::__Invalid__);
  i64 NewSize = Prod<u64>(Dims3) * SizeOf(Vol->Type);
  if (Size(Vol->Buffer) < NewSize)
    Resize(&Vol->Buffer, NewSize, Alloc);
  SetDims(Vol, Dims3);
}


void
Resize(volume* Vol, const v3i& Dims3, dtype Type, allocator* Alloc)
{
  auto OldType = Vol->Type;
  Vol->Type = Type;
  Resize(Vol, Dims3, Alloc);
  if (Size(Vol->Buffer) < Prod<i64>(Dims3) * SizeOf(Vol->Type))
    Vol->Type = OldType;
}


void
Dealloc(volume* Vol)
{
  idx2_Assert(Vol);
  DeallocBuf(&Vol->Buffer);
}


void
Clone(const volume& Src, volume* Dst, allocator* Alloc)
{
  Clone(Src.Buffer, &Dst->Buffer, Alloc);
  Dst->Dims = Src.Dims;
  Dst->Type = Src.Type;
}


grid_split
Split(const grid& Grid, dimension D, int N)
{
  v3i Dims3 = Dims(Grid);
  idx2_Assert(N <= Dims3[D] && N >= 0);
  grid_split GridSplit{ Grid, Grid };
  v3i Mask3 = Dims3;
  Mask3[D] = N;
  SetDims(&GridSplit.First, Mask3);
  Mask3[D] = Dims3[D] - N;
  SetDims(&GridSplit.Second, Mask3);
  v3i From3 = From(Grid);
  From3[D] += N * Strd(Grid)[D];
  SetFrom(&GridSplit.Second, From3);
  return GridSplit;
}


grid_split
SplitAlternate(const grid& Grid, dimension D)
{
  v3i Dims3 = Dims(Grid);
  v3i Strd3 = Strd(Grid);
  v3i From3 = From(Grid);
  From3[D] += Strd3[D];
  grid_split GridSplit{ Grid, Grid };
  v3i Mask3 = Dims3;
  Mask3[D] = (Dims3[D] + 1) >> 1;
  SetDims(&GridSplit.First, Mask3);
  v3i NewStrd3 = Strd3;
  NewStrd3[D] <<= 1;
  SetStrd(&GridSplit.First, NewStrd3);
  Mask3[D] = Dims3[D] - Mask3[D];
  SetDims(&GridSplit.Second, Mask3);
  SetStrd(&GridSplit.Second, NewStrd3);
  SetFrom(&GridSplit.Second, From3);
  return GridSplit;
}


extent
BoundingBox(const extent& Ext1, const extent& Ext2)
{
  v3i From3 = Min(From(Ext1), From(Ext2));
  v3i To3 = Max(To(Ext1), To(Ext2));
  return extent(From3, To3 - From3);
}


} // namespace idx2
