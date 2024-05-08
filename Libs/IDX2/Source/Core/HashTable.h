// TODO: inlining some of the functions
// TODO: move the implementations out of this file into an .inl file
// TODO: before each operation, check if the table has been init

#pragma once

#include "Algorithm.h"
#include "Assert.h"
#include "Common.h"
#include "Macros.h"
#include "Math.h"
#include "Memory.h"


namespace idx2
{


template <typename k, typename v> struct hash_table
{
  enum bucket_status : u8
  {
    Empty,
    Tombstone,
    Occupied
  };
  k* Keys = nullptr;
  v* Vals = nullptr;
  bucket_status* Stats = nullptr;
  i64 Size = 0;
  i64 LogCapacity = 0;
  allocator* Alloc = nullptr;

  struct iterator
  {
    k* Key;
    v* Val;
    hash_table* Ht;
    i64 Idx;
    iterator& operator++();
    bool operator!=(const iterator& Other) const;
    bool operator==(const iterator& Other) const;
    operator bool() const;
    idx2_Inline v&
    operator*()
    {
      return *Val;
    }
  };

  idx2_Inline v&
  operator[](const k& Key)
  {
    auto It = Lookup(this, Key);
    if (!It)
      Insert(&It, Key, v());
    return *(It.Val);
  }

  idx2_Inline operator bool() const { return Keys != nullptr; }
};


template <typename k, typename v> u64
HeapSize(const hash_table<k, v>& HashTable)
{
  u64 Capacity = 1ull << HashTable.LogCapacity;
  return (sizeof(k) + sizeof(v) + sizeof(typename hash_table<k, v>::bucket_status)) * Capacity;
}


template <typename k, typename v> idx2_Inline bool
hash_table<k, v>::iterator::operator!=(const hash_table<k, v>::iterator& Other) const
{
  return Ht != Other.Ht || Idx != Other.Idx;
}


template <typename k, typename v> idx2_Inline bool
hash_table<k, v>::iterator::operator==(const hash_table<k, v>::iterator& Other) const
{
  return Ht == Other.Ht && Idx == Other.Idx;
}


template <typename k, typename v> idx2_Inline hash_table<k, v>::iterator::operator bool() const
{
  return Ht->Stats[Idx] == Occupied;
}


template <typename k, typename v> idx2_Inline typename hash_table<k, v>::iterator
IterAt(const hash_table<k, v>& Ht, i64 H)
{
  return typename hash_table<k, v>::iterator{
    &(Ht.Keys[H]), &(Ht.Vals[H]), &(const_cast<hash_table<k, v>&>(Ht)), H
  };
}


template <typename k, typename v> typename hash_table<k, v>::iterator
Begin(const hash_table<k, v>& Ht)
{
  i64 C = Capacity(Ht);
  for (i64 I = 0; I <= C; ++I)
  {
    if (Ht.Stats[I] == hash_table<k, v>::Occupied)
      return IterAt(Ht, I);
  }
  return IterAt(Ht, C);
}


template <typename k, typename v> typename hash_table<k, v>::iterator
End(const hash_table<k, v>& Ht)
{
  i64 C = Capacity(Ht);
  idx2_Assert((Ht.Stats[C] == hash_table<k, v>::Occupied));
  return IterAt(Ht, C);
}


template <typename k, typename v> idx2_Inline typename hash_table<k, v>::iterator&
hash_table<k, v>::iterator::operator++()
{
  do
  {
    ++Idx;
  } while (Ht->Stats[Idx] != hash_table<k, v>::Occupied);
  Key = &(Ht->Keys[Idx]);
  Val = &(Ht->Vals[Idx]);
  return *this;
}


template <typename k, typename v> void
Init(hash_table<k, v>* Ht, allocator* AllocIn = &Mallocator())
{
  Init(Ht, 8, AllocIn); // starts with size = 2^8 = 128
}


template <typename k, typename v> void
Init(hash_table<k, v>* Ht, i64 LogCapacityIn, allocator* AllocIn = &Mallocator())
{
  Ht->Alloc = AllocIn;
  Ht->LogCapacity = LogCapacityIn;
  i64 Capacity = 1ll << LogCapacityIn;
  AllocPtr(&Ht->Keys, Capacity + 1, AllocIn);
  AllocPtr(&Ht->Vals, Capacity + 1, AllocIn);
  AllocPtr(&Ht->Stats, Capacity + 1, AllocIn);
  Fill(Ht->Stats, Ht->Stats + Capacity, hash_table<k, v>::Empty);
  Ht->Stats[Capacity] = hash_table<k, v>::Occupied; // sentinel
}


template <typename k, typename v> void
Clear(hash_table<k, v>* Ht)
{
  Ht->Size = 0;
  i64 Capacity = 1ll << Ht->LogCapacity;
  for (i64 I = 0; I < Capacity; ++I)
  {
    Ht->Stats[I] = hash_table<k, v>::Empty;
  }
}


template <typename k, typename v> void
Dealloc(hash_table<k, v>* Ht)
{
  if (Ht->Alloc)
  {
    DeallocPtr(&Ht->Keys, Ht->Alloc);
    DeallocPtr(&Ht->Vals, Ht->Alloc);
    DeallocPtr(&Ht->Stats, Ht->Alloc);
    Ht->Size = Ht->LogCapacity = 0;
    Ht->Alloc = nullptr;
  }
}


template <typename k, typename v> idx2_Inline i64
Size(const hash_table<k, v>& Ht)
{
  return Ht.Size;
}


template <typename k, typename v> idx2_Inline i64
Capacity(const hash_table<k, v>& Ht)
{
  return 1ll << Ht.LogCapacity;
}


// u64
// Hash(u64 Key) { // murmur3
//   Key ^= (Key >> 33);
//   Key *= 0xff51afd7ed558ccd;
//   Key ^= (Key >> 33);
//   key *= 0xc4ceb9fe1a85ec53;
//   Key ^= (Key >> 33);
//   return Key;
// }

// u32
// Hash(u32 Key) { // murmur3
//   Key ^= Key >> 16;
//   Key *= 0x85ebca6b;
//   Key ^= Key >> 13;
// 	 Key *= 0xc2b2ae35;
// 	 Key ^= Key >> 16;
//   return Key;
// }

// idx2_T(k) u64
// Hash(const k& Key) {

// }


idx2_Inline u64
Hash(u64 Key)
{
  return Key;
}


// NOTE: TODO: this is a 32-bit hash
idx2_Inline u32
Hash(cstr Key)
{                                   // FNV-1a (https://create.stephan-brumme.com/fnv-hash/)
  constexpr u32 Prime = 0x01000193; //   16777619
  u32 Hash = 0x811C9DC5;            // Seed: 2166136261
  while (*Key)
    Hash = (*Key++ ^ Hash) * Prime;
  return Hash;
}


idx2_Inline u32
Hash(const buffer& Buf)
{
  return Hash((cstr)Buf.Data);
}


template <typename k, typename v> idx2_Inline u64
Index(const hash_table<k, v>& Ht, u64 Key)
{ // Fibonacci hashing
  return (Key * 11400714819323198485llu) >> (64 - Ht.LogCapacity);
}


template <typename k, typename v> void
IncreaseCapacity(hash_table<k, v>* Ht)
{
  hash_table<k, v> NewHt;
  Init(&NewHt, Ht->LogCapacity + 1, Ht->Alloc);
  for (auto It = Begin(*Ht); It != End(*Ht); ++It)
    Insert(&NewHt, *(It.Key), *(It.Val));
  Dealloc(Ht);
  *Ht = NewHt;
}


template <typename k, typename v> typename hash_table<k, v>::iterator
IncreaseCapacity(hash_table<k, v>* Ht, const typename hash_table<k, v>::iterator& ItIn)
{
  auto ItOut = ItIn;
  hash_table<k, v> NewHt;
  Init(&NewHt, Ht->LogCapacity + 1, Ht->Alloc);
  for (auto It = Begin(*Ht); It != End(*Ht); ++It)
  {
    auto Result = Insert(&NewHt, *(It.Key), *(It.Val));
    if (*(ItIn.Key) == *(Result.Key))
      ItOut = Result;
  }
  Dealloc(Ht);
  *Ht = NewHt;
  ItOut.Ht = Ht;
  return ItOut;
}


template <typename k, typename v> typename hash_table<k, v>::iterator
Insert(hash_table<k, v>* Ht, const k& Key, const v& Val)
{
  if (Size(*Ht) * 10 >= Capacity(*Ht) * 7)
    IncreaseCapacity(Ht);

  i64 H = Index(*Ht, Hash(Key));
  while (Ht->Stats[H] == hash_table<k, v>::Occupied && !(Ht->Keys[H] == Key))
  {
    ++H;
    H &= Capacity(*Ht) - 1;
  }

  Ht->Keys[H] = Key;
  Ht->Vals[H] = Val;
  if (Ht->Stats[H] != hash_table<k, v>::Occupied)
  {
    ++Ht->Size;
    Ht->Stats[H] = hash_table<k, v>::Occupied;
  }

  return IterAt(*Ht, H);
}


/* Insert a new element "inplace" without performing lookup again */
template <typename k, typename v> void
Insert(typename hash_table<k, v>::iterator* It, const k& Key, const v& Val)
{
  idx2_Assert((*It) != End(*(It->Ht)));
  *(It->Key) = Key;
  *(It->Val) = Val;
  It->Ht->Stats[It->Idx] = hash_table<k, v>::Occupied;
  ++It->Ht->Size;

  if (Size(*(It->Ht)) * 10 >= Capacity(*(It->Ht)) * 7)
  {
    *(It) = IncreaseCapacity(It->Ht, *It);
  }
}


template <typename k, typename v> typename hash_table<k, v>::iterator
Lookup(const hash_table<k, v>& Ht, const k& Key)
{
  i64 H = Index(Ht, Hash(Key));
  i64 Start = H;
  bool Found = false;
  while (Ht.Stats[H] != hash_table<k, v>::Empty)
  { // either Occupied or Tombstone
    if (Ht.Keys[H] == Key)
    {
      Found = true;
      break;
    }
    ++H;
    if ((H &= Capacity(Ht) - 1) == Start)
      break;
  }
  if (!Found)
  {
    while (Ht.Stats[H] == hash_table<k, v>::Occupied)
    {
      ++H;
      H &= Capacity(Ht) - 1;
    }
  }
  auto Result = IterAt(Ht, H);
  if (Found)
  {
    idx2_Assert(*Result.Key == Key);
  }
  return Result;
}


/* Count how long a chain is */
template <typename k, typename v> i64
Probe(hash_table<k, v>* Ht, const k& Key)
{
  i64 Length = 0;
  i64 H = Index(Ht, Hash(Key));
  i64 Start = H;
  while (Ht->Stats[H] != hash_table<k, v>::Empty)
  { // either Occupied or Tombstone
    if (Ht->Keys[H] == Key)
      break;
    ++H;
    if ((H &= Capacity(*Ht) - 1) == Start)
      break;
    ++Length;
  }
  return Length;
}


template <typename k, typename v> typename hash_table<k, v>::iterator
Delete(hash_table<k, v>* Ht, const k& Key)
{
  i64 H = Index(*Ht, Hash(Key));
  i64 Start = H;
  bool Found = false;
  while (Ht->Stats[H] != hash_table<k, v>::Empty)
  {
    if (Ht->Keys[H] == Key)
    {
      Ht->Stats[H] = hash_table<k, v>::Tombstone;
      --Ht->Size;
      idx2_Assert(Ht->Size >= 0);
      Found = true;
      return IterAt(*Ht, H);
    }
    ++H;
    if ((H &= Capacity(*Ht) - 1) == Start)
      break;
  }
  if (Found)
  {
    idx2_Assert(Ht->Keys[H] == Key);
  }
  idx2_Assert(Found);
  return End(*Ht);
}


/* Note: this does not do "deep" cloning */
template <typename k, typename v> void
Clone(const hash_table<k, v>& Src, hash_table<k, v>* Dst)
{
  Dealloc(Dst);
  Dst->Alloc = Src.Alloc;
  Dst->Size = Src.Size;
  Dst->LogCapacity = Src.LogCapacity;
  i64 Capacity = 1ll << Dst->LogCapacity;
  AllocPtr(&Dst->Keys, Capacity + 1, Dst->Alloc);
  AllocPtr(&Dst->Vals, Capacity + 1, Dst->Alloc);
  AllocPtr(&Dst->Stats, Capacity + 1, Dst->Alloc);
  memcpy(Dst->Keys, Src.Keys, sizeof(k) * (Capacity + 1));
  memcpy(Dst->Vals, Src.Vals, sizeof(v) * (Capacity + 1));
  memcpy(Dst->Stats, Src.Stats, sizeof(typename hash_table<k, v>::bucket_status) * (Capacity + 1));
}


} // end namespace idx2

