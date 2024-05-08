#include "VarInt.h"
#include "Algorithm.h"
#include "Common.h"
#include "Math.h"

// TODO: make some functions inline

namespace idx2
{


// Base-128 varint
// Encode Vals in groups of 64 bits each, with 4-bit control
void
Write(simple8b* S8b, u32 Val)
{
  i8 MaxBitsIdx = S8b->MaxBitsIdx;
  i8 NewMaxBits = Val > 0 ? Log2Ceil(Val + 1) : 0;
  while (simple8b::BitsPerInt[MaxBitsIdx] < NewMaxBits)
    ++MaxBitsIdx;
  bool Overflow = Size(S8b->SavedVals) >= simple8b::IntsCoded[MaxBitsIdx];
  if (Overflow)
  { // overflow
    FlushVals(S8b);
    Write(S8b, Val);
  }
  else
  { // not overflow
    S8b->MaxBitsIdx = MaxBitsIdx;
    PushBack(&(S8b->SavedVals), Val);
  }
}


void
FlushVals(simple8b* S8b)
{
  if (S8b->MaxBitsIdx == 0 && Size(S8b->SavedVals) >= simple8b::IntsCoded[0])
  { // flush 240 zeros
    idx2_Assert(Size(S8b->SavedVals) == simple8b::IntsCoded[0]);
    Write(&(S8b->Stream), 0, 4);
    PopFront(&(S8b->SavedVals), Size(S8b->SavedVals));
  }
  else if (S8b->MaxBitsIdx == 0 && Size(S8b->SavedVals) >= simple8b::IntsCoded[1])
  { // flush 120 zeros
    Write(&(S8b->Stream), 1, 4);
    PopFront(&(S8b->SavedVals), simple8b::IntsCoded[1]);
  }
  else
  { // flush all saved vals
    while (Size(S8b->SavedVals) > 0)
    {
      i8 MaxBitsIdx = 0;
      while (Size(S8b->SavedVals) < simple8b::IntsCoded[MaxBitsIdx])
        ++MaxBitsIdx;
      Write(&(S8b->Stream), MaxBitsIdx, 4);
      for (i16 I = 0; I < simple8b::IntsCoded[MaxBitsIdx]; ++I)
        WriteLong(&(S8b->Stream), S8b->SavedVals[I], simple8b::BitsPerInt[MaxBitsIdx]);
      PopFront(&(S8b->SavedVals), simple8b::IntsCoded[MaxBitsIdx]);
    }
    S8b->MaxBitsIdx = 0;
  }
}


void
FlushStream(simple8b* S8b)
{
  Flush(&(S8b->Stream));
}


/* Read the next number from the stream */
u32
Read(simple8b* S8b)
{
  if (S8b->NSavedVals == 0)
  { // just starting a new sequence
    S8b->MaxBitsIdx = (i8)Read(&(S8b->Stream), 4);
    S8b->NSavedVals = simple8b::IntsCoded[S8b->MaxBitsIdx];
    idx2_Assert(S8b->NSavedVals > 0);
    --S8b->NSavedVals;
    return (S8b->MaxBitsIdx < 2) ? 0
                                 : (u32)Read(&(S8b->Stream), simple8b::BitsPerInt[S8b->MaxBitsIdx]);
  }
  else
  { // continue extracting values from the current sequence
    --S8b->NSavedVals;
    return (S8b->MaxBitsIdx < 2) ? 0
                                 : (u32)Read(&(S8b->Stream), simple8b::BitsPerInt[S8b->MaxBitsIdx]);
  }
}


// TODO: should we empty the SavedVals here?
void
Rewind(simple8b* S8b)
{
  Rewind(&(S8b->Stream));
  S8b->NSavedVals = S8b->MaxBitsIdx = 0;
  Clear(&(S8b->SavedVals));
}


int
BitCount(const simple8b& S8b)
{
  if (S8b.MaxBitsIdx == 0 && Size(S8b.SavedVals) >= simple8b::IntsCoded[0])
  { // 240 zeros
    idx2_Assert(Size(S8b.SavedVals) == simple8b::IntsCoded[0]);
    return (int)BitSize(S8b.Stream) + 4;
  }
  else if (S8b.MaxBitsIdx == 0 && Size(S8b.SavedVals) >= simple8b::IntsCoded[1])
  { // 120 zeros
    return (int)BitSize(S8b.Stream) + 4;
  }
  int Result = (int)BitSize(S8b.Stream);
  i8 S = (i8)Size(S8b.SavedVals);
  while (S > 0)
  {
    i8 MaxBitsIdx = 0;
    while (S < simple8b::IntsCoded[MaxBitsIdx])
      ++MaxBitsIdx;
    Result += 4 + simple8b::IntsCoded[MaxBitsIdx] * simple8b::BitsPerInt[MaxBitsIdx];
    S -= simple8b::IntsCoded[MaxBitsIdx];
  }
  return Result;
}


int
ByteCount(const simple8b& S8b)
{
  return (int)RoundUp(BitCount(S8b), 8) / 8;
}

// TODO: we can read faster by checking for the trailing zeros

u32
ReadUnaryWithBoundaryCheck(bitstream* Bs)
{
  u32 V = 0;
  while (true)
  {
    if (BitSize(*Bs) >= Size(Bs->Stream) * 8)
      return (u32)-1;
    if (!Read(Bs))
      ++V;
    else
      break;
  }
  return V;
}

// TODO: write golomb-2 encoder

} // namespace idx2
