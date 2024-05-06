#pragma once


#include "Assert.h"
#include "Common.h"
#include "Error.h"
#include "Math.h"


namespace idx2
{


u32
Murmur3_32(u8* Key, int Len, u32 Seed);


idx2_Inline i8
EffectiveDims(const nd_size& Dims)
{
  i8 D = Dims.Dims() - 1;
  while ((D >= 0) && (Dims[D] == 1))
    --D;
  return D + 1;
}


/* "Push" dimension D toward the beginning (index 0) so it becomes the fastest varying dimension. */
idx2_Inline nd_size
MakeFastestDimension(nd_size P, i8 D)
{
  while (D > 0)
  {
    Swap(&P[D - 1], &P[D]);
    --D;
  }
  return P;
}


idx2_Inline nd_size
SetDimension(nd_size P, i8 D, i32 Val)
{
  P[D] = Val;
  return P;
}



template <typename t> idx2_Inline void
ndLoop(const nd_index& Begin, const nd_size& End, const nd_size& Step, const t& Kernel)
{
  i8 D = EffectiveDims(End - Begin);
  idx2_Assert(D <= 6);
  int X, Y, Z, U, V, W;
  switch (D)
  {
    case 0:
      idx2_ExitIf(false, "Zero dimensional input\n");
      break;
    case 1:
      _Pragma("omp parallel for")
      for (X = Begin[0]; X < End[0]; X += Step[0])
        Kernel(nd_index(X, Y, Z, U, V, W));
      break;
    case 2:
      _Pragma("omp parallel for collapse(2)")
      for (Y = Begin[1]; Y < End[1]; Y += Step[1])
        for (X = Begin[0]; X < End[0]; X += Step[0])
          Kernel(nd_index(X, Y, Z, U, V, W));
      break;
    case 3:
      _Pragma("omp parallel for collapse(2)")
      for (Z = Begin[2]; Z < End[2]; Z += Step[2])
        for (Y = Begin[1]; Y < End[1]; Y += Step[1])
          for (X = Begin[0]; X < End[0]; X += Step[0])
            Kernel(nd_index(X, Y, Z, U, V, W));
      break;
    case 4:
      _Pragma("omp parallel for collapse(2)")
      for (U = Begin[3]; U < End[3]; U += Step[3])
        for (Z = Begin[2]; Z < End[2]; Z += Step[2])
          for (Y = Begin[1]; Y < End[1]; Y += Step[1])
            for (X = Begin[0]; X < End[0]; X += Step[0])
              Kernel(nd_index(X, Y, Z, U, V, W));
      break;
    case 5:
      _Pragma("omp parallel for collapse(2)")
      for (V = Begin[4]; V < End[4]; V += Step[4])
        for (U = Begin[3]; U < End[3]; U += Step[3])
          for (Z = Begin[2]; Z < End[2]; Z += Step[2])
            for (Y = Begin[1]; Y < End[1]; Y += Step[1])
              for (X = Begin[0]; X < End[0]; X += Step[0])
                Kernel(nd_index(X, Y, Z, U, V, W));
      break;
    case 6:
      _Pragma("omp parallel for collapse(2)")
      for (W = Begin[5]; W < End[5]; W += Step[5])
        for (V = Begin[4]; V < End[4]; V += Step[4])
          for (U = Begin[3]; U < End[3]; U += Step[3])
            for (Z = Begin[2]; Z < End[2]; Z += Step[2])
              for (Y = Begin[1]; Y < End[1]; Y += Step[1])
                for (X = Begin[0]; X < End[0]; X += Step[0])
                  Kernel(nd_index(X, Y, Z, U, V, W));
      break;
    default:
      idx2_Exit("Effective dimensionality greater than 6\n");
      break;
  };
}


/* Like ndLoop but exclude the fastest varying dimension (0) */
template <typename t> idx2_Inline void
ndOuterLoop(const nd_index& Begin, const nd_size& End, const nd_size& Step, const t& Kernel)
{
  i8 D = EffectiveDims(End);
  idx2_Assert(D <= 6);
  int X, Y, Z, U, V, W;
  switch (D)
  {
    case 0:
      idx2_Exit("Zero dimensional input\n");
      break;
    case 1:
      break;
    case 2:
      _Pragma("omp parallel for")
      for (Y = Begin[1]; Y < End[1]; Y += Step[1])
        Kernel(nd_index(X, Y, Z, U, V, W));
      break;
    case 3:
      _Pragma("omp parallel for collapse(2)")
      for (Z = Begin[2]; Z < End[2]; Z += Step[2])
        for (Y = Begin[1]; Y < End[1]; Y += Step[1])
          Kernel(nd_index(X, Y, Z, U, V, W));
      break;
    case 4:
      _Pragma("omp parallel for collapse(2)")
      for (U = Begin[3]; U < End[3]; U += Step[3])
        for (Z = Begin[2]; Z < End[2]; Z += Step[2])
          for (Y = Begin[1]; Y < End[1]; Y += Step[1])
            Kernel(nd_index(X, Y, Z, U, V, W));
      break;
    case 5:
      _Pragma("omp parallel for collapse(2)")
      for (V = Begin[4]; V < End[4]; V += Step[4])
        for (U = Begin[3]; U < End[3]; U += Step[3])
          for (Z = Begin[2]; Z < End[2]; Z += Step[2])
            for (Y = Begin[1]; Y < End[1]; Y += Step[1])
              Kernel(nd_index(X, Y, Z, U, V, W));
      break;
    case 6:
      _Pragma("omp parallel for collapse(2)")
      for (W = Begin[5]; W < End[5]; W += Step[5])
        for (V = Begin[4]; V < End[4]; V += Step[4])
          for (U = Begin[3]; U < End[3]; U += Step[3])
            for (Z = Begin[2]; Z < End[2]; Z += Step[2])
              for (Y = Begin[1]; Y < End[1]; Y += Step[1])
                Kernel(nd_index(X, Y, Z, U, V, W));
      break;
    default:
      idx2_Exit("Effective dimensionality greater than 6\n");
      break;
  };
}


} // namespace idx2
