#include "Zfp.h"
#include "Algorithm.h"
#include "BitStream.h"


namespace idx2
{


const v3i ZDims(4, 4, 4);


/* Only return true if the block is fully encoded */
bool
Encode(u64* Block, int B, i64 S, i8& N, i8& M, bool& In, bitstream* Bs)
{
  idx2_Assert(N <= 64);
  u64 X = 0;
  for (int I = M; I < 64; ++I)
    X += u64((Block[I] >> B) & 1u) << (I - M);
  i8 P = (i8)Min(i64(N - M), S - BitSize(*Bs));
  if (P > 0)
  {
    WriteLong(Bs, X, P);
    X >>= P; // P == 64 is fine since in that case we don't need X any more
  }
  u64 Lb = 1;
  if (In)
    goto INNER_LOOP;
  // TODO: we may be able to speed this up by getting rid of the shift of X
  // or the call bit BitSize()
  for (; BitSize(*Bs) < S && N < 64;)
  {
    if (1 == (Lb = Write(Bs, !!X)))
    {
    INNER_LOOP:
      for (; BitSize(*Bs) < S && N < 64 - 1;)
      {
        if (Write(Bs, X & 1u))
        {
          break;
        }
        else
        {
          X >>= 1;
          ++N;
          ++P;
        }
      }
      if (BitSize(*Bs) >= S)
      {
        In = true;
        break;
      }
      X >>= 1;
      ++N;
      ++P;
    }
    else
    {
      break;
    }
  }
  idx2_Assert(N <= 64);
  M += P;
  return ((N == 64 && M == N) || Lb == 0);
}


/* Only return true if the block is fully decoded */
bool
Decode(u64* Block, int B, i64 S, i8& N, i8& M, bool& In, bitstream* Bs)
{
  i8 P = (i8)Min(i64(N - M), S - BitSize(*Bs));
  u64 X = P > 0 ? ReadLong(Bs, P) : 0;
  u64 Lb = 1;
  if (In)
    goto INNER_LOOP;
  for (; BitSize(*Bs) < S && N < 64;)
  {
    if (1 == (Lb = Read(Bs)))
    {
    INNER_LOOP:
      for (; BitSize(*Bs) < S && N < 64 - 1;)
      {
        if (Read(Bs))
        {
          break;
        }
        else
        {
          ++N;
          ++P;
        }
      }
      if (BitSize(*Bs) >= S)
      {
        In = true;
        break;
      }
      X += 1ull << (P++);
      ++N;
    }
    else
    {
      break;
    }
  }
  /* deposit bit plane from x */
  for (int I = M; X; ++I, X >>= 1)
    Block[I] += (u64)(X & 1u) << B;
  M += P;
  return ((N == 64 && M == N) || Lb == 0);
}


bool
Encode(u64* Block, int B, i64 S, i8& N, i8& M, bitstream* Bs)
{
  idx2_Assert(N <= 64);
  u64 X = 0;
  for (int I = M; I < 64; ++I)
    X += u64((Block[I] >> B) & 1u) << (I - M);
  i8 P = (i8)Min(i64(N - M), S - BitSize(*Bs));
  if (P > 0)
  {
    WriteLong(Bs, X, P);
    X >>= P; // P == 64 is fine since in that case we don't need X any more
  }
  u64 Lb = 1;
  // TODO: we may be able to speed this up by getting rid of the shift of X
  // or the call bit BitSize()
  for (; BitSize(*Bs) < S && N < 64;)
  {
    if (1 == (Lb = Write(Bs, !!X)))
    { // group is significant
      for (; BitSize(*Bs) < S && N < 64 - 1;)
      {
        if (Write(Bs, X & 1u))
        { // found a significant coeff, break and retest
          break;
        }
        else
        { // have not found a significant coeff, continue until we find one
          X >>= 1;
          ++N;
          ++P;
        }
      }
      if (BitSize(*Bs) >= S)
        break;
      X >>= 1;
      ++N;
      ++P;
    }
    else
    {
      break;
    }
  }
  idx2_Assert(N <= 64);
  M += P;
  return ((N == 64 && M == N) || Lb == 0);
}


/* Only return true if the block is fully decoded */
bool
Decode(u64* Block, int B, i64 S, i8& N, i8& M, bitstream* Bs)
{
  i8 P = (i8)Min(i64(N - M), S - BitSize(*Bs));
  u64 X = P > 0 ? ReadLong(Bs, P) : 0;
  u64 Lb = 1;
  for (; BitSize(*Bs) < S && N < 64;)
  {
    if (1 == (Lb = Read(Bs)))
    {
      for (; BitSize(*Bs) < S && N < 64 - 1;)
      {
        if (Read(Bs))
        {
          break;
        }
        else
        {
          ++N;
          ++P;
        }
      }
      if (BitSize(*Bs) >= S)
      {
        break;
      }
      X += 1ull << (P++);
      ++N;
    }
    else
    {
      break;
    }
  }
  /* deposit bit plane from x */
  for (int I = M; X; ++I, X >>= 1)
    Block[I] += (u64)(X & 1u) << B;
  M += P;
  return ((N == 64 && M == N) || Lb == 0);
}


} // namespace idx2
