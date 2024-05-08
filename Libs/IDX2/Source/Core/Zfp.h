/* Adapted from the zfp compression library */

#pragma once

#include "Algorithm.h"
#include "Assert.h"
#include "BitOps.h"
#include "BitStream.h"
#include "Common.h"
#include "Macros.h"
#include "Math.h"
#include <immintrin.h>
//#include <iostream>


namespace idx2
{


extern const v3i ZDims; /* 4 x 4 x 4 */

/* Forward/inverse zfp lifting in 1D */
template <typename t> void
FLift(t* P, int S);

template <typename t> void
ILift(t* P, int S);


/* zfp transform in 3D. The input is assumed to be in row-major order. */
template <typename t> void
ForwardZfp(t* P);

template <typename t> void
ForwardZfp(t* P, int D);

template <typename t> void
InverseZfp(t* P);

template <typename t> void
InverseZfp(t* P, int D);

template <typename t, int S> void
ForwardZfp2D(t* P); // TODO: for now this works only for S = 4

template <typename t, int S> void
InverseZfp2D(t* P); // TODO: for now this works only for S = 4


/* Reorder coefficients within a zfp block, and convert them from/to negabinary */
template <typename t, typename u> void
ForwardShuffle(t* IBlock, u* UBlock);

template <typename t, typename u> void
InverseShuffle(u* UBlock, t* IBlock);

template <typename t, typename u> void
ForwardShuffle(t* IBlock, u* UBlock, int D);

template <typename t, typename u> void
InverseShuffle(u* UBlock, t* IBlock, int D);

template <typename t, typename u, int S> void
ForwardShuffle2D(t* IBlock, u* UBlock);

template <typename t, typename u, int S> void
InverseShuffle2D(u* UBlock, t* IBlock);


/* Pad partial block of width N < 4 and stride S */
template <typename t> void
PadBlock1D(t* P, int N, int S);

template <typename t> void
PadBlock2D(t* P, const v2i& N);

template <typename t> void
PadBlock3D(t* P, const v3i& N);


template <typename t> void
Encode(t* Block, int NVals, int B, i8& N, bitstream* Bs);

template <typename t> void
Decode(t* Block, int NVals, int B, i8& N, bitstream* Bs);


struct bitstream;

/* Encode/decode a single bit plane B of a zfp block */
// TODO: turn this into a template? TODO: pointer aliasing?
/*
B = the bit plane to encode
S = maximum number of bits to encode in the current pass
N keeps track of the number of coefficients that have previously become significant
Sometimes the stream is interrupted in the middle of a bit plane, so M keeps track
of the number of significant coefficients encoded in the current bit plane
*/
bool
Encode(u64* Block, int B, i64 S, i8& N, i8& M, bool& In, bitstream* Bs);

bool
Decode(u64* Block, int B, i64 S, i8& N, i8& M, bool& In, bitstream* Bs);

bool
Encode(u64* Block, int B, i64 S, i8& N, i8& M, bitstream* Bs);

bool
Decode(u64* Block, int B, i64 S, i8& N, i8& M, bitstream* Bs);

template <typename t, int D = 3, int K = 4> void
Encode(t* Block, int B, i64 S, i8& N, bitstream* Bs);

template <typename t, int D = 3, int K = 4> void
Decode(t* Block, int B, i64 S, i8& N, bitstream* Bs);

template <typename t, int D = 3, int K = 4> void
Decode2(t* Block, int B, i64 S, i8& N, bitstream* Bs);

template <typename t, int D = 3, int K = 4> void
Decode3(t* Block, int B, i64 S, i8& N, bitstream* Bs);

template <typename t, int D = 3, int K = 4> void
Decode4(t* Block, int B, i64 S, i8& N, bitstream* Bs);


} // namespace idx2



namespace idx2
{


/*
zfp lifting transform for 4 samples in 1D.
 non-orthogonal transform
        ( 4  4  4  4) (X)
 1/16 * ( 5  1 -1 -5) (Y)
        (-4  4  4 -4) (Z)
        (-2  6 -6  2) (W)
*/
// TODO: look into range expansion for this transform
template <typename t> idx2_Inline void
FLift(t* P, int S)
{
  idx2_Assert(P);
  idx2_Assert(S > 0);
  t X = P[0 * S], Y = P[1 * S], Z = P[2 * S], W = P[3 * S];
  X += W;
  X >>= 1;
  W -= X;
  Z += Y;
  Z >>= 1;
  Y -= Z;
  X += Z;
  X >>= 1;
  Z -= X;
  W += Y;
  W >>= 1;
  Y -= W;
  W += Y >> 1;
  Y -= W >> 1;
  P[0 * S] = X;
  P[1 * S] = Y;
  P[2 * S] = Z;
  P[3 * S] = W;
}


/*
zfp inverse lifting transform for 4 samples in 1D.
NOTE: this lifting is not perfectly reversible
 non-orthogonal transform
       ( 4  6 -4 -1) (x)
 1/4 * ( 4  2  4  5) (y)
       ( 4 -2  4 -5) (z)
       ( 4 -6 -4  1) (w)
*/
template <typename t> idx2_Inline void
ILift(t* P, int S)
{
  idx2_Assert(P);
  idx2_Assert(S > 0);
  t X = P[0 * S], Y = P[1 * S], Z = P[2 * S], W = P[3 * S];
  Y += W >> 1;
  W -= Y >> 1;
  Y += W;
  W <<= 1;
  W -= Y;
  Z += X;
  X <<= 1;
  X -= Z;
  Y += Z;
  Z <<= 1;
  Z -= Y;
  W += X;
  X <<= 1;
  X -= W;
  P[0 * S] = X;
  P[1 * S] = Y;
  P[2 * S] = Z;
  P[3 * S] = W;
}


template <typename t> void
ForwardZfp(t* P)
{
  idx2_Assert(P);
  /* transform along X */
  for (int Z = 0; Z < 4; ++Z)
    for (int Y = 0; Y < 4; ++Y)
      FLift(P + 4 * Y + 16 * Z, 1);
  /* transform along Y */
  for (int X = 0; X < 4; ++X)
    for (int Z = 0; Z < 4; ++Z)
      FLift(P + 16 * Z + 1 * X, 4);
  /* transform along Z */
  for (int Y = 0; Y < 4; ++Y)
    for (int X = 0; X < 4; ++X)
      FLift(P + 1 * X + 4 * Y, 16);
}


template <typename t, int D> void
ForwardZfpNew(t* P)
{
  idx2_Assert(P);
  if constexpr (D == 3)
  {
    /* transform along X */
    for (int Z = 0; Z < 4; ++Z)
      for (int Y = 0; Y < 4; ++Y)
        FLift(P + 4 * Y + 16 * Z, 1);
    /* transform along Y */
    for (int X = 0; X < 4; ++X)
      for (int Z = 0; Z < 4; ++Z)
        FLift(P + 16 * Z + 1 * X, 4);
    /* transform along Z */
    for (int Y = 0; Y < 4; ++Y)
      for (int X = 0; X < 4; ++X)
        FLift(P + 1 * X + 4 * Y, 16);
  }
  else if constexpr (D == 2)
  {
    /* transform along X */
    for (int Y = 0; Y < 4; ++Y)
      FLift(P + 4 * Y, 1);
    /* transform along Y */
    for (int X = 0; X < 4; ++X)
      FLift(P + 1 * X, 4);
  }
  else
  {
    FLift(P, 1);
  }
}


template <typename t> idx2_Inline void
ForwardZfp(t* P, int D)
{
  idx2_Assert(P);
  switch (D)
  {
    case 3:
      /* transform along X */
      for (int Z = 0; Z < 4; ++Z)
        for (int Y = 0; Y < 4; ++Y)
          FLift(P + 4 * Y + 16 * Z, 1);
      /* transform along Y */
      for (int X = 0; X < 4; ++X)
        for (int Z = 0; Z < 4; ++Z)
          FLift(P + 16 * Z + 1 * X, 4);
      /* transform along Z */
      for (int Y = 0; Y < 4; ++Y)
        for (int X = 0; X < 4; ++X)
          FLift(P + 1 * X + 4 * Y, 16);
      break;
    case 2:
      /* transform along X */
      for (int Y = 0; Y < 4; ++Y)
        FLift(P + 4 * Y, 1);
      /* transform along Y */
      for (int X = 0; X < 4; ++X)
        FLift(P + 1 * X, 4);
      break;
    case 1:
      FLift(P, 1);
      break;
    default:
      break;
  };
}


template <typename t, int S> void
ForwardZfp2D(t* P)
{
  idx2_Assert(P);
  /* transform along X */
  for (int Y = 0; Y < S; ++Y)
    FLift(P + S * Y, 1);
  /* transform along Y */
  for (int X = 0; X < S; ++X)
    FLift(P + 1 * X, S);
}


template <typename t> idx2_Inline void
InverseZfp(t* P, int D)
{
  idx2_Assert(P);
  switch (D)
  {
    case 3:
      /* transform along Z */
      for (int Y = 0; Y < 4; ++Y)
        for (int X = 0; X < 4; ++X)
          ILift(P + 1 * X + 4 * Y, 16);
      /* transform along y */
      for (int X = 0; X < 4; ++X)
        for (int Z = 0; Z < 4; ++Z)
          ILift(P + 16 * Z + 1 * X, 4);
      /* transform along X */
      for (int Z = 0; Z < 4; ++Z)
        for (int Y = 0; Y < 4; ++Y)
          ILift(P + 4 * Y + 16 * Z, 1);
      break;
    case 2:
      /* transform along y */
      for (int X = 0; X < 4; ++X)
        ILift(P + 1 * X, 4);
      /* transform along X */
      for (int Y = 0; Y < 4; ++Y)
        ILift(P + 4 * Y, 1);
      break;
    case 1:
      ILift(P, 1);
      break;
    default:
      break;
  };
}


template <typename t> void
InverseZfp(t* P)
{
  idx2_Assert(P);
  /* transform along Z */
  for (int Y = 0; Y < 4; ++Y)
    for (int X = 0; X < 4; ++X)
      ILift(P + 1 * X + 4 * Y, 16);
  /* transform along y */
  for (int X = 0; X < 4; ++X)
    for (int Z = 0; Z < 4; ++Z)
      ILift(P + 16 * Z + 1 * X, 4);
  /* transform along X */
  for (int Z = 0; Z < 4; ++Z)
    for (int Y = 0; Y < 4; ++Y)
      ILift(P + 4 * Y + 16 * Z, 1);
}


template <typename t, int S> void
InverseZfp2D(t* P)
{
  idx2_Assert(P);
  /* transform along y */
  for (int X = 0; X < S; ++X)
    ILift(P + 1 * X, S);
  /* transform along X */
  for (int Y = 0; Y < S; ++Y)
    ILift(P + S * Y, 1);
}


template <int S> struct perm2
{
  inline static const stack_array<int, S* S> Table = []()
  {
    stack_array<int, S * S> Arr;
    int I = 0;
    for (int Y = 0; Y < S; ++Y)
    {
      for (int X = 0; X < S; ++X)
      {
        Arr[I++] = Y * S + X;
      }
    }
    for (I = 0; I < Size(Arr); ++I)
    {
      for (int J = I + 1; J < Size(Arr); ++J)
      {
        int XI = Arr[I] % S, YI = Arr[I] / S;
        int XJ = Arr[J] % S, YJ = Arr[J] / S;
        if (XI + YI > XJ + YJ)
        {
          Swap(&Arr[I], &Arr[J]);
        }
        else if ((XI + YI == XJ + YJ) && (XI * XI + YI * YI > XJ * XJ + YJ * YJ))
        {
          Swap(&Arr[I], &Arr[J]);
        }
      }
    }
    return Arr;
  }();
};


/*
Use the following array to reorder transformed coefficients in a zfp block.
The ordering is first by i + j + k, then by i^2 + j^2 + k^2.
*/
#define idx2_Index(i, j, k) ((i) + 4 * (j) + 16 * (k))
constexpr i8 Perm3[64] = {
  idx2_Index(0, 0, 0), /*  0 : 0 */

  idx2_Index(1, 0, 0), /*  1 : 1 */
  idx2_Index(0, 1, 0), /*  2 : 1 */
  idx2_Index(0, 0, 1), /*  3 : 1 */

  idx2_Index(0, 1, 1), /*  4 : 2 */
  idx2_Index(1, 0, 1), /*  5 : 2 */
  idx2_Index(1, 1, 0), /*  6 : 2 */
  idx2_Index(2, 0, 0), /*  7 : 2 */
  idx2_Index(0, 2, 0), /*  8 : 2 */
  idx2_Index(0, 0, 2), /*  9 : 2 */

  idx2_Index(1, 1, 1), /* 10 : 3 */
  idx2_Index(2, 1, 0), /* 11 : 3 */
  idx2_Index(2, 0, 1), /* 12 : 3 */
  idx2_Index(0, 2, 1), /* 13 : 3 */
  idx2_Index(1, 2, 0), /* 14 : 3 */
  idx2_Index(1, 0, 2), /* 15 : 3 */
  idx2_Index(0, 1, 2), /* 16 : 3 */
  idx2_Index(3, 0, 0), /* 17 : 3 */
  idx2_Index(0, 3, 0), /* 18 : 3 */
  idx2_Index(0, 0, 3), /* 19 : 3 */

  idx2_Index(2, 1, 1), /* 20 : 4 */
  idx2_Index(1, 2, 1), /* 21 : 4 */
  idx2_Index(1, 1, 2), /* 22 : 4 */
  idx2_Index(0, 2, 2), /* 23 : 4 */
  idx2_Index(2, 0, 2), /* 24 : 4 */
  idx2_Index(2, 2, 0), /* 25 : 4 */
  idx2_Index(3, 1, 0), /* 26 : 4 */
  idx2_Index(3, 0, 1), /* 27 : 4 */
  idx2_Index(0, 3, 1), /* 28 : 4 */
  idx2_Index(1, 3, 0), /* 29 : 4 */
  idx2_Index(1, 0, 3), /* 30 : 4 */
  idx2_Index(0, 1, 3), /* 31 : 4 */

  idx2_Index(1, 2, 2), /* 32 : 5 */
  idx2_Index(2, 1, 2), /* 33 : 5 */
  idx2_Index(2, 2, 1), /* 34 : 5 */
  idx2_Index(3, 1, 1), /* 35 : 5 */
  idx2_Index(1, 3, 1), /* 36 : 5 */
  idx2_Index(1, 1, 3), /* 37 : 5 */
  idx2_Index(3, 2, 0), /* 38 : 5 */
  idx2_Index(3, 0, 2), /* 39 : 5 */
  idx2_Index(0, 3, 2), /* 40 : 5 */
  idx2_Index(2, 3, 0), /* 41 : 5 */
  idx2_Index(2, 0, 3), /* 42 : 5 */
  idx2_Index(0, 2, 3), /* 43 : 5 */

  idx2_Index(2, 2, 2), /* 44 : 6 */
  idx2_Index(3, 2, 1), /* 45 : 6 */
  idx2_Index(3, 1, 2), /* 46 : 6 */
  idx2_Index(1, 3, 2), /* 47 : 6 */
  idx2_Index(2, 3, 1), /* 48 : 6 */
  idx2_Index(2, 1, 3), /* 49 : 6 */
  idx2_Index(1, 2, 3), /* 50 : 6 */
  idx2_Index(0, 3, 3), /* 51 : 6 */
  idx2_Index(3, 0, 3), /* 52 : 6 */
  idx2_Index(3, 3, 0), /* 53 : 6 */

  idx2_Index(3, 2, 2), /* 54 : 7 */
  idx2_Index(2, 3, 2), /* 55 : 7 */
  idx2_Index(2, 2, 3), /* 56 : 7 */
  idx2_Index(1, 3, 3), /* 57 : 7 */
  idx2_Index(3, 1, 3), /* 58 : 7 */
  idx2_Index(3, 3, 1), /* 59 : 7 */

  idx2_Index(2, 3, 3), /* 60 : 8 */
  idx2_Index(3, 2, 3), /* 61 : 8 */
  idx2_Index(3, 3, 2), /* 62 : 8 */

  idx2_Index(3, 3, 3), /* 63 : 9 */
};
#undef idx2_Index


#define idx2_Index(i, j) ((i) + 4 * (j))
constexpr i8 Perm2[16] = {
  idx2_Index(0, 0), /*  0 : 0 */

  idx2_Index(1, 0), /*  1 : 1 */
  idx2_Index(0, 1), /*  2 : 1 */

  idx2_Index(1, 1), /*  3 : 2 */
  idx2_Index(2, 0), /*  4 : 2 */
  idx2_Index(0, 2), /*  5 : 2 */

  idx2_Index(2, 1), /*  6 : 3 */
  idx2_Index(1, 2), /*  7 : 3 */
  idx2_Index(3, 0), /*  8 : 3 */
  idx2_Index(0, 3), /*  9 : 3 */

  idx2_Index(2, 2), /* 10 : 4 */
  idx2_Index(3, 1), /* 11 : 4 */
  idx2_Index(1, 3), /* 12 : 4 */

  idx2_Index(3, 2), /* 13 : 5 */
  idx2_Index(2, 3), /* 14 : 5 */

  idx2_Index(3, 3), /* 15 : 6 */
};
#undef idx2_Index


template <typename t, typename u> idx2_Inline void
ForwardShuffle(t* IBlock, u* UBlock)
{
  auto Mask = traits<u>::NBinaryMask;
  for (int I = 0; I < 64; ++I)
    UBlock[I] = (u)((IBlock[Perm3[I]] + Mask) ^ Mask);
}


template <typename t, typename u> idx2_Inline void
ForwardShuffle(t* idx2_Restrict IBlock, u* idx2_Restrict UBlock, int D)
{
  auto Mask = traits<u>::NBinaryMask;
  switch (D)
  {
    case 3:
      for (int I = 0; I < 64; ++I)
        UBlock[I] = (u)((IBlock[Perm3[I]] + Mask) ^ Mask);
      break;
    case 2:
      for (int I = 0; I < 16; ++I)
        UBlock[I] = (u)((IBlock[Perm2[I]] + Mask) ^ Mask);
      break;
    case 1:
      for (int I = 0; I < 4; ++I)
        UBlock[I] = (u)((IBlock[I] + Mask) ^ Mask);
      break;
    case 0:
      for (int I = 0; I < 1; ++I)
        UBlock[I] = (u)((IBlock[I] + Mask) ^ Mask);
      break;
    default:
      idx2_Assert(false);
  };
}


template <typename t, typename u, int S> void
ForwardShuffle2D(t* IBlock, u* UBlock)
{
  auto Mask = traits<u>::NBinaryMask;
  for (int I = 0; I < S * S; ++I)
    UBlock[I] = (u)((IBlock[perm2<S>::Table[I]] + Mask) ^ Mask);
}


template <typename t, typename u> void
InverseShuffle(u* UBlock, t* IBlock)
{
  auto Mask = traits<u>::NBinaryMask;
  for (int I = 0; I < 64; ++I)
    IBlock[Perm3[I]] = (t)((UBlock[I] ^ Mask) - Mask);
}


template <typename t, typename u> idx2_Inline void
InverseShuffle(u* idx2_Restrict UBlock, t* idx2_Restrict IBlock, int D)
{
  auto Mask = traits<u>::NBinaryMask;
  switch (D)
  {
    case 3:
      for (int I = 0; I < 64; ++I)
        IBlock[Perm3[I]] = (t)((UBlock[I] ^ Mask) - Mask);
      break;
    case 2:
      for (int I = 0; I < 16; ++I)
        IBlock[Perm2[I]] = (t)((UBlock[I] ^ Mask) - Mask);
      break;
    case 1:
      for (int I = 0; I < 4; ++I)
        IBlock[I] = (t)((UBlock[I] ^ Mask) - Mask);
      break;
    case 0:
      for (int I = 0; I < 1; ++I)
        IBlock[I] = (t)((UBlock[I] ^ Mask) - Mask);
      break;
    default:
      idx2_Assert(false);
  };
}


template <typename t, typename u, int S> void
InverseShuffle2D(u* UBlock, t* IBlock)
{
  auto Mask = traits<u>::NBinaryMask;
  for (int I = 0; I < S * S; ++I)
    IBlock[perm2<S>::Table[I]] = (t)((UBlock[I] ^ Mask) - Mask);
}


// TODO: this function is only correct for block size 4
template <typename t> void
PadBlock1D(t* P, int N, int S)
{
  idx2_Assert(P);
  idx2_Assert(0 <= N && N <= 4);
  idx2_Assert(S > 0);
  switch (N)
  {
    case 0:
      P[0 * S] = 0; /* fall through */
    case 1:
      P[1 * S] = P[0 * S]; /* fall through */
    case 2:
      P[2 * S] = P[1 * S]; /* fall through */
    case 3:
      P[3 * S] = P[0 * S]; /* fall through */
    default:
      break;
  }
}


template <typename t> void
PadBlock3D(t* P, const v3i& N)
{
  for (int Z = 0; Z < 4; ++Z)
    for (int Y = 0; Y < 4; ++Y)
      PadBlock1D(P + Z * 16 + Y * 4, N.X, 1);

  for (int Z = 0; Z < 4; ++Z)
    for (int X = 0; X < 4; ++X)
      PadBlock1D(P + Z * 16 + X * 1, N.Y, 4);

  for (int Y = 0; Y < 4; ++Y)
    for (int X = 0; X < 4; ++X)
      PadBlock1D(P + Y * 4 + X * 1, N.Z, 16);
}


template <typename t> void
PadBlock2D(t* P, const v2i& N)
{
  for (int Y = 0; Y < 4; ++Y)
    PadBlock1D(P + Y * 4, N.X, 1);

  for (int X = 0; X < 4; ++X)
    PadBlock1D(P + X * 1, N.Y, 4);
}


// D is the dimension, K is the size of the block
template <typename t, int D, int K> void
Encode(t* Block, int B, i64 S, i8& N, bitstream* Bs)
{
  static_assert(is_unsigned<t>::Value);
  int NVals = power<int, K>::Table[D];
  idx2_Assert(NVals <= 64); // e.g. 4x4x4, 4x4, 8x8
  u64 X = 0;
  for (int I = 0; I < NVals; ++I)
    X += u64((Block[I] >> B) & 1u) << I;
  i8 P = (i8)Min((i64)N, S - BitSize(*Bs));
  if (P > 0)
  {
    WriteLong(Bs, X, P);
    X >>= P; // P == 64 is fine since in that case we don't need X any more
  }
  // TODO: we may be able to speed this up by getting rid of the shift of X
  // or the call bit BitSize()
  for (; BitSize(*Bs) < S && N < NVals;)
  {
    if (Write(Bs, !!X))
    { // group is significant
      for (; BitSize(*Bs) < S && N + 1 < NVals;)
      {
        if (Write(Bs, X & 1u))
        { // found a significant coeff, break and retest
          break;
        }
        else
        { // have not found a significant coeff, continue until we find one
          X >>= 1;
          ++N;
        }
      }
      if (BitSize(*Bs) >= S)
        break;
      X >>= 1;
      ++N;
    }
    else
    {
      break;
    }
  }
}


extern int MyCounter;

template <typename t, int D, int K> void
Decode(t* Block, int B, i64 S, i8& N, bitstream* Bs)
{
  static_assert(is_unsigned<t>::Value);
  int NVals = power<int, K>::Table[D];
  idx2_Assert(NVals <= 64); // e.g. 4x4x4, 4x4, 8x8
  i8 P = (i8)Min((i64)N, S - BitSize(*Bs));
  u64 X = P > 0 ? ReadLong(Bs, P) : 0;
  // std::cout << "Counter " << MyCounter << "P = " << int(P) << " X = " << X << " N = " << int(N)
  // << std::endl;
  for (; BitSize(*Bs) < S && N < NVals;)
  {
    if (Read(Bs))
    {
      for (; BitSize(*Bs) < S && N + 1 < NVals;)
      {
        if (Read(Bs))
        {
          break;
        }
        else
        {
          ++N;
        }
      }
      if (BitSize(*Bs) >= S)
        break;
      X += 1ull << (N++);
    }
    else
    {
      break;
    }
  }
  // std::cout << "N = " << int(N) << std::endl;
  /* deposit bit plane from x */
  for (int I = 0; X; ++I, X >>= 1)
    Block[I] += (t)(X & 1u) << B;
  //__m256i Add = _mm256_set1_epi32(t(1) << B); // TODO: should be epi64 with t == u64
  //__m256i Mask = _mm256_set_epi32(0xffffff7f, 0xffffffbf, 0xffffffdf, 0xffffffef, 0xfffffff7,
  //0xfffffffb, 0xfffffffd, 0xfffffffe); while (X) {
  //  __m256i Val = _mm256_set1_epi32(X);
  //  Val = _mm256_or_si256(Val, Mask);
  //  Val = _mm256_cmpeq_epi32(Val, _mm256_set1_epi64x(-1));
  //  //int table[8] ALIGNED(32) = { 1, 2, 3, 4, 5, 6, 7, 8 };
  //  _mm256_maskstore_epi32((int*)Block, Val, _mm256_add_epi32(_mm256_maskload_epi32((int*)Block,
  //  Val), Add)); X >>= 8; Block += 8;
  //}
  ++MyCounter;
}


// NOTE: this is the one being used
template <typename t> void
Encode(t* idx2_Restrict Block, int NVals, int B, /*i64 S, */ i8& N, bitstream* idx2_Restrict BsIn)
{
  static_assert(is_unsigned<t>::Value);
  idx2_Assert(NVals <= 64); // e.g. 4x4x4, 4x4, 8x8
  bitstream Bs = *BsIn;
  u64 X = 0;
  for (int I = 0; I < NVals; ++I)
    X += u64((Block[I] >> B) & 1u) << I;
  //  i8 P = (i8)Min((i64)N, S - BitSize(Bs));
  i8 P = N;
  if (P > 0)
  {
    WriteLong(&Bs, X, P);
    X >>= P; // P == 64 is fine since in that case we don't need X any more
  }
  for (; /*BitSize(Bs) < S &&*/ N < NVals;)
  {
    if (Write(&Bs, !!X))
    { // group is significant
      for (; /*BitSize(Bs) < S &&*/ N + 1 < NVals;)
      {
        if (Write(&Bs, X & 1u))
        { // found a significant coeff, break and retest
          break;
        }
        else
        { // have not found a significant coeff, continue until we find one
          X >>= 1;
          ++N;
        }
      }
      //      if (BitSize(Bs) >= S)
      //        break;
      X >>= 1;
      ++N;
    }
    else
    {
      break;
    }
  }
  *BsIn = Bs;
}


#if defined(idx2_Avx2) && defined(__AVX2__)
template <typename t> idx2_Inline void
TransposeAvx2(u64 X, int B, t* idx2_Restrict Block)
{
  __m256i Minus1 = _mm256_set1_epi64x(-1);
  __m256i Add = _mm256_set1_epi64x(t(1) << B); // TODO: should be epi64 with t == u64
  __m256i Mask = _mm256_set_epi64x(
    0xfffffffffffffff7ll, 0xfffffffffffffffbll, 0xfffffffffffffffdll, 0xfffffffffffffffell);
  // while (X) {
  __m256i Val = _mm256_set1_epi64x(X);
  Val = _mm256_or_si256(Val, Mask);
  Val = _mm256_cmpeq_epi64(Val, Minus1);
  // int table[8] ALIGNED(32) = { 1, 2, 3, 4, 5, 6, 7, 8 };
  _mm256_maskstore_epi64(
    (long long*)Block, Val, _mm256_add_epi64(_mm256_maskload_epi64((long long*)Block, Val), Add));
  // X >>= 4;
  // Block += 4;

  Val = _mm256_set1_epi64x(X >> 4);
  Val = _mm256_or_si256(Val, Mask);
  Val = _mm256_cmpeq_epi64(Val, Minus1);
  // int table[8] ALIGNED(32) = { 1, 2, 3, 4, 5, 6, 7, 8 };
  _mm256_maskstore_epi64((long long*)Block + 4,
                         Val,
                         _mm256_add_epi64(_mm256_maskload_epi64((long long*)Block + 4, Val), Add));
  // X >>= 4;
  // Block += 4;

  Val = _mm256_set1_epi64x(X >> 8);
  Val = _mm256_or_si256(Val, Mask);
  Val = _mm256_cmpeq_epi64(Val, Minus1);
  // int table[8] ALIGNED(32) = { 1, 2, 3, 4, 5, 6, 7, 8 };
  _mm256_maskstore_epi64((long long*)Block + 8,
                         Val,
                         _mm256_add_epi64(_mm256_maskload_epi64((long long*)Block + 8, Val), Add));

  Val = _mm256_set1_epi64x(X >> 12);
  Val = _mm256_or_si256(Val, Mask);
  Val = _mm256_cmpeq_epi64(Val, Minus1);
  // int table[8] ALIGNED(32) = { 1, 2, 3, 4, 5, 6, 7, 8 };
  _mm256_maskstore_epi64((long long*)Block + 12,
                         Val,
                         _mm256_add_epi64(_mm256_maskload_epi64((long long*)Block + 12, Val), Add));

  Val = _mm256_set1_epi64x(X >> 16);
  Val = _mm256_or_si256(Val, Mask);
  Val = _mm256_cmpeq_epi64(Val, Minus1);
  // int table[8] ALIGNED(32) = { 1, 2, 3, 4, 5, 6, 7, 8 };
  _mm256_maskstore_epi64((long long*)Block + 16,
                         Val,
                         _mm256_add_epi64(_mm256_maskload_epi64((long long*)Block + 16, Val), Add));

  Val = _mm256_set1_epi64x(X >> 20);
  Val = _mm256_or_si256(Val, Mask);
  Val = _mm256_cmpeq_epi64(Val, Minus1);
  // int table[8] ALIGNED(32) = { 1, 2, 3, 4, 5, 6, 7, 8 };
  _mm256_maskstore_epi64((long long*)Block + 20,
                         Val,
                         _mm256_add_epi64(_mm256_maskload_epi64((long long*)Block + 20, Val), Add));

  Val = _mm256_set1_epi64x(X >> 24);
  Val = _mm256_or_si256(Val, Mask);
  Val = _mm256_cmpeq_epi64(Val, Minus1);
  // int table[8] ALIGNED(32) = { 1, 2, 3, 4, 5, 6, 7, 8 };
  _mm256_maskstore_epi64((long long*)Block + 24,
                         Val,
                         _mm256_add_epi64(_mm256_maskload_epi64((long long*)Block + 24, Val), Add));

  Val = _mm256_set1_epi64x(X >> 28);
  Val = _mm256_or_si256(Val, Mask);
  Val = _mm256_cmpeq_epi64(Val, Minus1);
  // int table[8] ALIGNED(32) = { 1, 2, 3, 4, 5, 6, 7, 8 };
  _mm256_maskstore_epi64((long long*)Block + 28,
                         Val,
                         _mm256_add_epi64(_mm256_maskload_epi64((long long*)Block + 28, Val), Add));

  Val = _mm256_set1_epi64x(X >> 32);
  Val = _mm256_or_si256(Val, Mask);
  Val = _mm256_cmpeq_epi64(Val, Minus1);
  // int table[8] ALIGNED(32) = { 1, 2, 3, 4, 5, 6, 7, 8 };
  _mm256_maskstore_epi64((long long*)Block + 32,
                         Val,
                         _mm256_add_epi64(_mm256_maskload_epi64((long long*)Block + 32, Val), Add));

  Val = _mm256_set1_epi64x(X >> 36);
  Val = _mm256_or_si256(Val, Mask);
  Val = _mm256_cmpeq_epi64(Val, Minus1);
  // int table[8] ALIGNED(32) = { 1, 2, 3, 4, 5, 6, 7, 8 };
  _mm256_maskstore_epi64((long long*)Block + 36,
                         Val,
                         _mm256_add_epi64(_mm256_maskload_epi64((long long*)Block + 36, Val), Add));

  Val = _mm256_set1_epi64x(X >> 40);
  Val = _mm256_or_si256(Val, Mask);
  Val = _mm256_cmpeq_epi64(Val, Minus1);
  // int table[8] ALIGNED(32) = { 1, 2, 3, 4, 5, 6, 7, 8 };
  _mm256_maskstore_epi64((long long*)Block + 40,
                         Val,
                         _mm256_add_epi64(_mm256_maskload_epi64((long long*)Block + 40, Val), Add));

  Val = _mm256_set1_epi64x(X >> 44);
  Val = _mm256_or_si256(Val, Mask);
  Val = _mm256_cmpeq_epi64(Val, Minus1);
  // int table[8] ALIGNED(32) = { 1, 2, 3, 4, 5, 6, 7, 8 };
  _mm256_maskstore_epi64((long long*)Block + 44,
                         Val,
                         _mm256_add_epi64(_mm256_maskload_epi64((long long*)Block + 44, Val), Add));

  Val = _mm256_set1_epi64x(X >> 48);
  Val = _mm256_or_si256(Val, Mask);
  Val = _mm256_cmpeq_epi64(Val, Minus1);
  // int table[8] ALIGNED(32) = { 1, 2, 3, 4, 5, 6, 7, 8 };
  _mm256_maskstore_epi64((long long*)Block + 48,
                         Val,
                         _mm256_add_epi64(_mm256_maskload_epi64((long long*)Block + 48, Val), Add));

  Val = _mm256_set1_epi64x(X >> 52);
  Val = _mm256_or_si256(Val, Mask);
  Val = _mm256_cmpeq_epi64(Val, Minus1);
  // int table[8] ALIGNED(32) = { 1, 2, 3, 4, 5, 6, 7, 8 };
  _mm256_maskstore_epi64((long long*)Block + 52,
                         Val,
                         _mm256_add_epi64(_mm256_maskload_epi64((long long*)Block + 52, Val), Add));

  Val = _mm256_set1_epi64x(X >> 56);
  Val = _mm256_or_si256(Val, Mask);
  Val = _mm256_cmpeq_epi64(Val, Minus1);
  // int table[8] ALIGNED(32) = { 1, 2, 3, 4, 5, 6, 7, 8 };
  _mm256_maskstore_epi64((long long*)Block + 56,
                         Val,
                         _mm256_add_epi64(_mm256_maskload_epi64((long long*)Block + 56, Val), Add));

  Val = _mm256_set1_epi64x(X >> 60);
  Val = _mm256_or_si256(Val, Mask);
  Val = _mm256_cmpeq_epi64(Val, Minus1);
  // int table[8] ALIGNED(32) = { 1, 2, 3, 4, 5, 6, 7, 8 };
  _mm256_maskstore_epi64((long long*)Block + 60,
                         Val,
                         _mm256_add_epi64(_mm256_maskload_epi64((long long*)Block + 60, Val), Add));
  //}
}
#endif


idx2_Inline void
DecodeTest(u64* idx2_Restrict Block, int NVals, i8& N, bitstream* idx2_Restrict BsIn)
{
  idx2_Assert(NVals <= 64); // e.g. 4x4x4, 4x4, 8x8
  bitstream Bs = *BsIn;
  i8 P = N;
  u64 X = P > 0 ? ReadLong(&Bs, P) : 0;
  for (; N < NVals;)
  {
    if (Read(&Bs))
    {
      for (; N + 1 < NVals;)
        if (Read(&Bs))
          break;
        else
          ++N;
      X += 1ull << (N++);
    }
    else
    {
      break;
    }
  }
  *Block = X;
  *BsIn = Bs;
}


// NOTE: This is the one being used
template <typename t> void
Decode(t* idx2_Restrict Block,
       int NVals,
       int B, /*i64 S, */
       i8& N,
       bitstream* idx2_Restrict BsIn,
       bool BypassDecode = false)
{
  static_assert(is_unsigned<t>::Value);
  idx2_Assert(NVals <= 64); // e.g. 4x4x4, 4x4, 8x8
  bitstream Bs = *BsIn;
  //  i8 P = (i8)Min((i64)N, S - BitSize(Bs));
  i8 P = N;
  u64 X = P > 0 ? ReadLong(&Bs, P) : 0;
  for (; /*BitSize(Bs) < S &&*/ N < NVals;)
  {
    if (Read(&Bs))
    {
      for (; /*BitSize(Bs) < S &&*/ N + 1 < NVals;)
        if (Read(&Bs))
          break;
        else
          ++N;
      // if (BitSize(Bs) >= S)
      //   break;
      X += 1ull << (N++);
    }
    else
    {
      break;
    }
  }
    //  TransposeRecursive();
//  int K = Msb(X);
//  if (K >= 0) {
//    for (int I = 0; I < K; ++I)
//      Block[I] += (t)((X >> I) & 1u) << B;
//  }

  if (!BypassDecode)
  {
    #if defined(idx2_Avx2) && defined(__AVX2__)
      //printf("avx\n");
      __m256i Minus1 = _mm256_set1_epi64x(-1);
      __m256i Add = _mm256_set1_epi64x(t(1) << B);
      __m256i Mask = _mm256_set_epi64x(
        0xfffffffffffffff7ll, 0xfffffffffffffffbll, 0xfffffffffffffffdll, 0xfffffffffffffffell);
      while (X)
      { // the input value X is used as a mask to add the shifted 1 bits (in Add) to the 4 values in
        // Block
        __m256i Val = _mm256_set1_epi64x(X);
        Val = _mm256_or_si256(Val, Mask);
        Val = _mm256_cmpeq_epi64(Val, Minus1); // "spread" the bits of X to 4 lanes
        // TODO: to decode more than one bit plane, we can spread the bits of 8 bit planes (or more) to
        // 4 lanes and then add only once we can even work with 32 8-bit lanes (_epu8 unsigned char) to
        // do bit transposing and shift the values when adding back to the results later
        // int table[8] ALIGNED(32) = { 1, 2, 3, 4, 5, 6, 7, 8 };
        _mm256_maskstore_epi64(
          (long long int*)Block,
          Val,
          _mm256_add_epi64(_mm256_maskload_epi64((long long int*)Block, Val), Add));
        X >>= 4;
        Block += 4;
      }
    #else
      for (int I = 0; X; ++I, X >>= 1)
        Block[I] += (t)(X & 1u) << B;
    #endif
  }
  *BsIn = Bs;
}


#if defined(idx2_Avx2) && defined(__AVX2__)
template <typename t, int D, int K> void
Decode4(t* Block, int B, i64 S, i8& N, bitstream* Bs)
{
  static_assert(is_unsigned<t>::Value);
  int NVals = power<int, K>::Table[D];
  idx2_Assert(NVals <= 64); // e.g. 4x4x4, 4x4, 8x8
  i8 P = (i8)Min((i64)N, S - BitSize(*Bs));
  u64 X = P > 0 ? ReadLong(Bs, P) : 0;
  // std::cout << "Counter " << MyCounter << "P = " << int(P) << " X = " << X << " N = " << int(N)
  // << std::endl;
  for (; /*BitSize(*Bs) < S &&*/ N < NVals;)
  {
    if (Read(Bs))
    {
      for (; /*BitSize(*Bs) < S &&*/ N + 1 < NVals;)
      {
        if (Read(Bs))
          break;
        else
          ++N;
      }
      /*if (BitSize(*Bs) >= S)
        break;*/
      X += 1ull << (N++);
    }
    else
    {
      break;
    }
  }
  // std::cout << "N = " << int(N) << std::endl;
  /* deposit bit plane from x */
  // for (int I = 0; X; ++I, X >>= 1)
  //   Block[I] += (t)(X & 1u) << B;
  __m256i Add = _mm256_set1_epi32(t(1) << B); // TODO: should be epi64 with t == u64
  __m256i Mask = _mm256_set_epi32(
    0xffffff7f, 0xffffffbf, 0xffffffdf, 0xffffffef, 0xfffffff7, 0xfffffffb, 0xfffffffd, 0xfffffffe);
  while (X)
  {
    __m256i Val = _mm256_set1_epi32(X);
    Val = _mm256_or_si256(Val, Mask);
    Val = _mm256_cmpeq_epi32(Val, _mm256_set1_epi64x(-1));
    // int table[8] ALIGNED(32) = { 1, 2, 3, 4, 5, 6, 7, 8 };
    _mm256_maskstore_epi32(
      (int*)Block, Val, _mm256_add_epi32(_mm256_maskload_epi32((int*)Block, Val), Add));
    X >>= 8;
    Block += 8;
  }
  ++MyCounter;
}
#endif


template <typename t> idx2_Inline void
TransposeNormal(u64 X, int B, t* idx2_Restrict Block)
{
  for (int I = 0; X; ++I, X >>= 1)
    Block[I] += (t)(X & 1u) << B;
}


#define zfp_swap(x, y, l)                                                                          \
  do                                                                                               \
  {                                                                                                \
    const uint64 m[] = {                                                                           \
      0x5555555555555555ul, 0x3333333333333333ul, 0x0f0f0f0f0f0f0f0ful,                            \
      0x00ff00ff00ff00fful, 0x0000ffff0000fffful, 0x00000000fffffffful,                            \
    };                                                                                             \
    uint s = 1u << (l);                                                                            \
    uint64 v = ((x) ^ ((y) >> s)) & m[(l)];                                                        \
    (x) ^= v;                                                                                      \
    (y) ^= v << s;                                                                                 \
  } while (0)


/* compress sequence of 4^3 = 64 unsigned integers */
template <typename t> void
TransposeRecursive(t* data, int NBps)
{
  /* working space for 64 bit planes */
  u64 a00 = 0, a01 = 0, a02 = 0, a03 = 0, a04 = 0;
  u64 a05 = 0, a06 = 0, a07 = 0, a08 = 0, a09 = 0;
  u64 a0a = 0, a0b = 0, a0c = 0, a0d = 0, a0e = 0;
  u64 a0f = 0, a10 = 0, a11 = 0, a12 = 0, a13 = 0;
  u64 a14 = 0, a15 = 0, a16 = 0, a17 = 0, a18 = 0;
  u64 a19 = 0, a1a = 0, a1b = 0, a1c = 0, a1d = 0;
  u64 a1e = 0, a1f = 0, a20 = 0, a21 = 0, a22 = 0;
  u64 a23 = 0, a24 = 0, a25 = 0, a26 = 0, a27 = 0;
  u64 a28 = 0, a29 = 0, a2a = 0, a2b = 0, a2c = 0;
  u64 a2d = 0, a2e = 0, a2f = 0, a30 = 0, a31 = 0;
  u64 a32 = 0, a33 = 0, a34 = 0, a35 = 0, a36 = 0;
  u64 a37 = 0, a38 = 0, a39 = 0, a3a = 0, a3b = 0;
  u64 a3c = 0, a3d = 0, a3e = 0, a3f = 0;

  if (NBps <= 0x02)
    goto A02;
  if (NBps <= 0x04)
    goto A04;
  if (NBps <= 0x06)
    goto A06;
  if (NBps <= 0x08)
    goto A08;
  if (NBps <= 0x0A)
    goto A0A;
  if (NBps <= 0x0C)
    goto A0C;
  if (NBps <= 0x0E)
    goto A0E;
  if (NBps <= 0x10)
    goto A10;
  if (NBps <= 0x12)
    goto A12;
  if (NBps <= 0x14)
    goto A14;
  if (NBps <= 0x16)
    goto A16;
  if (NBps <= 0x18)
    goto A18;
  if (NBps <= 0x1A)
    goto A1A;
  if (NBps <= 0x1C)
    goto A1C;
  if (NBps <= 0x1E)
    goto A1E;
  if (NBps <= 0x20)
    goto A20;
  if (NBps <= 0x22)
    goto A22;
  if (NBps <= 0x24)
    goto A24;
  if (NBps <= 0x26)
    goto A26;
  if (NBps <= 0x28)
    goto A28;
  if (NBps <= 0x2A)
    goto A2A;
  if (NBps <= 0x2C)
    goto A2C;
  if (NBps <= 0x2E)
    goto A2E;
  if (NBps <= 0x30)
    goto A30;
  if (NBps <= 0x32)
    goto A32;
  if (NBps <= 0x34)
    goto A34;
  if (NBps <= 0x36)
    goto A36;
  if (NBps <= 0x38)
    goto A38;
  if (NBps <= 0x3A)
    goto A3A;
  if (NBps <= 0x3C)
    goto A3C;
  if (NBps <= 0x3E)
    goto A3E;

  /* 00, 01 */
  // encode_bit_plane(a3f);
  // encode_bit_plane(a3e);
  a3e = data[62], a3f = data[63];
  zfp_swap(a3e, a3f, 0);
  /* 3e, 3f */
  // encode_bit_plane(a3d);
  // encode_bit_plane(a3c);
A3E:
  a3c = data[60];
  a3d = data[61];
  zfp_swap(a3c, a3d, 0);

  zfp_swap(a3d, a3f, 1);
  zfp_swap(a3c, a3e, 1);
  /* 3c, 3d */
  // encode_bit_plane(a3b);
  // encode_bit_plane(a3a);
A3C:
  a3a = data[58];
  a3b = data[59];
  zfp_swap(a3a, a3b, 0);
  /* 3a, 3b */
  // encode_bit_plane(a39);
  // encode_bit_plane(a38);
A3A:
  a38 = data[56];
  a39 = data[57];
  zfp_swap(a38, a39, 0);

  zfp_swap(a39, a3b, 1);
  zfp_swap(a38, a3a, 1);

  zfp_swap(a3b, a3f, 2);
  zfp_swap(a3a, a3e, 2);
  zfp_swap(a39, a3d, 2);
  zfp_swap(a38, a3c, 2);
  /* 38, 39 */
  // encode_bit_plane(a37);
  // encode_bit_plane(a36);
A38:
  a36 = data[54];
  a37 = data[55];
  zfp_swap(a36, a37, 0);
  /* 36, 37 */

  // encode_bit_plane(a35);
  // encode_bit_plane(a34);
A36:
  a34 = data[52];
  a35 = data[53];
  zfp_swap(a34, a35, 0);

  zfp_swap(a35, a37, 1);
  zfp_swap(a34, a36, 1);
  /* 34, 35 */
  // encode_bit_plane(a33);
  // encode_bit_plane(a32);
A34:
  a32 = data[50];
  a33 = data[51];
  zfp_swap(a32, a33, 0);
  /* 32, 33 */
  // encode_bit_plane(a31);
  // encode_bit_plane(a30);
A32:
  a30 = data[48];
  a31 = data[49];
  zfp_swap(a30, a31, 0);

  zfp_swap(a31, a33, 1);
  zfp_swap(a30, a32, 1);

  zfp_swap(a33, a37, 2);
  zfp_swap(a32, a36, 2);
  zfp_swap(a31, a35, 2);
  zfp_swap(a30, a34, 2);

  zfp_swap(a37, a3f, 3);
  zfp_swap(a36, a3e, 3);
  zfp_swap(a35, a3d, 3);
  zfp_swap(a34, a3c, 3);
  zfp_swap(a33, a3b, 3);
  zfp_swap(a32, a3a, 3);
  zfp_swap(a31, a39, 3);
  zfp_swap(a30, a38, 3);
  /* 30, 31 */
  // encode_bit_plane(a2f);
  // encode_bit_plane(a2e);
A30:
  a2e = data[46];
  a2f = data[47];
  zfp_swap(a2e, a2f, 0);
  /* 2e, 2f */
  // encode_bit_plane(a2d);
  // encode_bit_plane(a2c);
A2E:
  a2c = data[44];
  a2d = data[45];
  zfp_swap(a2c, a2d, 0);

  zfp_swap(a2d, a2f, 1);
  zfp_swap(a2c, a2e, 1);
  /* 2c, 2d */
  // encode_bit_plane(a2b);
  // encode_bit_plane(a2a);
A2C:
  a2a = data[42];
  a2b = data[43];
  zfp_swap(a2a, a2b, 0);
  /* 2a, 2b */

  // encode_bit_plane(a29);
  // encode_bit_plane(a28);
A2A:
  a28 = data[40];
  a29 = data[41];
  zfp_swap(a28, a29, 0);

  zfp_swap(a29, a2b, 1);
  zfp_swap(a28, a2a, 1);

  zfp_swap(a2b, a2f, 2);
  zfp_swap(a2a, a2e, 2);
  zfp_swap(a29, a2d, 2);
  zfp_swap(a28, a2c, 2);
  /* 28, 29 */
  // encode_bit_plane(a27);
  // encode_bit_plane(a26);
A28:
  a26 = data[38];
  a27 = data[39];
  zfp_swap(a26, a27, 0);
  /* 26, 27 */
  // encode_bit_plane(a25);
  // encode_bit_plane(a24);
A26:
  a24 = data[36];
  a25 = data[37];
  zfp_swap(a24, a25, 0);

  zfp_swap(a25, a27, 1);
  zfp_swap(a24, a26, 1);
  /* 24, 25 */
  // encode_bit_plane(a23);
  // encode_bit_plane(a22);
A24:
  a22 = data[34];
  a23 = data[35];
  zfp_swap(a22, a23, 0);
  /* 22, 23 */
  // encode_bit_plane(a21);
  // encode_bit_plane(a20);
A22:
  a20 = data[32];
  a21 = data[33];
  zfp_swap(a20, a21, 0);

  zfp_swap(a21, a23, 1);
  zfp_swap(a20, a22, 1);

  zfp_swap(a23, a27, 2);
  zfp_swap(a22, a26, 2);
  zfp_swap(a21, a25, 2);
  zfp_swap(a20, a24, 2);

  zfp_swap(a27, a2f, 3);
  zfp_swap(a26, a2e, 3);
  zfp_swap(a25, a2d, 3);
  zfp_swap(a24, a2c, 3);
  zfp_swap(a23, a2b, 3);
  zfp_swap(a22, a2a, 3);
  zfp_swap(a21, a29, 3);
  zfp_swap(a20, a28, 3);

  zfp_swap(a2f, a3f, 4);
  zfp_swap(a2e, a3e, 4);
  zfp_swap(a2d, a3d, 4);
  zfp_swap(a2c, a3c, 4);
  zfp_swap(a2b, a3b, 4);
  zfp_swap(a2a, a3a, 4);
  zfp_swap(a29, a39, 4);
  zfp_swap(a28, a38, 4);
  zfp_swap(a27, a37, 4);
  zfp_swap(a26, a36, 4);
  zfp_swap(a25, a35, 4);
  zfp_swap(a24, a34, 4);
  zfp_swap(a23, a33, 4);
  zfp_swap(a22, a32, 4);
  zfp_swap(a21, a31, 4);
  zfp_swap(a20, a30, 4);
  /* 20, 21 */
  // encode_bit_plane(a1f);
  // encode_bit_plane(a1e);
A20:
  a1e = data[30];
  a1f = data[31];
  zfp_swap(a1e, a1f, 0);
  /* 1e, 1f */
  // encode_bit_plane(a1d);
  // encode_bit_plane(a1c);
A1E:
  a1c = data[28];
  a1d = data[29];
  zfp_swap(a1c, a1d, 0);

  zfp_swap(a1d, a1f, 1);
  zfp_swap(a1c, a1e, 1);
  /* 1c, 1d */
  // encode_bit_plane(a1b);
  // encode_bit_plane(a1a);
A1C:
  a1a = data[26];
  a1b = data[27];
  zfp_swap(a1a, a1b, 0);
  /* 1a, 1b */
  // encode_bit_plane(a19);
  // encode_bit_plane(a18);
A1A:
  a18 = data[24];
  a19 = data[25];
  zfp_swap(a18, a19, 0);

  zfp_swap(a19, a1b, 1);
  zfp_swap(a18, a1a, 1);

  zfp_swap(a1b, a1f, 2);
  zfp_swap(a1a, a1e, 2);
  zfp_swap(a19, a1d, 2);
  zfp_swap(a18, a1c, 2);
  /* 18, 19 */
  // encode_bit_plane(a17);
  // encode_bit_plane(a16);
A18:
  a16 = data[22];
  a17 = data[23];
  zfp_swap(a16, a17, 0);
  /* 16, 17 */

  // encode_bit_plane(a15);
  // encode_bit_plane(a14);
A16:
  a14 = data[20];
  a15 = data[21];
  zfp_swap(a14, a15, 0);

  zfp_swap(a15, a17, 1);
  zfp_swap(a14, a16, 1);
  /* 14, 15 */

  // encode_bit_plane(a13);
  // encode_bit_plane(a12);
A14:
  a12 = data[18];
  a13 = data[19];
  zfp_swap(a12, a13, 0);
  /* 12, 13 */

  // encode_bit_plane(a11);
  // encode_bit_plane(a10);
A12:
  a10 = data[16];
  a11 = data[17];
  zfp_swap(a10, a11, 0);

  zfp_swap(a11, a13, 1);
  zfp_swap(a10, a12, 1);

  zfp_swap(a13, a17, 2);
  zfp_swap(a12, a16, 2);
  zfp_swap(a11, a15, 2);
  zfp_swap(a10, a14, 2);

  zfp_swap(a17, a1f, 3);
  zfp_swap(a16, a1e, 3);
  zfp_swap(a15, a1d, 3);
  zfp_swap(a14, a1c, 3);
  zfp_swap(a13, a1b, 3);
  zfp_swap(a12, a1a, 3);
  zfp_swap(a11, a19, 3);
  zfp_swap(a10, a18, 3);
  /* 10, 11 */

  // encode_bit_plane(a0f);
  // encode_bit_plane(a0e);
A10:
  a0e = data[14];
  a0f = data[15];
  zfp_swap(a0e, a0f, 0);
  /* 0e, 0f */

  // encode_bit_plane(a0d);
  // encode_bit_plane(a0c);
A0E:
  a0c = data[12], a0d = data[13];
  zfp_swap(a0c, a0d, 0);

  zfp_swap(a0d, a0f, 1);
  zfp_swap(a0c, a0e, 1);
  /* 0c, 0d */

  // encode_bit_plane(a0b);
  // encode_bit_plane(a0a);
A0C:
  a0a = data[10];
  a0b = data[11];
  zfp_swap(a0a, a0b, 0);
  /* 0a, 0b */

  // encode_bit_plane(a09);
  // encode_bit_plane(a08);
A0A:
  a08 = data[8];
  a09 = data[9];
  zfp_swap(a08, a09, 0);

  zfp_swap(a09, a0b, 1);
  zfp_swap(a08, a0a, 1);

  zfp_swap(a0b, a0f, 2);
  zfp_swap(a0a, a0e, 2);
  zfp_swap(a09, a0d, 2);
  zfp_swap(a08, a0c, 2);
  /* 08, 09 */
  // encode_bit_plane(a07);
  // encode_bit_plane(a06);
A08:
  a06 = data[6];
  a07 = data[7];
  zfp_swap(a06, a07, 0);
  /* 06, 07 */
  // encode_bit_plane(a05);
  // encode_bit_plane(a04);
A06:
  a04 = data[4];
  a05 = data[5];
  zfp_swap(a04, a05, 0);

  zfp_swap(a05, a07, 1);
  zfp_swap(a04, a06, 1);
  /* 04, 05 */
  // encode_bit_plane(a03);
  // encode_bit_plane(a02);
A04:
  a02 = data[2];
  a03 = data[3];
  zfp_swap(a02, a03, 0);
  /* 02, 03 */
  // encode_bit_plane(a01);
  // encode_bit_plane(a00);
A02:
  a00 = data[0];
  a01 = data[1];
  zfp_swap(a00, a01, 0);

  zfp_swap(a01, a03, 1);
  zfp_swap(a00, a02, 1);

  zfp_swap(a03, a07, 2);
  zfp_swap(a02, a06, 2);
  zfp_swap(a01, a05, 2);
  zfp_swap(a00, a04, 2);

  zfp_swap(a07, a0f, 3);
  zfp_swap(a06, a0e, 3);
  zfp_swap(a05, a0d, 3);
  zfp_swap(a04, a0c, 3);
  zfp_swap(a03, a0b, 3);
  zfp_swap(a02, a0a, 3);
  zfp_swap(a01, a09, 3);
  zfp_swap(a00, a08, 3);

  zfp_swap(a0f, a1f, 4);
  zfp_swap(a0e, a1e, 4);
  zfp_swap(a0d, a1d, 4);
  zfp_swap(a0c, a1c, 4);
  zfp_swap(a0b, a1b, 4);
  zfp_swap(a0a, a1a, 4);
  zfp_swap(a09, a19, 4);
  zfp_swap(a08, a18, 4);
  zfp_swap(a07, a17, 4);
  zfp_swap(a06, a16, 4);
  zfp_swap(a05, a15, 4);
  zfp_swap(a04, a14, 4);
  zfp_swap(a03, a13, 4);
  zfp_swap(a02, a12, 4);
  zfp_swap(a01, a11, 4);
  zfp_swap(a00, a10, 4);

  zfp_swap(a1f, a3f, 5);
  zfp_swap(a1e, a3e, 5);
  zfp_swap(a1d, a3d, 5);
  zfp_swap(a1c, a3c, 5);
  zfp_swap(a1b, a3b, 5);
  zfp_swap(a1a, a3a, 5);
  zfp_swap(a19, a39, 5);
  zfp_swap(a18, a38, 5);
  zfp_swap(a17, a37, 5);
  zfp_swap(a16, a36, 5);
  zfp_swap(a15, a35, 5);
  zfp_swap(a14, a34, 5);
  zfp_swap(a13, a33, 5);
  zfp_swap(a12, a32, 5);
  zfp_swap(a11, a31, 5);
  zfp_swap(a10, a30, 5);
  zfp_swap(a0f, a2f, 5);
  zfp_swap(a0e, a2e, 5);
  zfp_swap(a0d, a2d, 5);
  zfp_swap(a0c, a2c, 5);
  zfp_swap(a0b, a2b, 5);
  zfp_swap(a0a, a2a, 5);
  zfp_swap(a09, a29, 5);
  zfp_swap(a08, a28, 5);
  zfp_swap(a07, a27, 5);
  zfp_swap(a06, a26, 5);
  zfp_swap(a05, a25, 5);
  zfp_swap(a04, a24, 5);
  zfp_swap(a03, a23, 5);
  zfp_swap(a02, a22, 5);
  zfp_swap(a01, a21, 5);
  zfp_swap(a00, a20, 5);

  /* copy 64x64 matrix from input */
  data[0x3f - 0x00] = a00;
  data[0x3f - 0x01] = a01;
  data[0x3f - 0x02] = a02;
  data[0x3f - 0x03] = a03;
  data[0x3f - 0x04] = a04;
  data[0x3f - 0x05] = a05;
  data[0x3f - 0x06] = a06;
  data[0x3f - 0x07] = a07;
  data[0x3f - 0x08] = a08;
  data[0x3f - 0x09] = a09;
  data[0x3f - 0x0a] = a0a;
  data[0x3f - 0x0b] = a0b;
  data[0x3f - 0x0c] = a0c;
  data[0x3f - 0x0d] = a0d;
  data[0x3f - 0x0e] = a0e;
  data[0x3f - 0x0f] = a0f;
  data[0x3f - 0x10] = a10;
  data[0x3f - 0x11] = a11;
  data[0x3f - 0x12] = a12;
  data[0x3f - 0x13] = a13;
  data[0x3f - 0x14] = a14;
  data[0x3f - 0x15] = a15;
  data[0x3f - 0x16] = a16;
  data[0x3f - 0x17] = a17;
  data[0x3f - 0x18] = a18;
  data[0x3f - 0x19] = a19;
  data[0x3f - 0x1a] = a1a;
  data[0x3f - 0x1b] = a1b;
  data[0x3f - 0x1c] = a1c;
  data[0x3f - 0x1d] = a1d;
  data[0x3f - 0x1e] = a1e;
  data[0x3f - 0x1f] = a1f;
  data[0x3f - 0x20] = a20;
  data[0x3f - 0x21] = a21;
  data[0x3f - 0x22] = a22;
  data[0x3f - 0x23] = a23;
  data[0x3f - 0x24] = a24;
  data[0x3f - 0x25] = a25;
  data[0x3f - 0x26] = a26;
  data[0x3f - 0x27] = a27;
  data[0x3f - 0x28] = a28;
  data[0x3f - 0x29] = a29;
  data[0x3f - 0x2a] = a2a;
  data[0x3f - 0x2b] = a2b;
  data[0x3f - 0x2c] = a2c;
  data[0x3f - 0x2d] = a2d;
  data[0x3f - 0x2e] = a2e;
  data[0x3f - 0x2f] = a2f;
  data[0x3f - 0x30] = a30;
  data[0x3f - 0x31] = a31;
  data[0x3f - 0x32] = a32;
  data[0x3f - 0x33] = a33;
  data[0x3f - 0x34] = a34;
  data[0x3f - 0x35] = a35;
  data[0x3f - 0x36] = a36;
  data[0x3f - 0x37] = a37;
  data[0x3f - 0x38] = a38;
  data[0x3f - 0x39] = a39;
  data[0x3f - 0x3a] = a3a;
  data[0x3f - 0x3b] = a3b;
  data[0x3f - 0x3c] = a3c;
  data[0x3f - 0x3d] = a3d;
  data[0x3f - 0x3e] = a3e;
  data[0x3f - 0x3f] = a3f;
}


#if defined(idx2_Avx2) && defined(__AVX2__)
template <typename t, int D, int K> void
Decode2(t* Block, int B, i64 S, i8& N, bitstream* Bs)
{
  static_assert(is_unsigned<t>::Value);
  int NVals = power<int, K>::Table[D];
  idx2_Assert(NVals <= 64); // e.g. 4x4x4, 4x4, 8x8
  i8 P = (i8)Min((i64)N, S - BitSize(*Bs));
  u64 X = P > 0 ? ReadLong(Bs, P) : 0;
  // std::cout << "Counter " << MyCounter << "P = " << int(P) << " X = " << X << " N = " << int(N)
  // << std::endl;
  bool ExpectGroupTestBit = true;
  while (N < NVals)
  {
    Refill(Bs);
    int MaxBits = 64 - Bs->BitPos;
    u64 Next = Peek(Bs, MaxBits);
    i8 BitCurr = -1 + !ExpectGroupTestBit;
    i8 BitPrev = BitCurr;
    if (ExpectGroupTestBit)
    {
      BitCurr = Lsb(Next, -1);
      if (BitCurr != 0)
      {
        // there are no 1 bits, or the first bit is not 1 (the group is insignificant)
        Consume(Bs, 1);
        break;
      }
      /* group test bit is 1, at position 0. now move on to the next value 1-bit */
      idx2_Assert(BitCurr == 0);
      // if ((BitCurr = TzCnt(Next = UnsetBit(Next, 0), MaxBits)) != MaxBits) // could be MaxBits
      BitCurr = Lsb(Next = UnsetBit(Next, 0), MaxBits);
      BitPrev = 0;
      if (BitCurr - BitPrev >= NVals - N)
        goto JUMP2;
    }
    else
    {
      goto JUMP;
    }
    /* BitCurr == position of the next value 1-bit (could be -1) */
    while (true)
    { // we loop through every other 1-bit
      N += BitCurr - BitPrev;
      if (BitCurr < MaxBits || N == NVals)
      {
        X += 1ull << (N - 1);
        // Block[N - 1] += 1ull << (N - 1);
      }
      if (BitCurr + 1 >= MaxBits)
      { // there is no bit left
        ExpectGroupTestBit = BitCurr < MaxBits;
        Consume(Bs, MaxBits);
        goto OUTER;
      }
      else if (!BitSet(Next, BitCurr + 1))
      { // next group test bit is 0
        Consume(Bs, BitCurr + 2);
        goto DONE;
      }
      else
      {                                        // next group test bit is 1
        Next &= ~(3ull << (BitCurr + 1)) >> 1; // unset BitCurr and BitCurr + 1
        BitPrev = BitCurr + 1;
      JUMP:
        BitCurr = Lsb(Next, MaxBits);
        if (BitCurr - BitPrev >= NVals - N)
        {
        JUMP2:
          Consume(Bs, (NVals - N) + BitPrev);
          N = NVals;
          X += 1ull << (NVals - 1);
          // Block[NVals - 1] += 1ull << (NVals - 1);
          goto DONE;
        }
      }
    }
  OUTER:;
  }
DONE:
  // std::cout << "N = " << int(N) << std::endl;
  ++MyCounter;

  /* deposit bit plane from x */
  // for (int I = 0; X; ++I, X >>= 1)
  //   Block[I] += (t)(X & 1u) << B;
  __m256i Add = _mm256_set1_epi32(t(1) << B); // TODO: should be epi64 with t == u64
  __m256i Mask = _mm256_set_epi32(
    0xffffff7f, 0xffffffbf, 0xffffffdf, 0xffffffef, 0xfffffff7, 0xfffffffb, 0xfffffffd, 0xfffffffe);
  while (X)
  {
    __m256i Val = _mm256_set1_epi32(X);
    Val = _mm256_or_si256(Val, Mask);
    Val = _mm256_cmpeq_epi32(Val, _mm256_set1_epi64x(-1));
    // int table[8] ALIGNED(32) = { 1, 2, 3, 4, 5, 6, 7, 8 };
    _mm256_maskstore_epi32(
      (int*)Block, Val, _mm256_add_epi32(_mm256_maskload_epi32((int*)Block, Val), Add));
    X >>= 8;
    Block += 8;
  }
}
#endif


template <typename t, int D, int K> void
Decode3(t* Block, int B, i64 S, i8& N, bitstream* Bs)
{
  static_assert(is_unsigned<t>::Value);
  int NVals = power<int, K>::Table[D];
  idx2_Assert(NVals <= 64); // e.g. 4x4x4, 4x4, 8x8
  i8 P = (i8)Min((i64)N, S - BitSize(*Bs));
  u64 X = P > 0 ? ReadLong(Bs, P) : 0;
  // std::cout << "Counter " << MyCounter << "P = " << int(P) << " X = " << X << " N = " << int(N)
  // << std::endl;
  bool ExpectGroupTestBit = true;
  while (N < NVals)
  {
    Refill(Bs);
    int MaxBits = 64 - Bs->BitPos;
    u64 Next = Peek(Bs, MaxBits);
    i8 BitCurr = -1 + !ExpectGroupTestBit;
    i8 BitPrev = BitCurr;
    if (ExpectGroupTestBit)
    {
      BitCurr = Lsb(Next, -1);
      if (BitCurr != 0)
      {
        // there are no 1 bits, or the first bit is not 1 (the group is insignificant)
        Consume(Bs, 1);
        break;
      }
      /* group test bit is 1, at position 0. now move on to the next value 1-bit */
      idx2_Assert(BitCurr == 0);
      // if ((BitCurr = TzCnt(Next = UnsetBit(Next, 0), MaxBits)) != MaxBits) // could be MaxBits
      BitCurr = Lsb(Next = UnsetBit(Next, 0), MaxBits);
      BitPrev = 0;
      if (BitCurr - BitPrev >= NVals - N)
        goto JUMP2;
    }
    else
    {
      goto JUMP;
    }
    /* BitCurr == position of the next value 1-bit (could be -1) */
    while (true)
    { // we loop through every other 1-bit
      N += BitCurr - BitPrev;
      if (BitCurr < MaxBits || N == NVals)
      {
        X += 1ull << (N - 1);
        // Block[N - 1] += 1ull << (N - 1);
      }
      if (BitCurr + 1 >= MaxBits)
      { // there is no bit left
        ExpectGroupTestBit = BitCurr < MaxBits;
        Consume(Bs, MaxBits);
        goto OUTER;
      }
      else if (!BitSet(Next, BitCurr + 1))
      { // next group test bit is 0
        Consume(Bs, BitCurr + 2);
        goto DONE;
      }
      else
      {                                        // next group test bit is 1
        Next &= ~(3ull << (BitCurr + 1)) >> 1; // unset BitCurr and BitCurr + 1
        BitPrev = BitCurr + 1;
      JUMP:
        BitCurr = Lsb(Next, MaxBits);
        if (BitCurr - BitPrev >= NVals - N)
        {
        JUMP2:
          Consume(Bs, (NVals - N) + BitPrev);
          N = NVals;
          X += 1ull << (NVals - 1);
          // Block[NVals - 1] += 1ull << (NVals - 1);
          goto DONE;
        }
      }
    }
  OUTER:;
  }
DONE:
  // std::cout << "N = " << int(N) << std::endl;
  ++MyCounter;

  /* deposit bit plane from x */
  for (int I = 0; X; ++I, X >>= 1)
    Block[I] += (t)(X & 1u) << B;
  //__m256i Add = _mm256_set1_epi32(t(1) << B); // TODO: should be epi64 with t == u64
  //__m256i Mask = _mm256_set_epi32(0xffffff7f, 0xffffffbf, 0xffffffdf, 0xffffffef, 0xfffffff7,
  //0xfffffffb, 0xfffffffd, 0xfffffffe); while (X) {
  //  __m256i Val = _mm256_set1_epi32(X);
  //  Val = _mm256_or_si256(Val, Mask);
  //  Val = _mm256_cmpeq_epi32(Val, _mm256_set1_epi64x(-1));
  //  //int table[8] ALIGNED(32) = { 1, 2, 3, 4, 5, 6, 7, 8 };
  //  _mm256_maskstore_epi32((int*)Block, Val, _mm256_add_epi32(_mm256_maskload_epi32((int*)Block,
  //  Val), Add)); X >>= 8; Block += 8;
  //}
}


} // namespace idx2
