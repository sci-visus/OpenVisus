#pragma once

#include "Array.h"
#include "Common.h"
#include "Volume.h"


namespace idx2
{


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


void
ExtrapolateCdf53(const v3i& Dims3, u64 TransformOrder, volume* Vol);

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
//#include <stlab/concurrency/future.hpp>


#define idx2_RowX(x, y, z, N) i64(z) * N.X* N.Y + i64(y) * N.X + (x)
#define idx2_RowY(y, x, z, N) i64(z) * N.X* N.Y + i64(y) * N.X + (x)
#define idx2_RowZ(z, x, y, N) i64(z) * N.X* N.Y + i64(y) * N.X + (x)


/* Forward lifting */
#define idx2_FLiftCdf53(z, y, x)                                                                   \
  template <typename t> void FLiftCdf53##x(                                                        \
    const grid& Grid, const v3i& M, lift_option Opt, volume* Vol)                                  \
  {                                                                                                \
    v3i P = From(Grid), D = Dims(Grid), S = Strd(Grid), N = Dims(*Vol);                            \
    if (D.x == 1)                                                                                  \
      return;                                                                                      \
    idx2_Assert(M.x <= N.x);                                                                       \
    idx2_Assert(IsPow2(S.X) && IsPow2(S.Y) && IsPow2(S.Z));                                        \
    idx2_Assert(D.x >= 2);                                                                         \
    idx2_Assert(IsEven(P.x));                                                                      \
    idx2_Assert(P.x + S.x * (D.x - 2) < M.x);                                                      \
    buffer_t<t> F(Vol->Buffer);                                                                    \
    int x0 = Min(P.x + S.x * D.x, M.x);       /* extrapolated position */                          \
    int x1 = Min(P.x + S.x * (D.x - 1), M.x); /* last position */                                  \
    int x2 = P.x + S.x * (D.x - 2);           /* second last position */                           \
    int x3 = P.x + S.x * (D.x - 3);           /* third last position */                            \
    bool Ext = IsEven(D.x);                                                                        \
    for (int z = P.z; z < P.z + S.z * D.z; z += S.z)                                               \
    {                                                                                              \
      int zz = Min(z, M.z);                                                                        \
      for (int y = P.y; y < P.y + S.y * D.y; y += S.y)                                             \
      {                                                                                            \
        int yy = Min(y, M.y);                                                                      \
        if (Ext)                                                                                   \
        {                                                                                          \
          idx2_Assert(M.x < N.x);                                                                  \
          t A = F[idx2_Row##x(x2, yy, zz, N)]; /* 2nd last (even) */                               \
          t B = F[idx2_Row##x(x1, yy, zz, N)]; /* last (odd) */                                    \
          /* store the extrapolated value at the boundary position */                              \
          F[idx2_Row##x(x0, yy, zz, N)] = 2 * B - A;                                               \
        }                                                                                          \
        /* predict (excluding last odd position) */                                                \
        for (int x = P.x + S.x; x < P.x + S.x * (D.x - 2); x += 2 * S.x)                           \
        {                                                                                          \
          t& Val = F[idx2_Row##x(x, yy, zz, N)];                                                   \
          Val -= (F[idx2_Row##x(x - S.x, yy, zz, N)] + F[idx2_Row##x(x + S.x, yy, zz, N)]) / 2;    \
        }                                                                                          \
        if (!Ext)                                                                                  \
        { /* no extrapolation, predict at the last odd position */                                 \
          t& Val = F[idx2_Row##x(x2, yy, zz, N)];                                                  \
          Val -= (F[idx2_Row##x(x1, yy, zz, N)] + F[idx2_Row##x(x3, yy, zz, N)]) / 2;              \
        }                                                                                          \
        else if (x1 < M.x)                                                                         \
        {                                                                                          \
          F[idx2_Row##x(x1, yy, zz, N)] = 0;                                                       \
        }                                                                                          \
        /* update (excluding last odd position) */                                                 \
        if (Opt != lift_option::NoUpdate)                                                          \
        {                                                                                          \
          for (int x = P.x + S.x; x < P.x + S.x * (D.x - 2); x += 2 * S.x)                         \
          {                                                                                        \
            t Val = F[idx2_Row##x(x, yy, zz, N)];                                                  \
            F[idx2_Row##x(x - S.x, yy, zz, N)] += Val / 4;                                         \
            F[idx2_Row##x(x + S.x, yy, zz, N)] += Val / 4;                                         \
          }                                                                                        \
          if (!Ext)                                                                                \
          { /* no extrapolation, update at the last odd position */                                \
            t Val = F[idx2_Row##x(x2, yy, zz, N)];                                                 \
            F[idx2_Row##x(x3, yy, zz, N)] += Val / 4;                                              \
            if (Opt == lift_option::Normal)                                                        \
              F[idx2_Row##x(x1, yy, zz, N)] += Val / 4;                                            \
            else if (Opt == lift_option::PartialUpdateLast)                                        \
              F[idx2_Row##x(x1, yy, zz, N)] = Val / 4;                                             \
          }                                                                                        \
        }                                                                                          \
      }                                                                                            \
    }                                                                                              \
  }


// TODO: this function does not make use of PartialUpdateLast
#define idx2_ILiftCdf53(z, y, x)                                                                   \
  template <typename t> void ILiftCdf53##x(                                                        \
    const grid& Grid, const v3i& M, lift_option Opt, volume* Vol)                                  \
  {                                                                                                \
    v3i P = From(Grid), D = Dims(Grid), S = Strd(Grid), N = Dims(*Vol);                            \
    if (D.x == 1)                                                                                  \
      return;                                                                                      \
    idx2_Assert(M.x <= N.x);                                                                       \
    idx2_Assert(IsPow2(S.X) && IsPow2(S.Y) && IsPow2(S.Z));                                        \
    idx2_Assert(D.x >= 2);                                                                         \
    idx2_Assert(IsEven(P.x));                                                                      \
    idx2_Assert(P.x + S.x * (D.x - 2) < M.x);                                                      \
    buffer_t<t> F(Vol->Buffer);                                                                    \
    int x0 = Min(P.x + S.x * D.x, M.x);       /* extrapolated position */                          \
    int x1 = Min(P.x + S.x * (D.x - 1), M.x); /* last position */                                  \
    int x2 = P.x + S.x * (D.x - 2);           /* second last position */                           \
    int x3 = P.x + S.x * (D.x - 3);           /* third last position */                            \
    bool Ext = IsEven(D.x);                                                                        \
    for (int z = P.z; z < P.z + S.z * D.z; z += S.z)                                               \
    {                                                                                              \
      int zz = Min(z, M.z);                                                                        \
      for (int y = P.y; y < P.y + S.y * D.y; y += S.y)                                             \
      {                                                                                            \
        int yy = Min(y, M.y);                                                                      \
        /* inverse update (excluding last odd position) */                                         \
        if (Opt != lift_option::NoUpdate)                                                          \
        {                                                                                          \
          for (int x = P.x + S.x; x < P.x + S.x * (D.x - 2); x += 2 * S.x)                         \
          {                                                                                        \
            t Val = F[idx2_Row##x(x, yy, zz, N)];                                                  \
            F[idx2_Row##x(x - S.x, yy, zz, N)] -= Val / 4;                                         \
            F[idx2_Row##x(x + S.x, yy, zz, N)] -= Val / 4;                                         \
          }                                                                                        \
          if (!Ext)                                                                                \
          { /* no extrapolation, inverse update at the last odd position */                        \
            t Val = F[idx2_Row##x(x2, yy, zz, N)];                                                 \
            F[idx2_Row##x(x3, yy, zz, N)] -= Val / 4;                                              \
            if (Opt == lift_option::Normal)                                                        \
              F[idx2_Row##x(x1, yy, zz, N)] -= Val / 4;                                            \
          }                                                                                        \
          else                                                                                     \
          { /* extrapolation, need to "fix" the last position (odd) */                             \
            t A = F[idx2_Row##x(x0, yy, zz, N)];                                                   \
            t B = F[idx2_Row##x(x2, yy, zz, N)];                                                   \
            F[idx2_Row##x(x1, yy, zz, N)] = (A + B) / 2;                                           \
          }                                                                                        \
        }                                                                                          \
        /* inverse predict (excluding last odd position) */                                        \
        for (int x = P.x + S.x; x < P.x + S.x * (D.x - 2); x += 2 * S.x)                           \
        {                                                                                          \
          t& Val = F[idx2_Row##x(x, yy, zz, N)];                                                   \
          Val += (F[idx2_Row##x(x - S.x, yy, zz, N)] + F[idx2_Row##x(x + S.x, yy, zz, N)]) / 2;    \
        }                                                                                          \
        if (!Ext)                                                                                  \
        { /* no extrapolation, inverse predict at the last odd position */                         \
          t& Val = F[idx2_Row##x(x2, yy, zz, N)];                                                  \
          Val += (F[idx2_Row##x(x1, yy, zz, N)] + F[idx2_Row##x(x3, yy, zz, N)]) / 2;              \
        }                                                                                          \
      }                                                                                            \
    }                                                                                              \
  }


namespace idx2
{


idx2_FLiftCdf53(Z, Y, X)   // X forward lifting
idx2_FLiftCdf53(Z, X, Y) // Y forward lifting
idx2_FLiftCdf53(Y, X, Z) // Z forward lifting
idx2_ILiftCdf53(Z, Y, X) // X inverse lifting
idx2_ILiftCdf53(Z, X, Y) // Y inverse lifting
idx2_ILiftCdf53(Y, X, Z) // Z inverse lifting


} // namespace idx2

#undef idx2_FLiftCdf53
#undef idx2_ILiftCdf53
#undef idx2_RowX
#undef idx2_RowY
#undef idx2_RowZ
