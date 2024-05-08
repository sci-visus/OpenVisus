#pragma once

#include "Common.h"
#include "Macros.h"
#include "Memory.h"
#include <initializer_list>
#include <string.h>


namespace idx2
{


/*
Only works for POD types. For other types, use std::vector.
NOTE: elements must be explicitly initialized (they are not initialized to zero
or anything)
NOTE: do not make copies of a dynamic_array using operator=, then work on them
as if they were independent from the original object (i.e., an array does not
assume ownership of its memory buffer)
*/
template <typename t> struct array
{
  buffer Buffer = {};
  i64 Size = 0;
  i64 Capacity = 0;
  allocator* Alloc = nullptr;
  array(allocator* Alloc = &Mallocator());
  array(const std::initializer_list<t>& List, allocator* Alloc = &Mallocator());
  t& operator[](i64 Idx) const;
};

template <typename t> void Init(array<t>* Array, i64 Size);
template <typename t> void Init(array<t>* Array, i64 Size, const t& Val);

template <typename t> i64 Size(const array<t>& Array);

template <typename t> t& Front(const array<t>& Array);
template <typename t> t& Back(const array<t>& Array);
template <typename t> t* Begin(const array<t>& Array);
template <typename t> t* End(const array<t>& Array);

template <typename t> void Clone(const array<t>& Src, array<t>* Dst);
template <typename t> void Relocate(array<t>* Array, buffer Buf);

template <typename t> void GrowCapacity(array<t>* Array, i64 NewCapacity = 0);
template <typename t> void Resize(array<t>* Array, i64 NewSize);
template <typename t> void Reserve(array<t>* Array, i64 Capacity);
template <typename t> void Clear(array<t>* Array);

template <typename t> void PushBack(array<t>* Array, const t& Item);
template <typename t> void PushBack(array<t>* Array, const t* Items, i64 NItems);
template <typename t> void PushBack(array<t>* Array);
template <typename t> void PopBack(array<t>* Array);
template <typename t> buffer ToBuffer(const array<t>& Array);

template <typename t> void Dealloc(array<t>* Array);


} // namespace idx2



#include "Algorithm.h"
#include "Assert.h"


namespace idx2
{


template <typename t> idx2_Inline
array<t>::array(allocator* Alloc)
  : Buffer()
  , Size(0)
  , Capacity(0)
  , Alloc(Alloc)
{
  idx2_Assert(Alloc);
}


template <typename t> idx2_Inline
array<t>::array(const std::initializer_list<t>& List, allocator* Alloc)
  : array(Alloc)
{
  idx2_Assert(Alloc);
  Init(this, List.size());
  int Count = 0;
  for (const auto& Elem : List)
    (*this)[Count++] = Elem;
}


template <typename t> idx2_Inline t&
array<t>::operator[](i64 Idx) const
{
  idx2_Assert(Idx < Size);
  return const_cast<t&>(((t*)Buffer.Data)[Idx]);
}


template <typename t> idx2_Inline void
Init(array<t>* Array, i64 Size)
{
  Array->Alloc->Alloc(&Array->Buffer, Size * sizeof(t));
  Array->Size = Array->Capacity = Size;
}


template <typename t> idx2_Inline void
Init(array<t>* Array, i64 Size, const t& Val)
{
  Init(Array, Size);
  Fill((t*)Array->Buffer.Data, (t*)Array->Buffer.Data + Size, Val);
}


template <typename t> idx2_Inline i64
Size(const array<t>& Array)
{
  return Array.Size;
}


template <typename t> idx2_Inline t&
Front(const array<t>& Array)
{
  idx2_Assert(Size(Array) > 0);
  return const_cast<t&>(Array[0]);
}


template <typename t> idx2_Inline t&
Back(const array<t>& Array)
{
  idx2_Assert(Size(Array) > 0);
  return const_cast<t&>(Array[Size(Array) - 1]);
}


template <typename t> idx2_Inline t*
Begin(const array<t>& Array)
{
  return (t*)const_cast<byte*>(Array.Buffer.Data);
}


template <typename t> idx2_Inline t*
End(const array<t>& Array)
{
  return (t*)const_cast<byte*>(Array.Buffer.Data) + Array.Size;
}


template <typename t> void
Relocate(array<t>* Array, buffer Buf)
{
  MemCopy(Array->Buffer, &Buf);
  Array->Alloc->Dealloc(&Array->Buffer);
  Array->Buffer = Buf;
  Array->Capacity = Buf.Bytes / sizeof(t);
  idx2_Assert(Array->Size <= Array->Capacity);
}


template <typename t> void
GrowCapacity(array<t>* Array, i64 NewCapacity)
{
  if (NewCapacity == 0) // default
    NewCapacity = Array->Capacity * 3 / 2 + 8;
  if (Array->Capacity < NewCapacity)
  {
    buffer Buf;
    Array->Alloc->Alloc(&Buf, NewCapacity * sizeof(t));
    idx2_Assert(Buf);
    Relocate(Array, Buf);
    Array->Capacity = NewCapacity;
  }
}


template <typename t> idx2_Inline void
PushBack(array<t>* Array, const t& Item)
{
  if (Array->Size >= Array->Capacity)
    GrowCapacity(Array);
  (*Array)[Array->Size++] = Item;
}

template <typename t> idx2_Inline void
PushBack(array<t>* Array, const t* Items, i64 NItems)
{
  i64 Size = Array->Size;
  i64 NewCapacity = Array->Capacity;
  while (Size + NItems >= NewCapacity)
    NewCapacity = NewCapacity * 3 / 2 + 8;
  GrowCapacity(Array, NewCapacity);
  memcpy(&Array->Buffer[Size], Items, NItems * sizeof(t));
  Array->Size = Size + NItems;
}


template <typename t> idx2_Inline void
PushBack(array<t>* Array)
{
  if (Array->Size >= Array->Capacity)
    GrowCapacity(Array);
  ++Array->Size;
}


template <typename t> idx2_Inline void
PopBack(array<t>* Array)
{
  if (Array->Size > 0)
    --Array->Size;
}


template <typename t> buffer
ToBuffer(const array<t>& Array)
{
  return buffer{ Array.Buffer.Data, Size(Array) * (i64)sizeof(t), Array.Buffer.Alloc };
}


template <typename t> idx2_Inline void
Clear(array<t>* Array)
{
  Array->Size = 0;
}


template <typename t> void
Resize(array<t>* Array, i64 NewSize)
{
  if (NewSize > Array->Capacity) // TODO: maybe grow the capacity until this is false
    GrowCapacity(Array, NewSize);
  if (Array->Size < NewSize)
    Fill(Begin(*Array) + Array->Size, Begin(*Array) + NewSize, t{});
  Array->Size = NewSize;
}


template <typename t> idx2_Inline void
Reserve(array<t>* Array, i64 Capacity)
{
  GrowCapacity(Array, Capacity);
}


// TODO: test to see if t is POD, if yes, just memcpy
template <typename t> void
Clone(const array<t>& Src, array<t>* Dst)
{
  Resize(Dst, Size(Src));
  for (int I = 0; I < Size(Src); ++I)
    (*Dst)[I] = Src[I];
}


template <typename t> idx2_Inline void
Dealloc(array<t>* Array)
{
  Array->Alloc->Dealloc(&Array->Buffer);
  Array->Size = Array->Capacity = 0;
}


} // namespace idx2
