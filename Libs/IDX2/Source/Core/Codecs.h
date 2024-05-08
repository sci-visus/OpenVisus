#pragma once

#include "Assert.h"
#include "BitOps.h"
#include "BitStream.h"
#include "Common.h"


namespace idx2
{


/* v is from 0 to n-1 */
void EncodeCenteredMinimal(u32 v, u32 n, bitstream* Bs);
u32 DecodeCenteredMinimal(u32 n, bitstream* Bs);


} // namespace idx2



namespace idx2
{


inline void
EncodeCenteredMinimal(u32 v, u32 n, bitstream* Bs)
{
  idx2_Assert(n > 0);
  idx2_Assert(v < n);
  if (n == 2)
  {
    Write(Bs, v == 1);
    return;
  }
  else if (n == 1)
  {
    return;
  }
  u32 l1 = Msb(n);
  u32 l2 = ((1 << l1) == n) ? l1 : l1 + 1;
  u32 d = (1 << l2) - n;
  u32 m = (n - d) / 2;
  if (v < m)
  {
    v = BitReverse(v);
    v >>= sizeof(v) * 8 - l2;
    Write(Bs, v, l2);
  }
  else if (v >= m + d)
  {
    v = BitReverse(v - d);
    v >>= sizeof(v) * 8 - l2;
    Write(Bs, v, l2);
  }
  else
  { // middle
    v = BitReverse(v);
    v >>= sizeof(v) * 8 - l1;
    Write(Bs, v, l1);
  }
}


inline u32
DecodeCenteredMinimal(u32 n, bitstream* Bs)
{
  idx2_Assert(n > 0);
  if (n == 2)
  {
    return (u32)Read(Bs);
  }
  u32 l1 = Msb(n);
  u32 l2 = ((1 << l1) == n) ? l1 : l1 + 1;
  u32 d = (1 << l2) - n;
  u32 m = (n - d) / 2;
  Refill(Bs); // TODO: minimize the number of refill
  u32 v = (u32)Peek(Bs, l2);
  v <<= sizeof(v) * 8 - l2;
  v = BitReverse(v);
  if (v < m)
  {
    Consume(Bs, l2);
    return v;
  }
  else if (v < 2 * m)
  {
    Consume(Bs, l2);
    return v + d;
  }
  else
  {
    Consume(Bs, l1);
    return v >> 1;
  }
}


} // namespace idx2
