#include "Algorithm.h"
#include "Array.h"
#include "Assert.h"
#include "BitOps.h"
#include "CircularQueue.h"
#include "Common.h"
#include "DataTypes.h"
#include "Function.h"
#include "Logger.h"
#include "Math.h"
#include "Memory.h"
#include "ScopeGuard.h"
#include "Utilities.h"
#include "nd_wavelet.h"
#include "nd_volume.h"


namespace idx2
{


/*
The TransformTemplate is a string that looks like ':210210:210:210', where each number denotes a dimension.
The transform happens along the dimensions from right to left.
The ':' character denotes level boundary.
The number of ':' characters is the same as the number of levels (which is also Idx2.NLevels).
This is in contrast to v1, in which NLevels is always just 1.
*/
static transform_info_v2
ComputeTransformInfo(stref TransformTemplate, const nd_index& Dims)
{
  transform_info_v2 TransformInfo;
  nd_index CurrentDims = Dims;
  nd_index ExtrapolatedDims = CurrentDims;
  nd_index CurrentSpacing(1); // spacing
  nd_grid G(Dims);
  i8 Pos = TransformTemplate.Size - 1;
  i8 NLevels = 0;
  while (Pos >= 0)
  {
    idx2_Assert(Pos >= 0);
    if (TransformTemplate[Pos--] == ':') // the character ':' (next level)
    {
      G.Spacing = CurrentSpacing;
      G.Dims = CurrentDims;
      ExtrapolatedDims = CurrentDims;
      ++NLevels;
    }
    else // one of 0, 1, 2
    {
      i8 D = TransformTemplate[Pos--] - '0';
      PushBack(&TransformInfo.Grids, G);
      PushBack(&TransformInfo.Axes, D);
      ExtrapolatedDims[D] = CurrentDims[D] + IsEven(CurrentDims[D]);
      G.Dims = ExtrapolatedDims;
      CurrentDims[D] = (ExtrapolatedDims[D] + 1) / 2;
      CurrentSpacing[D] *= 2;
    }
  }
  TransformInfo.BasisNorms = GetCdf53NormsFast<16>();
  TransformInfo.NLevels = NLevels;

  return TransformInfo;
}


template <typename t> void
FLiftCdf53(const nd_grid& Grid, const nd_size& StorageDims, i8 d, lift_option Option, nd_volume* Vol)
{
  // TODO: check alignment when allocating to ensure auto SIMD works
  // TODO: add openmp simd
  nd_index P = MakeFastestDimension(Grid.From   , d);
  nd_size  D = MakeFastestDimension(Grid.Dims   , d);
  nd_size  S = MakeFastestDimension(Grid.Spacing, d);
  nd_size  N = MakeFastestDimension(Vol->Dims   , d);
  nd_size  M = MakeFastestDimension(StorageDims , d);
  // now dimension d is effectively dimension 0
  if (D[0] <= 1)
    return;

  idx2_Assert(M[0] <= N[0]);
  idx2_Assert(IsPow2(S));
  idx2_Assert(IsEven(P[0]));
  idx2_Assert(P[0] + S[0] * (D[0] - 2) < M[0]);

  buffer_t<t> F(Vol->Buffer);
  auto X0 = Min(P[0] + S[0] * D[0], M[0]);       /* extrapolated position */
  auto X1 = Min(P[0] + S[0] * (D[0] - 1), M[0]); /* last position */
  auto X2 = P[0] + S[0] * (D[0] - 2);            /* second last position */
  auto X3 = P[0] + S[0] * (D[0] - 3);            /* third last position */
  bool SignalIsEven = IsEven(D[0]);
  ndOuterLoop(P, P + S * D, S, [&](const nd_index& ndI)
  {
    nd_index MinIdx = Min(ndI, M);
    nd_index SecondLastIdx = SetDimension(MinIdx, 0, X2);
    nd_index LastIdx       = SetDimension(MinIdx, 0, X1);
    nd_index ThirdLastIdx  = SetDimension(MinIdx, 0, X3);
    /* extrapolate if needed */
    if (SignalIsEven)
    {
      idx2_Assert(M[0] < N[0]);
      t A = F[LinearIndex(MinIdx, N)]; /* 2nd last (even) */
      t B = F[LinearIndex(MinIdx, N)]; /* last (odd) */
      /* store the extrapolated value at the boundary position */
      nd_index ExtrapolateIdx = SetDimension(MinIdx, 0, X0);
      F[LinearIndex(ExtrapolateIdx, N)] = 2 * B - A;
    }
    /* predict (excluding last odd position) */
    for (auto X = P[0] + S[0]; X < X2; X += 2 * S[0])
    {
      nd_index MiddleIdx = SetDimension(MinIdx, 0, X);
      nd_index LeftIdx   = SetDimension(MinIdx, 0, X - S[0]);
      nd_index RightIdx  = SetDimension(MinIdx, 0, X + S[0]);
      t& Val = F[LinearIndex(MiddleIdx, N)];
      Val -= (F[LinearIndex(LeftIdx, N)] + F[LinearIndex(RightIdx, N)]) / 2;
    }
    /* predict at the last odd position */
    if (!SignalIsEven)
    {
      t& Val = F[LinearIndex(SecondLastIdx, N)];
      Val -= (F[LinearIndex(LastIdx, N)] + F[LinearIndex(ThirdLastIdx, N)]) / 2;
    }
    else if (X1 < M[0])
    {
      nd_index LastIdx       = SetDimension(MinIdx, 0, X1);
      F[LinearIndex(LastIdx, N)] = 0;
    }
    /* update (excluding last odd position) */
    if (Option != lift_option::NoUpdate)
    {
      /* update excluding the last odd position */
      for (auto X = P[0] + S[0]; X < X2; X += 2 * S[0])
      {
        nd_index MiddleIdx = SetDimension(MinIdx, 0, X);
        nd_index LeftIdx   = SetDimension(MinIdx, 0, X - S[0]);
        nd_index RightIdx  = SetDimension(MinIdx, 0, X + S[0]);
        t Val = F[LinearIndex(MiddleIdx, N)];
        F[LinearIndex(LeftIdx, N)]  += Val / 4;
        F[LinearIndex(RightIdx, N)] += Val / 4;
      }
      /* update at the last odd position */
      if (!SignalIsEven)
      {
        t Val = F[LinearIndex(SecondLastIdx, N)];
        F[LinearIndex(ThirdLastIdx, N)] += Val / 4;
        if (Option == lift_option::Normal)
          F[LinearIndex(LastIdx, N)] += Val / 4;
        else if (Option == lift_option::PartialUpdateLast)
          F[LinearIndex(LastIdx, N)] = Val / 4;
      }
    }
  });
}


// TODO: this function does not make use of PartialUpdateLast
template <typename t> void
ILiftCdf53(const nd_grid& Grid, const nd_size& StorageDims, i8 d, lift_option Option, nd_volume* Vol)
{
  nd_index P = MakeFastestDimension(Grid.From, d);
  nd_size D = MakeFastestDimension(Grid.Dims, d);
  nd_size S = MakeFastestDimension(Grid.Spacing, d);
  nd_size N = MakeFastestDimension(Vol->Dims, d);
  nd_size M = MakeFastestDimension(StorageDims, d);
  // now dimension d is effectively dimension 0
  if (D[0] <= 1)
    return;

  idx2_Assert(M[0] <= N[0]);
  idx2_Assert(IsPow2(S));
  idx2_Assert(IsEven(P[0]));
  idx2_Assert(P[0] + S[0] * (D[0] - 2) < M[0]);

  buffer_t<t> F(Vol->Buffer);
  auto X0 = Min(P[0] + S[0] * D[0], M[0]);       /* extrapolated position */
  auto X1 = Min(P[0] + S[0] * (D[0] - 1), M[0]); /* last position */
  auto X2 = P[0] + S[0] * (D[0] - 2);            /* second last position */
  auto X3 = P[0] + S[0] * (D[0] - 3);            /* third last position */
  bool SignalIsEven = IsEven(D[0]);
  ndOuterLoop(P, P + S * D, S, [&](const nd_index& ndI)
  {
    nd_index MinIdx = Min(ndI, M);
    nd_index SecondLastIdx = SetDimension(MinIdx, 0, X2);
    nd_index LastIdx       = SetDimension(MinIdx, 0, X1);
    nd_index ThirdLastIdx  = SetDimension(MinIdx, 0, X3);
    /* inverse update (excluding last odd position) */
    if (Option != lift_option::NoUpdate)
    {
      for (auto X = P[0] + S[0]; X < X2; X += 2 * S[0])
      {
        nd_index MiddleIdx = SetDimension(MinIdx, 0, X);
        nd_index LeftIdx   = SetDimension(MinIdx, 0, X - S[0]);
        nd_index RightIdx  = SetDimension(MinIdx, 0, X + S[0]);
        t Val = F[LinearIndex(MiddleIdx, N)];
        F[LinearIndex(LeftIdx, N)] -= Val / 4;
        F[LinearIndex(RightIdx, N)] -= Val / 4;
      }
      if (!SignalIsEven)
      { /* no extrapolation, inverse update at the last odd position */
        t Val = F[LinearIndex(SecondLastIdx, N)];
        F[LinearIndex(ThirdLastIdx, N)] -= Val / 4;
        if (Option == lift_option::Normal)
          F[LinearIndex(LastIdx, N)] -= Val / 4;
      }
      else
      { /* extrapolation, need to "fix" the last position (odd) */
        nd_index ExtrapolateIdx = SetDimension(MinIdx, 0, X0);
        t A = F[LinearIndex(ExtrapolateIdx, N)];
        t B = F[LinearIndex(SecondLastIdx, N)];
        F[LinearIndex(LastIdx, N)] = (A + B) / 2;
      }
    }
    /* inverse predict (excluding last odd position) */
    for (auto X = P[0] + S[0]; X < X2; X += 2 * S[0])
    {
      nd_index MiddleIdx = SetDimension(MinIdx, 0, X);
      nd_index LeftIdx   = SetDimension(MinIdx, 0, X - S[0]);
      nd_index RightIdx  = SetDimension(MinIdx, 0, X + S[0]);
      t& Val = F[LinearIndex(MiddleIdx, N)];
      Val += (F[LinearIndex(LeftIdx, N)] + F[LinearIndex(RightIdx, N)]) / 2;
    }
    if (!SignalIsEven)
    { /* no extrapolation, inverse predict at the last odd position */
      t& Val = F[LinearIndex(SecondLastIdx, N)];
      Val += (F[LinearIndex(LastIdx, N)] + F[LinearIndex(ThirdLastIdx, N)]) / 2;
    }
  });
}


//idx2_ILiftCdf53(Z, Y, X) // X inverse lifting




/*
Extrapolate a volume to (2^N+1) x (2^N+1) x (2^N+1).
Dims3 are the dimensions of the volume, not Dims(*Vol), which has to be (2^X+1) x (2^Y+1) x (2^Z+1).
*/
//static void
//ExtrapolateCdf53(const v3i& Dims3, u64 TransformOrder, volume* Vol)
//{
//  v3i N3 = Dims(*Vol);
//  v3i M3(N3.X == 1 ? 1 : N3.X - 1, N3.Y == 1 ? 1 : N3.Y - 1, N3.Z == 1 ? 1 : N3.Z - 1);
//  // printf("M3 = " idx2_PrStrV3i "\n", idx2_PrV3i(M3));
//  idx2_Assert(IsPow2(M3.X) && IsPow2(M3.Y) && IsPow2(M3.Z));
//  int NLevels = Log2Floor(Max(Max(N3.X, N3.Y), N3.Z));
//  ForwardCdf53(Dims3, M3, 0, NLevels, TransformOrder, Vol);
//  InverseCdf53(N3, M3, 0, NLevels, TransformOrder, Vol);
//}


static void
ForwardCdf53(const v3i& M3,
             int Iter,
             const array<subband>& Subbands,
             const transform_info_v2& TransformInfo,
             volume* Vol,
             bool CoarsestLevel)
{
//  idx2_For (int, I, 0, Size(TransformInfo.Axes))
//  {
//    int D = TransformInfo.Axes[I];
//    if (Vol->Type == dtype::float64)
//    {
//      if (D == 0)
//        FLiftCdf53X<f64>(TransformInfo.Grids[I], M3, lift_option::Normal, Vol);
//      else if (D == 1)
//        FLiftCdf53Y<f64>(TransformInfo.Grids[I], M3, lift_option::Normal, Vol);
//      else if (D == 2)
//        FLiftCdf53Z<f64>(TransformInfo.Grids[I], M3, lift_option::Normal, Vol);
//      // TODO: need higher dimensional transforms
//    }
//  }
//
//  /* Optionally normalize */
//  idx2_Assert(IsFloatingPoint(Vol->Type));
//  for (int I = 0; I < Size(Subbands); ++I)
//  {
//    if (I == 0 && !CoarsestLevel)
//      continue; // do not normalize subband 0
//    subband& S = Subbands[I];
//    f64 Wx = M3.X == 1 ? 1
//                       : (S.LowHigh3.X == 0
//                            ? TransformInfo.BasisNorms
//                                .ScalNorms[Iter * TransformInfo.NLevels + S.Level3Rev.X - 1]
//                            : TransformInfo.BasisNorms
//                                .WaveNorms[Iter * TransformInfo.NLevels + S.Level3Rev.X]);
//    f64 Wy = M3.Y == 1 ? 1
//                       : (S.LowHigh3.Y == 0
//                            ? TransformInfo.BasisNorms
//                                .ScalNorms[Iter * TransformInfo.NLevels + S.Level3Rev.Y - 1]
//                            : TransformInfo.BasisNorms
//                                .WaveNorms[Iter * TransformInfo.NLevels + S.Level3Rev.Y]);
//    f64 Idx2 = M3.Z == 1 ? 1
//                         : (S.LowHigh3.Z == 0
//                              ? TransformInfo.BasisNorms
//                                  .ScalNorms[Iter * TransformInfo.NLevels + S.Level3Rev.Z - 1]
//                              : TransformInfo.BasisNorms
//                                  .WaveNorms[Iter * TransformInfo.NLevels + S.Level3Rev.Z]);
//    f64 W = Wx * Wy * Idx2;
//#define Body(type)                                                                                 \
//  auto ItEnd = End<type>(S.Grid, *Vol);                                                            \
//  for (auto It = Begin<type>(S.Grid, *Vol); It != ItEnd; ++It)                                     \
//    *It = type(*It * W);
//    idx2_DispatchOnType(Vol->Type);
//#undef Body
//  }
}


/* The reason we need to know if the input is on the coarsest level is because we do not want
to normalize subband 0 otherwise */
//static void
//InverseCdf53(const v3i& M3,
//             int Iter,
//             const array<subband>& Subbands,
//             const transform_info& TransformDetails,
//             volume* Vol,
//             bool CoarsestLevel)
//{
//  /* inverse normalize if required */
//  idx2_Assert(IsFloatingPoint(Vol->Type));
//  for (int I = 0; I < Size(Subbands); ++I)
//  {
//    if (I == 0 && !CoarsestLevel)
//      continue; // do not normalize subband 0
//    subband& S = Subbands[I];
//    f64 Wx = M3.X == 1 ? 1
//                       : (S.LowHigh3.X == 0
//                            ? TransformDetails.BasisNorms
//                                .ScalNorms[Iter * TransformDetails.NPasses + S.Level3Rev.X - 1]
//                            : TransformDetails.BasisNorms
//                                .WaveNorms[Iter * TransformDetails.NPasses + S.Level3Rev.X]);
//    f64 Wy = M3.Y == 1 ? 1
//                       : (S.LowHigh3.Y == 0
//                            ? TransformDetails.BasisNorms
//                                .ScalNorms[Iter * TransformDetails.NPasses + S.Level3Rev.Y - 1]
//                            : TransformDetails.BasisNorms
//                                .WaveNorms[Iter * TransformDetails.NPasses + S.Level3Rev.Y]);
//    f64 Idx2 = M3.Z == 1 ? 1
//                         : (S.LowHigh3.Z == 0
//                              ? TransformDetails.BasisNorms
//                                  .ScalNorms[Iter * TransformDetails.NPasses + S.Level3Rev.Z - 1]
//                              : TransformDetails.BasisNorms
//                                  .WaveNorms[Iter * TransformDetails.NPasses + S.Level3Rev.Z]);
//    f64 W = 1.0 / (Wx * Wy * Idx2);
//#define Body(type)                                                                                 \
//  auto ItEnd = End<type>(S.Grid, *Vol);                                                            \
//  for (auto It = Begin<type>(S.Grid, *Vol); It != ItEnd; ++It)                                     \
//    *It = type(*It * W);
//    idx2_DispatchOnType(Vol->Type);
//#undef Body
//  }
//
//  /* perform the inverse transform */
//  int I = TransformDetails.StackSize;
//  while (I-- > 0)
//  {
//    int D = TransformDetails.StackAxes[I];
//#define Body(type)                                                                                 \
//  switch (D)                                                                                       \
//  {                                                                                                \
//    case 0:                                                                                        \
//      ILiftCdf53X<f64>(TransformDetails.StackGrids[I], M3, lift_option::Normal, Vol);              \
//      break;                                                                                       \
//    case 1:                                                                                        \
//      ILiftCdf53Y<f64>(TransformDetails.StackGrids[I], M3, lift_option::Normal, Vol);              \
//      break;                                                                                       \
//    case 2:                                                                                        \
//      ILiftCdf53Z<f64>(TransformDetails.StackGrids[I], M3, lift_option::Normal, Vol);              \
//      break;                                                                                       \
//    default:                                                                                       \
//      idx2_Assert(false);                                                                          \
//      break;                                                                                       \
//  };
//    idx2_DispatchOnType(Vol->Type);
//#undef Body
//  }
//}


} // namespace idx2
