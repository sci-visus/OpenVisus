#pragma once

#include "BitStream.h"
#include "CircularQueue.h"
#include "Common.h"
#include <string.h>


namespace idx2
{


idx2_Inline int
WriteVarByte(bitstream* Bs, u64 Val)
{
  int BytesWritten = 0;
  while (++BytesWritten)
  {
    Write(Bs, Val & 0x7F, 7);
    Val >>= 7;
    Write(Bs, Val != 0);
    if (Val == 0)
      break;
  }
  return BytesWritten;
}


idx2_Inline u64
ReadVarByte(bitstream* Bs)
{
  u64 Val = 0;
  int Shift = 0;
  while (true)
  {
    Val = (Read(Bs, 7) << (7 * Shift++)) + Val;
    if (Read(Bs) == 0)
      break;
  }
  return Val;
}


// TODO: for faster decoding, use i64[] instead of bitstream, and
// flush even when the control sequence is 240 zeros or 120 zeros
struct simple8b
{
  static constexpr u8 IntsCoded[] = { 240, 120, 60, 30, 20, 15, 12, 10, 8, 7, 6, 5, 4, 3, 2, 1 };
  static constexpr u8 BitsPerInt[] = { 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 15, 20, 30, 60 };
  bitstream Stream;
  circular_queue<u32, 256> SavedVals;
  u8 NSavedVals = 0;
  i8 MaxBitsIdx = 0;
};

void
Write(simple8b* S8b, u32 Val);

void
FlushVals(simple8b* S8b);

void
FlushStream(simple8b* S8b);

void
Rewind(simple8b* S8b);

int
BitCount(const simple8b& S8b);

int
ByteCount(const simple8b& S8b);

u32
Read(simple8b* S8b);

void
WriteUnary(bitstream* Bs, u32 Val);


idx2_Inline void
WriteUnary(bitstream* Bs, u32 Val)
{

  //  while (Val > 57) {
  //    Write(Bs, (u64)0, 57); // write at most 57 0-bits
  //    Val -= 57;
  //  }
  //  Write(Bs, (u64)0, Val); // write Val 0 bits
  //  Write(Bs, 1); // write last 1 bit
  u32 Len = idx2_BitSizeOf(Bs->BitBuf) - Bs->BitPos;
  if (Val > Len)
  {
    WriteLong(Bs, (u64)0, Len);
    Val -= Len;
    FlushAndMoveToNextByte(Bs);
    int ByteLen = Val >> 3;
    memset(Bs->BitPtr, 0, ByteLen);
    Bs->BitPtr += ByteLen;
    Write(Bs, (u64)0, Val & 0x7);
  }
  else
  {
    WriteLong(Bs, (u64)0, Val);
  }
  Write(Bs, 1);
}


idx2_Inline u32
ReadUnary(bitstream* Bs)
{
  u32 V = 0;
  while (!Read(Bs))
    ++V;
  return V;
}


} // namespace idx2
