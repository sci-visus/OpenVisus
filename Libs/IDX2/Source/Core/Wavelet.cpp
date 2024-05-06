#include "Wavelet.h"
#include "Algorithm.h"
#include "Array.h"
#include "Assert.h"
#include "BitOps.h"
#include "Common.h"
#include "DataTypes.h"
#include "Function.h"
#include "Logger.h"
#include "Math.h"
#include "Memory.h"
#include "ScopeGuard.h"
#include "CircularQueue.h"


namespace idx2
{


void
ComputeTransformDetails(transform_info* Td, const v3i& Dims3, int NPasses, u64 TformOrder)
{
  int Pass = 0;
  u64 PrevOrder = TformOrder;
  v3i D3 = Dims3;
  v3i R3 = D3;
  v3i S3(1);
  grid G(Dims3);
  int StackSize = 0;
  while (Pass < NPasses)
  {
    idx2_Assert(TformOrder != 0);
    int D = TformOrder & 0x3;
    TformOrder >>= 2;
    if (D == 3)
    {                      // next level
      if (TformOrder == 3) // next one is the last |
        TformOrder = PrevOrder;
      else
        PrevOrder = TformOrder;
      SetStrd(&G, S3);
      SetDims(&G, D3);
      R3 = D3;
      ++Pass;
    }
    else
    {
      Td->StackGrids[StackSize] = G;
      Td->StackAxes[StackSize++] = D;
      R3[D] = D3[D] + IsEven(D3[D]);
      SetDims(&G, R3);
      D3[D] = (R3[D] + 1) >> 1;
      S3[D] <<= 1;
    }
  }
  Td->TformOrder = TformOrder;
  Td->StackSize = StackSize;
  Td->BasisNorms = GetCdf53NormsFast<16>();
  Td->NPasses = NPasses;
}


/*
Extrapolate a volume to (2^N+1) x (2^N+1) x (2^N+1).
Dims3 are the dimensions of the volume, not Dims(*Vol), which has to be (2^X+1) x (2^Y+1) x (2^Z+1).
*/
void
ExtrapolateCdf53(const v3i& Dims3, u64 TransformOrder, volume* Vol)
{
  v3i N3 = Dims(*Vol);
  v3i M3(N3.X == 1 ? 1 : N3.X - 1, N3.Y == 1 ? 1 : N3.Y - 1, N3.Z == 1 ? 1 : N3.Z - 1);
  // printf("M3 = " idx2_PrStrV3i "\n", idx2_PrV3i(M3));
  idx2_Assert(IsPow2(M3.X) && IsPow2(M3.Y) && IsPow2(M3.Z));
  int NLevels = Log2Floor(Max(Max(N3.X, N3.Y), N3.Z));
  ForwardCdf53(Dims3, M3, 0, NLevels, TransformOrder, Vol);
  InverseCdf53(N3, M3, 0, NLevels, TransformOrder, Vol);
}


void
ForwardCdf53(const v3i& M3,
             int Iter,
             const array<subband>& Subbands,
             const transform_info& TransformDetails,
             volume* Vol,
             bool CoarsestLevel)
{
  idx2_For (int, I, 0, TransformDetails.StackSize)
  {
    int D = TransformDetails.StackAxes[I];
#define Body(type)                                                                                 \
  switch (D)                                                                                       \
  {                                                                                                \
    case 0:                                                                                        \
      FLiftCdf53X<type>(TransformDetails.StackGrids[I], M3, lift_option::Normal, Vol);                           \
      break;                                                                                       \
    case 1:                                                                                        \
      FLiftCdf53Y<type>(TransformDetails.StackGrids[I], M3, lift_option::Normal, Vol);                           \
      break;                                                                                       \
    case 2:                                                                                        \
      FLiftCdf53Z<type>(TransformDetails.StackGrids[I], M3, lift_option::Normal, Vol);                           \
      break;                                                                                       \
    default:                                                                                       \
      idx2_Assert(false);                                                                          \
      break;                                                                                       \
  };
    idx2_DispatchOnType(Vol->Type);
#undef Body
  }

  /* Optionally normalize */
  idx2_Assert(IsFloatingPoint(Vol->Type));
  for (int I = 0; I < Size(Subbands); ++I)
  {
    if (I == 0 && !CoarsestLevel)
      continue; // do not normalize subband 0
    subband& S = Subbands[I];
    f64 Wx = M3.X == 1
               ? 1
               : (S.LowHigh3.X == 0 ? TransformDetails.BasisNorms.ScalNorms[Iter * TransformDetails.NPasses + S.Level3Rev.X - 1]
                                    : TransformDetails.BasisNorms.WaveNorms[Iter * TransformDetails.NPasses + S.Level3Rev.X]);
    f64 Wy = M3.Y == 1
               ? 1
               : (S.LowHigh3.Y == 0 ? TransformDetails.BasisNorms.ScalNorms[Iter * TransformDetails.NPasses + S.Level3Rev.Y - 1]
                                    : TransformDetails.BasisNorms.WaveNorms[Iter * TransformDetails.NPasses + S.Level3Rev.Y]);
    f64 Idx2 = M3.Z == 1 ? 1
                         : (S.LowHigh3.Z == 0
                              ? TransformDetails.BasisNorms.ScalNorms[Iter * TransformDetails.NPasses + S.Level3Rev.Z - 1]
                              : TransformDetails.BasisNorms.WaveNorms[Iter * TransformDetails.NPasses + S.Level3Rev.Z]);
    f64 W = Wx * Wy * Idx2;
#define Body(type)                                                                                 \
  auto ItEnd = End<type>(S.Grid, *Vol);                                                            \
  for (auto It = Begin<type>(S.Grid, *Vol); It != ItEnd; ++It)                                     \
    *It = type(*It * W);
    idx2_DispatchOnType(Vol->Type);
#undef Body
  }
}


/* The reason we need to know if the input is on the coarsest level is because we do not want
to normalize subband 0 otherwise */
void
InverseCdf53(const v3i& M3,
             int Iter,
             const array<subband>& Subbands,
             const transform_info& TransformDetails,
             volume* Vol,
             bool CoarsestLevel)
{
  /* inverse normalize if required */
  idx2_Assert(IsFloatingPoint(Vol->Type));
  for (int I = 0; I < Size(Subbands); ++I)
  {
    if (I == 0 && !CoarsestLevel)
      continue; // do not normalize subband 0
    subband& S = Subbands[I];
    f64 Wx = M3.X == 1
               ? 1
               : (S.LowHigh3.X == 0 ? TransformDetails.BasisNorms.ScalNorms[Iter * TransformDetails.NPasses + S.Level3Rev.X - 1]
                                    : TransformDetails.BasisNorms.WaveNorms[Iter * TransformDetails.NPasses + S.Level3Rev.X]);
    f64 Wy = M3.Y == 1
               ? 1
               : (S.LowHigh3.Y == 0 ? TransformDetails.BasisNorms.ScalNorms[Iter * TransformDetails.NPasses + S.Level3Rev.Y - 1]
                                    : TransformDetails.BasisNorms.WaveNorms[Iter * TransformDetails.NPasses + S.Level3Rev.Y]);
    f64 Idx2 = M3.Z == 1 ? 1
                         : (S.LowHigh3.Z == 0
                              ? TransformDetails.BasisNorms.ScalNorms[Iter * TransformDetails.NPasses + S.Level3Rev.Z - 1]
                              : TransformDetails.BasisNorms.WaveNorms[Iter * TransformDetails.NPasses + S.Level3Rev.Z]);
    f64 W = 1.0 / (Wx * Wy * Idx2);
#define Body(type)                                                                                 \
  auto ItEnd = End<type>(S.Grid, *Vol);                                                            \
  for (auto It = Begin<type>(S.Grid, *Vol); It != ItEnd; ++It)                                     \
    *It = type(*It * W);
    idx2_DispatchOnType(Vol->Type);
#undef Body
  }

  /* perform the inverse transform */
  int I = TransformDetails.StackSize;
  while (I-- > 0)
  {
    int D = TransformDetails.StackAxes[I];
#define Body(type)                                                                                 \
  switch (D)                                                                                       \
  {                                                                                                \
    case 0:                                                                                        \
      ILiftCdf53X<f64>(TransformDetails.StackGrids[I], M3, lift_option::Normal, Vol);                            \
      break;                                                                                       \
    case 1:                                                                                        \
      ILiftCdf53Y<f64>(TransformDetails.StackGrids[I], M3, lift_option::Normal, Vol);                            \
      break;                                                                                       \
    case 2:                                                                                        \
      ILiftCdf53Z<f64>(TransformDetails.StackGrids[I], M3, lift_option::Normal, Vol);                            \
      break;                                                                                       \
    default:                                                                                       \
      idx2_Assert(false);                                                                          \
      break;                                                                                       \
  };
    idx2_DispatchOnType(Vol->Type);
#undef Body
  }
}

/* With extrapolation (if needed). Support custom transform order. */
// TODO: as currently implemented, the behavior of this is different from the BuildSubbands()
// routine when given a transform order such as XXY++ (BuildSubbands will restrict the second X to
// the coarse subband produced by the first X, while this function will not).
void
ForwardCdf53(const v3i& Dims3,
             const v3i& M3,
             int Iter,
             int NLevels,
             u64 TformOrder,
             volume* Vol,
             bool Normalize)
{
  idx2_Assert(Dims3 <= Dims(*Vol));
  int Level = 0;
  u64 PrevOrder = TformOrder;
  v3i D3 = Dims3;
  v3i R3 = D3;
  v3i S3(1);
  grid G(Dims3);
  while (Level < NLevels)
  {
    idx2_Assert(TformOrder != 0);
    int D = TformOrder & 0x3;
    TformOrder >>= 2;
    if (D == 3)
    {                      // next level
      if (TformOrder == 3) // next one is the last |
        TformOrder = PrevOrder;
      else
        PrevOrder = TformOrder;
      SetStrd(&G, S3);
      SetDims(&G, D3);
      R3 = D3;
      ++Level;
    }
    else
    {
#define Body(type)                                                                                 \
  switch (D)                                                                                       \
  {                                                                                                \
    case 0:                                                                                        \
      idx2_Assert(Dims3.X > 1);                                                                    \
      FLiftCdf53X<type>(G, M3, lift_option::Normal, Vol);                                          \
      break;                                                                                       \
    case 1:                                                                                        \
      idx2_Assert(Dims3.Y > 1);                                                                    \
      FLiftCdf53Y<type>(G, M3, lift_option::Normal, Vol);                                          \
      break;                                                                                       \
    case 2:                                                                                        \
      idx2_Assert(Dims3.Z > 1);                                                                    \
      FLiftCdf53Z<type>(G, M3, lift_option::Normal, Vol);                                          \
      break;                                                                                       \
    default:                                                                                       \
      idx2_Assert(false);                                                                          \
      break;                                                                                       \
  };
      idx2_DispatchOnType(Vol->Type);
#undef Body
      R3[D] = D3[D] + IsEven(D3[D]);
      SetDims(&G, R3);
      D3[D] = (R3[D] + 1) >> 1;
      S3[D] <<= 1;
    }
  }

  /* Optionally normalize */
  if (NLevels > 0 && Normalize)
  {
    idx2_Assert(IsFloatingPoint(Vol->Type));
    idx2_RAII(array<subband>, Subbands, BuildSubbands(Dims3, NLevels, TformOrder, &Subbands));
    auto BasisNorms = GetCdf53NormsFast<16>();
    for (int I = 0; I < Size(Subbands); ++I)
    {
      subband& S = Subbands[I];
      f64 Wx = Dims3.X == 1
                 ? 1
                 : (S.LowHigh3.X == 0 ? BasisNorms.ScalNorms[Iter * NLevels + S.Level3Rev.X - 1]
                                      : BasisNorms.WaveNorms[Iter * NLevels + S.Level3Rev.X]);
      f64 Wy = Dims3.Y == 1
                 ? 1
                 : (S.LowHigh3.Y == 0 ? BasisNorms.ScalNorms[Iter * NLevels + S.Level3Rev.Y - 1]
                                      : BasisNorms.WaveNorms[Iter * NLevels + S.Level3Rev.Y]);
      f64 Idx2 = Dims3.Z == 1
                   ? 1
                   : (S.LowHigh3.Z == 0 ? BasisNorms.ScalNorms[Iter * NLevels + S.Level3Rev.Z - 1]
                                        : BasisNorms.WaveNorms[Iter * NLevels + S.Level3Rev.Z]);
      f64 W = Wx * Wy * Idx2;
#define Body(type)                                                                                 \
  auto ItEnd = End<type>(S.Grid, *Vol);                                                            \
  for (auto It = Begin<type>(S.Grid, *Vol); It != ItEnd; ++It)                                     \
    *It = type(*It * W);
      idx2_DispatchOnType(Vol->Type);
#undef Body
    }
  }
}

void
InverseCdf53(const v3i& Dims3,
             const v3i& M3,
             int Iter,
             int NLevels,
             u64 TformOrder,
             volume* Vol,
             bool Normalize)
{
  idx2_Assert(Dims3 <= Dims(*Vol));
  /* "simulate" the forward transform */
  stack_array<grid, 32> StackGrids;
  stack_array<int, 32> StackAxes;
  int Level = 0;
  u64 PrevOrder = TformOrder;
  v3i D3 = Dims3;
  v3i R3 = D3;
  v3i S3(1);
  grid G(Dims3);
  int Iteration = 0;
  while (Level < NLevels)
  {
    idx2_Assert(TformOrder != 0);
    int D = TformOrder & 0x3;
    TformOrder >>= 2;
    if (D == 3)
    {                      // next level
      if (TformOrder == 3) // next one is the last |
        TformOrder = PrevOrder;
      else
        PrevOrder = TformOrder;
      SetStrd(&G, S3);
      SetDims(&G, D3);
      R3 = D3;
      ++Level;
    }
    else
    {
      StackGrids[Iteration] = G;
      StackAxes[Iteration++] = D;
      R3[D] = D3[D] + IsEven(D3[D]);
      SetDims(&G, R3);
      D3[D] = (R3[D] + 1) >> 1;
      S3[D] <<= 1;
    }
  }

  /* inverse normalize if required */
  if (NLevels > 0 && Normalize)
  {
    idx2_Assert(IsFloatingPoint(Vol->Type));
    idx2_RAII(array<subband>, Subbands, BuildSubbands(Dims3, NLevels, TformOrder, &Subbands));
    auto BasisNorms = GetCdf53NormsFast<16>();
    for (int I = 0; I < Size(Subbands); ++I)
    {
      subband& S = Subbands[I];
      f64 Wx = Dims3.X == 1
                 ? 1
                 : (S.LowHigh3.X == 0 ? BasisNorms.ScalNorms[Iter * NLevels + S.Level3Rev.X - 1]
                                      : BasisNorms.WaveNorms[Iter * NLevels + S.Level3Rev.X]);
      f64 Wy = Dims3.Y == 1
                 ? 1
                 : (S.LowHigh3.Y == 0 ? BasisNorms.ScalNorms[Iter * NLevels + S.Level3Rev.Y - 1]
                                      : BasisNorms.WaveNorms[Iter * NLevels + S.Level3Rev.Y]);
      f64 Idx2 = Dims3.Z == 1
                   ? 1
                   : (S.LowHigh3.Z == 0 ? BasisNorms.ScalNorms[Iter * NLevels + S.Level3Rev.Z - 1]
                                        : BasisNorms.WaveNorms[Iter * NLevels + S.Level3Rev.Z]);
      f64 W = 1.0 / (Wx * Wy * Idx2);
#define Body(type)                                                                                 \
  auto ItEnd = End<type>(S.Grid, *Vol);                                                            \
  for (auto It = Begin<type>(S.Grid, *Vol); It != ItEnd; ++It)                                     \
    *It = type(*It * W);
      idx2_DispatchOnType(Vol->Type);
#undef Body
    }
  }

  /* perform the inverse transform */
  while (Iteration-- > 0)
  {
    int D = StackAxes[Iteration];
#define Body(type)                                                                                 \
  switch (D)                                                                                       \
  {                                                                                                \
    case 0:                                                                                        \
      ILiftCdf53X<f64>(StackGrids[Iteration], M3, lift_option::Normal, Vol);                       \
      break;                                                                                       \
    case 1:                                                                                        \
      ILiftCdf53Y<f64>(StackGrids[Iteration], M3, lift_option::Normal, Vol);                       \
      break;                                                                                       \
    case 2:                                                                                        \
      ILiftCdf53Z<f64>(StackGrids[Iteration], M3, lift_option::Normal, Vol);                       \
      break;                                                                                       \
    default:                                                                                       \
      idx2_Assert(false);                                                                          \
      break;                                                                                       \
  };
    idx2_DispatchOnType(Vol->Type);
#undef Body
  }
}

u64
EncodeTransformOrder(const stref& TransformOrder)
{
  u64 Result = 0;
  int Len = Size(TransformOrder);
  for (int I = Len - 1; I >= 0; --I)
  {
    char C = TransformOrder[I];
    idx2_Assert(C == 'X' || C == 'Y' || C == 'Z' || C == '+');
    int T = C == '+' ? 3 : C - 'X';
    Result = (Result << 2) + T;
  }
  return Result;
}


void
DecodeTransformOrder(u64 Input, str Output)
{
  int Len = 0;
  while (Input != 0)
  {
    int T = Input & 0x3;
    Output[Len++] = T == 3 ? '+' : char('X' + T);
    Input >>= 2;
  }
  Output[Len] = '\0';
}

i8
DecodeTransformOrder(u64 Input, v3i N3, str Output)
{
  N3 = v3i((int)NextPow2(N3.X), (int)NextPow2(N3.Y), (int)NextPow2(N3.Z));
  i8 Len = 0;
  u64 SavedInput = Input;
  while (Prod<u64>(N3) > 1)
  {
    int T = Input & 0x3;
    Input >>= 2;
    if (T == 3)
    {
      if (Input == 3)
        Input = SavedInput;
      else
        SavedInput = Input;
    }
    else
    {
      Output[Len++] = char('X' + T);
      N3[T] >>= 1;
    }
  }
  Output[Len] = '\0';
  return Len;
}


i8
DecodeTransformOrder(u64 Input, int Passes, str Output)
{
  i8 Len = 0;
  u64 SavedInput = Input;
  v3i Passes3(Passes);
  while (true)
  {
    int T = Input & 0x3;
    Input >>= 2;
    if (T == 3)
    {
      if (Input == 3)
        Input = SavedInput;
      else
        SavedInput = Input;
    }
    else
    {
      --Passes3[T];
      if (Passes3[T] < 0)
        break;
      Output[Len++] = char('X' + T);
    }
  }
  Output[Len] = '\0';
  return Len;
}



/*
In string form, a TransformOrder is made from 4 characters: X,Y,Z, and +
X, Y, and Z denotes the axis where the transform happens, while + denotes where the next
level begins (any subsequent transform will be done on the coarsest level subband only).
Two consecutive ++ signifies the end of the string. If the end is reached before the number of
levels, the last order gets applied.
In integral form, TransformOrder = T encodes the axis order of the transform in base-4 integers.
T % 4 = D in [0, 3] where 0 = X, 1 = Y, 2 = Z, 3 = +*/
void
BuildSubbands(const v3i& N3, int NLevels, u64 TransformOrder, array<subband>* Subbands)
{
  Clear(Subbands);
  Reserve(Subbands, ((1 << NumDims(N3)) - 1) * NLevels + 1);
  circular_queue<subband, 256> Queue;
  PushBack(&Queue, subband{ grid(N3), grid(N3), v3<i8>(0), v3<i8>(0), v3<i8>(0), i8(0) });
  i8 Level = 0;
  u64 PrevOrder = TransformOrder;
  v3<i8> MaxLevel3(0);
  stack_array<grid, 32> Grids;
  while (Level < NLevels)
  {
    idx2_Assert(TransformOrder != 0);
    int D = TransformOrder & 0x3;
    TransformOrder >>= 2;
    if (D == 3)
    {                          // next level
      if (TransformOrder == 3) // next one is the last +
        TransformOrder = PrevOrder;
      else
        PrevOrder = TransformOrder;
      i16 Sz = Size(Queue);
      for (i16 I = Sz - 1; I >= 1; --I)
      {
        PushBack(Subbands, Queue[I]);
        PopBack(&Queue);
      }
      ++Level;
      Grids[Level] = Queue[0].Grid;
    }
    else
    {
      ++MaxLevel3[D];
      i16 Sz = Size(Queue);
      for (i16 I = 0; I < Sz; ++I)
      {
        const grid& G = Queue[0].Grid;
        grid_split Gs = SplitAlternate(G, dimension(D));
        v3<i8> NextLevel3 = Queue[0].Level3;
        ++NextLevel3[D];
        v3<i8> NextLowHigh3 = Queue[0].LowHigh3;
        idx2_Assert(NextLowHigh3[D] == 0);
        NextLowHigh3[D] = 1;
        PushBack(&Queue,
                 subband{ Gs.First, Gs.First, NextLevel3, NextLevel3, Queue[0].LowHigh3, Level });
        PushBack(
          &Queue,
          subband{ Gs.Second, Gs.Second, Queue[0].Level3, Queue[0].Level3, NextLowHigh3, Level });
        PopFront(&Queue);
      }
    }
  }
  if (Size(Queue) > 0)
    PushBack(Subbands, Queue[0]);
  Reverse(Begin(Grids), Begin(Grids) + Level + 1);
  i64 Sz = Size(*Subbands);
  for (i64 I = 0; I < Sz; ++I)
  {
    subband& Sband = (*Subbands)[I];
    Sband.Level3 = MaxLevel3 - Sband.Level3Rev;
    Sband.Level = i8(NLevels - Sband.Level - 1);
    Sband.AccumGrid = MergeSubbandGrids(Grids[Sband.Level], Sband.Grid);
  }
  Reverse(Begin(*Subbands), End(*Subbands));
}


grid
MergeSubbandGrids(const grid& Sb1, const grid& Sb2)
{
  v3i Off3 = Abs(From(Sb2) - From(Sb1));
  v3i Strd3 = Min(Strd(Sb1), Strd(Sb2)) * Equals(Off3, v3i(0)) + Off3;
  // TODO: works in case of subbands but not in general
  v3i Dims3 = Dims(Sb1) + NotEquals(From(Sb1), From(Sb2)) * Dims(Sb2);
  v3i From3 = Min(From(Sb2), From(Sb1));
  return grid(From3, Dims3, Strd3);
}


} // namespace idx2

