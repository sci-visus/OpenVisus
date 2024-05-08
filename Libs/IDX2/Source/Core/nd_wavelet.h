#pragma once

#include "Array.h"
#include "Common.h"
#include "Volume.h"
#include "nd_volume.h"


namespace idx2
{


struct wavelet_block
{
  v3i Levels;
  grid Grid;
  volume Volume;
  bool IsPacked = false; // if the Volume stores data just for this block
};


struct wav_basis_norms
{
  array<f64> ScalNorms; // scaling function norms
  array<f64> WaveNorms; // wavelet function norms
};


wav_basis_norms
GetCdf53Norms(int NLevels);

void
Dealloc(wav_basis_norms* WbN);


template <int N> struct wav_basis_norms_static
{
  stack_array<f64, N> ScalNorms;
  stack_array<f64, N> WaveNorms;
};


template <int N> wav_basis_norms_static<N>
GetCdf53NormsFast()
{
  wav_basis_norms_static<N> Result;
  f64 Num1 = 3, Num2 = 23;
  for (int I = 0; I < N; ++I)
  {
    Result.ScalNorms[I] = sqrt(Num1 / (1 << (I + 1)));
    Num1 = Num1 * 4 - 1;
    Result.WaveNorms[I] = sqrt(Num2 / (1 << (I + 5)));
    Num2 = Num2 * 4 - 33;
  }
  return Result;
}

struct subband
{
  grid Grid;
  grid AccumGrid;   // accumulative grid (with the coarsest same-level subband)
  v3<i8> Level3;    // convention: 0 is the coarsest
  v3<i8> Level3Rev; // convention: 0 is the finest
  v3<i8> LowHigh3;
  i8 Level = 0;
};


struct transform_info
{
  wav_basis_norms_static<16> BasisNorms;
  stack_array<grid, 32> StackGrids;
  stack_array<int, 32> StackAxes;
  u64 TformOrder;
  int StackSize;
  int NPasses;
};

struct transform_info_v2
{
  wav_basis_norms_static<16> BasisNorms;
  array<nd_grid> Grids;
  array<i8> Axes;
  i8 NLevels;
};

void
ComputeTransformDetails(transform_info* Td, const v3i& Dims3, int NLevels, u64 TformOrder);


/*
New set of lifting functions. We assume the volume where we want to transform
to happen (M) is contained within a bigger volume (Vol). When Dims(Grid) is even,
extrapolation will happen, in a way that the last (odd) wavelet coefficient is 0.
We assume the storage at index M3.(x/y/z) is available to store the extrapolated
values if necessary. We could always store the extrapolated value at the correct
position, but storing it at M3.(x/y/z) allows us to avoid having to actually use
extra storage (which are mostly used to store 0 wavelet coefficients).
*/
enum lift_option
{
  Normal,
  PartialUpdateLast,
  NoUpdateLast,
  NoUpdate
};
template <typename t> void
FLiftCdf53X(const grid& Grid, const v3i& M3, lift_option Opt, volume* Vol);
template <typename t> void
FLiftCdf53Y(const grid& Grid, const v3i& M3, lift_option Opt, volume* Vol);
template <typename t> void
FLiftCdf53Z(const grid& Grid, const v3i& M3, lift_option Opt, volume* Vol);
/* The inverse lifting functions can be used to extrapolate the volume */
template <typename t> void
ILiftCdf53X(const grid& Grid, const v3i& M3, lift_option Opt, volume* Vol);
template <typename t> void
ILiftCdf53Y(const grid& Grid, const v3i& M3, lift_option Opt, volume* Vol);
template <typename t> void
ILiftCdf53Z(const grid& Grid, const v3i& M3, lift_option Opt, volume* Vol);

/*
"In-place" extrapolate a volume to size 2^L+1, which is assumed to be the
dims of Vol. The original volume is stored in the D3 sub-volume of Vol.
*/
void
Extrapolate(v3i D3, volume* Vol);

void
ForwardCdf53(const extent& Ext, int NLevels, volume* Vol);
void
InverseCdf53(const extent& Ext, int NLevels, volume* Vol);
void
ExtrapolateCdf53(const v3i& Dims3, u64 TransformOrder, volume* Vol);
void
ExtrapolateCdf53(const transform_info& Td, volume* Vol);
void
ForwardCdf53(const v3i& Dims3,
             const v3i& M3,
             int Iter,
             int NLevels,
             u64 TformOrder,
             volume* Vol,
             bool Normalize = false);
void
InverseCdf53(const v3i& Dims3,
             const v3i& M3,
             int Iter,
             int NLevels,
             u64 TformOrder,
             volume* Vol,
             bool Normalize = false);
void
ForwardCdf53(const v3i& M3,
             int Iter,
             const array<subband>& Subbands,
             const transform_info& Td,
             volume* Vol,
             bool Normalize = false);
void
InverseCdf53(const v3i& M3,
             int Iter,
             const array<subband>& Subbands,
             const transform_info& Td,
             volume* Vol,
             bool Normalize = false);

template <typename t> struct array;

void
BuildSubbands(const v3i& N3, int NLevels, array<extent>* Subbands);

void
BuildSubbands(const v3i& N3, int NLevels, array<grid>* Subbands);

u64
EncodeTransformOrder(const stref& TransformOrder);

void
DecodeTransformOrder(u64 Input, str Output);

i8
DecodeTransformOrder(u64 Input, v3i N3, str Output);

i8
DecodeTransformOrder(u64 Input, int Passes, str Output);

void
BuildSubbands(const v3i& N3, int NLevels, u64 TransformOrder, array<subband>* Subbands);

grid
MergeSubbandGrids(const grid& Sb1, const grid& Sb2);

struct wav_grids
{
  grid WavGrid; // grid of wavelet coefficients to copy
  grid ValGrid; // the output grid of values
  grid WrkGrid; // determined using the WavGrid
};

} // namespace idx2


#include "Algorithm.h"
#include "Assert.h"
#include "BitOps.h"
#include "Math.h"
#include "Memory.h"
#include "Volume.h"


