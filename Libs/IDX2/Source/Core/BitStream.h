// TODO: this code currently works only on little-endian machine. make sure it
// works for both types of endianess
// TODO: bound checking?

/*
A bit stream. LSB bits are written first. Bytes are written in little-endian order.
*/
#pragma once

#include "Common.h"
#include "Memory.h"


namespace idx2
{


/* Support only either reading or writing, not both at the same time */
struct bitstream
{
  buffer Stream = {};
  byte* BitPtr = nullptr; // Pointer to current byte
  u64 BitBuf = 0;         // buffer
  int BitPos = 0;         // how many of those bits we've consumed/written

  inline static stack_array<u64, 65> Masks = []()
  {
    stack_array<u64, 65> Masks;
    for (int I = 0; I < 64; ++I)
      Masks[I] = (u64(1) << I) - 1;
    Masks[64] = ~u64(0);
    return Masks;
  }();
};

void Rewind(bitstream* Bs);
i64 Size(const bitstream& Bs);
i64 BitSize(const bitstream& Bs);
int BufferSize(const bitstream& Bs);
/* ---------------- Read functions ---------------- */
void InitRead(bitstream* Bs, const buffer& Stream);
/* Refill our buffer (replace the consumed bytes with new bytes from memory) */
void Refill(bitstream* Bs);
/*
Peek the next "Count" bits from the buffer without consuming them
(Count <= 64 - BitPos). This is often called after Refill(). */
u64 Peek(bitstream* Bs, int Count = 1);
/* Consume the next "Count" bits from the buffer (Count <= 64 - 7).
This is often called after Refill() and potentially Peek(). */
void Consume(bitstream* Bs, int Count = 1);
/*
Extract "Count" bits from the stream (Count <= 64 - 7). This performs at most
one Refill() call. The restriction on Count is due to the fact that Refill()
works in units of bytes, so at most 7 already consumed bits can be left over. */
u64 Read(bitstream* Bs, int Count = 1);
/* Similar to Read() but Count is less restrictive (Count <= 64) */
u64 ReadLong(bitstream* Bs, int Count);

/* ---------------- Write functions ---------------- */
void InitWrite(bitstream* Bs, i64 Bytes, allocator* Alloc = &Mallocator());
void InitWrite(bitstream* Bs, const buffer& Buf);
/* Flush the written BYTES in our buffer to memory */
void Flush(bitstream* Bs);
/* Flush and move the pointer to the next byte in memory */
void FlushAndMoveToNextByte(bitstream* Bs);
/* Put "Count" bits into the buffer (Count <= 64 - BitPos) */
void Put(bitstream* Bs, u64 N, int Count = 1);
/* Write "Count" bits into the stream (Count <= 64 - 7) */
u64 Write(bitstream* Bs, u64 N, int Count = 1);
/* Similar to Write() but Count is less restrictive (Count <= 64) */
u64 WriteLong(bitstream* Bs, u64 N, int Count);
/* Flush and write stream Src to Bs, at byte granularity */
void WriteStream(bitstream* Bs, bitstream* Src);
void WriteBuffer(bitstream* Bs, const buffer& Src);
/* Write "Count" bits into the stream (Count >= 0) */
void RepeatedWrite(bitstream* Bs, bool B, int Count);
/* Zero out the whole buffer */
void Zero(bitstream* Bs);
/* Pad the stream with 0s until a specified number of bits */
void Pad0sUntil(bitstream* Bs, i64 BitCount);

/* Grow the underlying buffer if it is somewhat full */
void GrowIfTooFull(bitstream* Bs);
void GrowToAccomodate(bitstream* Bs, i64 AddedCapacity);
/* Grow the underlying buffer */
void IncreaseCapacity(bitstream* Bs, i64 NewCapacity);

/* Seek to a given byte offset from the start of the stream */
void SeekToByte(bitstream* Bs, i64 ByteOffset);
void SeekToNextByte(bitstream* Bs);
/* Seek to a given bit offset from the start of the stream */
void SeekToBit(bitstream* Bs, i64 BitOffset);
buffer ToBuffer(const bitstream& Bs);

// Only call this if the bit stream itself manages its memory
void Dealloc(bitstream* Bs);


} // namespace idx2



#include "Algorithm.h"
#include "Assert.h"
#include "Macros.h"


namespace idx2
{


idx2_Inline void
Rewind(bitstream* Bs)
{
  Bs->BitPtr = Bs->Stream.Data;
  Bs->BitBuf = Bs->BitPos = 0;
}


idx2_Inline i64
Size(const bitstream& Bs)
{
  return (Bs.BitPtr - Bs.Stream.Data) + (Bs.BitPos + 7) / 8;
}


idx2_Inline i64
BitSize(const bitstream& Bs)
{
  return (Bs.BitPtr - Bs.Stream.Data) * 8 + Bs.BitPos;
}


idx2_Inline int
BufferSize(const bitstream& Bs)
{
  return sizeof(Bs.BitBuf);
}


idx2_Inline void
InitRead(bitstream* Bs, const buffer& Stream)
{
  idx2_Assert(!Stream.Data || Stream.Bytes > 0);
  Bs->Stream = Stream;
  Rewind(Bs);
  Refill(Bs);
}


idx2_Inline void
Refill(bitstream* Bs)
{
  idx2_Assert(Bs->BitPos <= 64);
  Bs->BitPtr += Bs->BitPos >> 3;  // ignore the bytes we've consumed
  Bs->BitBuf = *(u64*)Bs->BitPtr; // refill
  Bs->BitPos &= 7;                // (% 8) left over bits that don't make a full byte
}


idx2_Inline u64
Peek(bitstream* Bs, int Count)
{
  idx2_Assert(Count >= 0 && Bs->BitPos + Count <= 64);
  u64 Remaining = Bs->BitBuf >> Bs->BitPos;   // the bits we have not consumed
  return Remaining & bitstream::Masks[Count]; // return the bottom count bits
}


idx2_Inline void
Consume(bitstream* Bs, int Count)
{
  idx2_Assert(Count + Bs->BitPos <= 64);
  Bs->BitPos += Count;
}


idx2_Inline u64
Read(bitstream* Bs, int Count)
{
  idx2_Assert(Count >= 0 && Count <= 64 - 7);
  if (Count + Bs->BitPos > 64)
    Refill(Bs);
  u64 Result = Peek(Bs, Count);
  Consume(Bs, Count);
  return Result;
}


idx2_Inline u64
ReadLong(bitstream* Bs, int Count)
{
  idx2_Assert(Count >= 0 && Count <= 64);
  int FirstBatchCount = Min(Count, 64 - Bs->BitPos);
  u64 Result = Peek(Bs, FirstBatchCount);
  Consume(Bs, FirstBatchCount);
  if (Count > FirstBatchCount)
  {
    Refill(Bs);
    Result |= Peek(Bs, Count - FirstBatchCount) << FirstBatchCount;
    Consume(Bs, Count - FirstBatchCount);
  }
  return Result;
}


idx2_Inline void
InitWrite(bitstream* Bs, const buffer& Buf)
{
  idx2_Assert((size_t)Buf.Bytes >= sizeof(Bs->BitBuf));
  Bs->Stream = Buf;
  Bs->BitPtr = Buf.Data;
  Bs->BitBuf = Bs->BitPos = 0;
}


idx2_Inline void
InitWrite(bitstream* Bs, i64 Bytes, allocator* Alloc)
{
  Alloc->Alloc(&Bs->Stream, Bytes + sizeof(Bs->BitBuf));
  Bs->BitPtr = Bs->Stream.Data;
  Bs->BitBuf = Bs->BitPos = 0;
}


idx2_Inline void
Flush(bitstream* Bs)
{
  idx2_Assert(Bs->BitPos <= 64);
  /* write the buffer to memory */
  *(u64*)Bs->BitPtr = Bs->BitBuf; // TODO: make sure this write is in little-endian
  int BytePos = Bs->BitPos >> 3;  // number of bytes in the buffer we have used
  /* shift the buffer to the right (the convoluted logic is to avoid shifting by 64 bits) */
  if (BytePos > 0)
    Bs->BitBuf = (Bs->BitBuf >> 1) >> ((BytePos << 3) - 1);
  Bs->BitPtr += BytePos; // advance the pointer
  Bs->BitPos &= 7;       // % 8
}


idx2_Inline void
FlushAndMoveToNextByte(bitstream* Bs)
{
  *(u64*)Bs->BitPtr = Bs->BitBuf;
  int BytePos = Bs->BitPos >> 3;
  Bs->BitPtr += BytePos + ((Bs->BitPos & 0x7) != 0); // advance the pointer
  Bs->BitBuf = Bs->BitPos = 0;
}


idx2_Inline void
Put(bitstream* Bs, u64 N, int Count)
{
  idx2_Assert(Count >= 0 && Bs->BitPos + Count <= 64);
  Bs->BitBuf |= (N & bitstream::Masks[Count]) << Bs->BitPos;
  Bs->BitPos += Count;
}


idx2_Inline u64
Write(bitstream* Bs, u64 N, int Count)
{
  idx2_Assert(Count >= 0 && Count <= 64 - 7);
  if (Count + Bs->BitPos >= 64)
    Flush(Bs);
  Put(Bs, N, Count);
  return N;
}


idx2_Inline u64
WriteLong(bitstream* Bs, u64 N, int Count)
{
  idx2_Assert(Count >= 0 && Count <= 64);
  int FirstBatchCount = Min(Count, 64 - Bs->BitPos);
  Put(Bs, N, FirstBatchCount);
  if (Count > FirstBatchCount)
  {
    Flush(Bs);
    Put(Bs, N >> FirstBatchCount, Count - FirstBatchCount);
  }
  return N;
}


inline void
WriteStream(bitstream* Bs, bitstream* Src)
{
  FlushAndMoveToNextByte(Bs);
  Flush(Src);
  buffer Dst = Bs->Stream + Size(*Bs);
  MemCopy(ToBuffer(*Src), &Dst);
  Bs->BitPtr += Size(*Src);
}


inline void
WriteBuffer(bitstream* Bs, const buffer& Src)
{
  FlushAndMoveToNextByte(Bs);
  buffer Dst = Bs->Stream + Size(*Bs);
  MemCopy(Src, &Dst);
  Bs->BitPtr += Size(Src);
}


idx2_Inline void
RepeatedWrite(bitstream* Bs, bool B, int Count)
{
  idx2_Assert(Count >= 0);
  u64 N = ~(u64(B) - 1);
  if (Count <= 64 - 7)
  { // write at most 57 bits
    Write(Bs, N, Count);
  }
  else
  { // write more than 57 bits
    while (true)
    {
      int NBits = 64 - Bs->BitPos;
      if (NBits <= Count)
      {
        Put(Bs, N, NBits);
        Count -= NBits;
        Flush(Bs);
      }
      else
      {
        Put(Bs, N, Count);
        break;
      }
    }
  }
}


idx2_Inline void
SeekToByte(bitstream* Bs, i64 ByteOffset)
{
  Bs->BitPtr = Bs->Stream.Data + ByteOffset;
  Bs->BitBuf = *(u64*)Bs->BitPtr; // refill
  Bs->BitPos = 0;
}


idx2_Inline void
SeekToNextByte(bitstream* Bs)
{
  SeekToByte(Bs, Bs->BitPtr - Bs->Stream.Data + ((Bs->BitPos + 7) >> 3));
}


idx2_Inline void
SeekToBit(bitstream* Bs, i64 BitOffset)
{
  Bs->BitPtr = Bs->Stream.Data + (BitOffset >> 3);
  Bs->BitBuf = *(u64*)Bs->BitPtr; // refill
  Bs->BitPos = (BitOffset & 7);   // (% 8)
}


idx2_Inline buffer
ToBuffer(const bitstream& Bs)
{
  return buffer{ Bs.Stream.Data, Size(Bs), Bs.Stream.Alloc };
}


idx2_Inline void
Dealloc(bitstream* Bs)
{
  Bs->Stream.Alloc->Dealloc(&(Bs->Stream));
  Bs->BitPtr = Bs->Stream.Data;
  Bs->BitBuf = Bs->BitPos = 0;
}


idx2_Inline void
Zero(bitstream* Bs)
{
  ZeroBuf(&(Bs->Stream));
  Bs->BitBuf = 0;
}


idx2_Inline void
Pad0sUntil(bitstream* Bs, i64 BitCount)
{
  RepeatedWrite(Bs, false, int(BitCount - BitSize(*Bs)));
}


idx2_Inline void
GrowIfTooFull(bitstream* Bs)
{
  if (Size(*Bs) * 10 > Size(Bs->Stream) * 8)
  { // we grow at 80% capacity
    auto NewCapacity = (Size(Bs->Stream) * 3) / 2 + 8;
    IncreaseCapacity(Bs, NewCapacity);
  }
}


idx2_Inline void
GrowToAccomodate(bitstream* Bs, i64 AddedCapacity)
{
  i64 OriginalCapacity = Size(Bs->Stream);
  i64 NewCapacity = OriginalCapacity;
  while (Size(*Bs) + AddedCapacity + (i64)sizeof(Bs->BitBuf) >= NewCapacity)
    NewCapacity = (NewCapacity * 3) / 2 + 8;
  if (NewCapacity > OriginalCapacity)
    IncreaseCapacity(Bs, NewCapacity);
}


idx2_Inline void
IncreaseCapacity(bitstream* Bs, i64 NewCapacity)
{
  NewCapacity += sizeof(Bs->BitBuf);
  if (Size(Bs->Stream) < NewCapacity)
  {
    buffer NewBuf;
    AllocBuf(&NewBuf, NewCapacity, Bs->Stream.Alloc);
    MemCopy(Bs->Stream, &NewBuf, Size(*Bs));
    Bs->BitPtr = (Bs->BitPtr - Bs->Stream.Data) + NewBuf.Data;
    DeallocBuf(&Bs->Stream);
    Bs->Stream = NewBuf;
  }
}


} // namespace idx2
