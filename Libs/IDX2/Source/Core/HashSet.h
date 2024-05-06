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


template <typename k> struct hash_set
{
  enum bucket_status : u8
  {
    Empty,
    Tombstone,
    Occupied
  };
  k* Keys = nullptr;
  bucket_status* Stats = nullptr;
  i64 Size = 0;
  i64 LogCapacity = 0;
  allocator* Alloc = nullptr;

  struct iterator
  {
    k* Key;
    hash_set* Hs;
    i64 Idx;
    iterator& operator++();
    bool operator!=(const iterator& Other) const;
    bool operator==(const iterator& Other) const;
    operator bool() const;
    idx2_Inline k&
    operator*()
    {
      return *Key;
    }
  };

  idx2_Inline operator bool() const { return Keys != nullptr; }
};


template <typename k> u64
HeapSize(const hash_set<k>& HashSet)
{
  u64 Capacity = 1ull << HashSet.LogCapacity;
  return (sizeof(k) + sizeof(typename hash_set<k>::bucket_status)) * Capacity;
}


/* Compare two iterators for inequality */
template <typename k> idx2_Inline bool
hash_set<k>::iterator::operator!=(const hash_set<k>::iterator& Other) const
{
  return Hs != Other.Hs || Idx != Other.Idx;
}


/* Compare two iterators for equality */
template <typename k> idx2_Inline bool
hash_set<k>::iterator::operator==(const hash_set<k>::iterator& Other) const
{
  return Hs == Other.Hs && Idx == Other.Idx;
}


/* Check if an iterator is "occupied" */
template <typename k> idx2_Inline hash_set<k>::iterator::operator bool() const
{
  return Hs->Stats[Idx] == Occupied;
}


/* Create an iterator at position H */
template <typename k> idx2_Inline typename hash_set<k>::iterator
IterAt(const hash_set<k>& Ht, i64 H)
{
  return typename hash_set<k>::iterator{ &(Ht.Keys[H]), &(const_cast<hash_set<k>&>(Ht)), H };
}


/* Begin iterator */
template <typename k> typename hash_set<k>::iterator
Begin(const hash_set<k>& Hs)
{
  i64 C = Capacity(Hs);
  for (i64 I = 0; I <= C; ++I)
  {
    if (Hs.Stats[I] == hash_set<k>::Occupied)
      return IterAt(Hs, I);
  }
  return IterAt(Hs, C);
}


/* End iterator */
template <typename k> typename hash_set<k>::iterator
End(const hash_set<k>& Hs)
{
  i64 C = Capacity(Hs);
  idx2_Assert((Ht.Stats[C] == hash_set<k>::Occupied));
  return IterAt(Hs, C);
}


/* Advance iterator */
template <typename k> idx2_Inline typename hash_set<k>::iterator&
hash_set<k>::iterator::operator++()
{
  do
  {
    ++Idx;
  } while (Hs->Stats[Idx] != hash_set<k>::Occupied);
  Key = &(Hs->Keys[Idx]);
  return *this;
}


/* Init with a default capacity */
template <typename k> void
Init(hash_set<k>* Hs, allocator* AllocIn = &Mallocator())
{
  Init(Hs, 8, AllocIn); // starts with size = 2^8 = 128
}


/* Init with a given capacity */
template <typename k> void
Init(hash_set<k>* Hs, i64 LogCapacityIn, allocator* AllocIn = &Mallocator())
{
  Hs->Alloc = AllocIn;
  Hs->LogCapacity = LogCapacityIn;
  i64 Capacity = 1ll << LogCapacityIn;
  AllocPtr(&Hs->Keys, Capacity + 1, AllocIn);
  AllocPtr(&Hs->Stats, Capacity + 1, AllocIn);
  Fill(Hs->Stats, Hs->Stats + Capacity, hash_set<k>::Empty);
  Hs->Stats[Capacity] = hash_set<k>::Occupied; // sentinel
}


/* Clear the hash set (but not deallocate it) */
template <typename k> void
Clear(hash_set<k>* Hs)
{
  Hs->Size = 0;
  i64 Capacity = 1ll << Hs->LogCapacity;
  for (i64 I = 0; I < Capacity; ++I)
  {
    Hs->Stats[I] = hash_set<k>::Empty;
  }
}


/* Deallocate the hash set */
template <typename k> void
Dealloc(hash_set<k>* Hs)
{
  if (Hs->Alloc)
  {
    DeallocPtr(&Hs->Keys, Hs->Alloc);
    DeallocPtr(&Hs->Stats, Hs->Alloc);
    Hs->Size = Hs->LogCapacity = 0;
    Hs->Alloc = nullptr;
  }
}


/* Return the size of the hash set */
template <typename k> idx2_Inline i64
Size(const hash_set<k>& Hs)
{
  return Hs.Size;
}


/* Return the capacity of the hash set */
template <typename k> idx2_Inline i64
Capacity(const hash_set<k>& Hs)
{
  return 1ll << Hs.LogCapacity;
}


/* Hash a 64-bit key */
idx2_Inline u64
Hash(u64 Key)
{
  return Key;
}


/* Hash a null-terminated string */
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


/* Hash a buffer representing a string */
idx2_Inline u32
Hash(const buffer& Buf)
{
  return Hash((cstr)Buf.Data);
}


/* Given a key, return an index in the hash set */
template <typename k> idx2_Inline u64
Index(hash_set<k>* Hs, u64 Key)
{ // Fibonacci hashing
  return (Key * 11400714819323198485llu) >> (64 - Hs->LogCapacity);
}


/* Increase the capacity of a hash set */
template <typename k> void
IncreaseCapacity(hash_set<k>* Hs)
{
  hash_set<k> NewHs;
  Init(&NewHs, Hs->LogCapacity + 1, Hs->Alloc);
  for (auto It = Begin(*Hs); It != End(*Hs); ++It)
    Insert(&NewHs, *(It.Key), *(It.Val));
  Dealloc(Hs);
  *Hs = NewHs;
}


/* Increase the capacity of a hash set while making sure an iterator is valid */
template <typename k> typename hash_set<k>::iterator
IncreaseCapacity(hash_set<k>* Hs, const typename hash_set<k>::iterator& ItIn)
{
  auto ItOut = ItIn;
  hash_set<k> NewHs;
  Init(&NewHs, Hs->LogCapacity + 1, Hs->Alloc);
  for (auto It = Begin(*Hs); It != End(*Hs); ++It)
  {
    auto Result = Insert(&NewHs, *(It.Key));
    if (*(ItIn.Key) == *(Result.Key))
      ItOut = Result;
  }
  Dealloc(Hs);
  *Hs = NewHs;
  ItOut.Hs = Hs;
  return ItOut;
}


/* Insert a key and return an iterator to its position */
template <typename k> typename hash_set<k>::iterator
Insert(hash_set<k>* Hs, const k& Key)
{
  if (Size(*Hs) * 10 >= Capacity(*Hs) * 7)
    IncreaseCapacity(Hs);

  i64 H = Index(Hs, Hash(Key));
  while (Hs->Stats[H] == hash_set<k>::Occupied && !(Hs->Keys[H] == Key))
  {
    ++H;
    H &= Capacity(*Hs) - 1;
  }

  Hs->Keys[H] = Key;
  if (Hs->Stats[H] != hash_set<k>::Occupied)
  {
    ++Hs->Size;
    Hs->Stats[H] = hash_set<k>::Occupied;
  }

  return IterAt(*Hs, H);
}


/* Insert a key into a hash set through an iterator */
template <typename k> void
Insert(typename hash_set<k>::iterator* It, const k& Key)
{
  idx2_Assert((*It) != End(*(It->Hs)));
  *(It->Key) = Key;
  It->Hs->Stats[It->Idx] = hash_set<k>::Occupied;
  ++It->Hs->Size;

  if (Size(*(It->Hs)) * 10 >= Capacity(*(It->Hs)) * 7)
  {
    *(It) = IncreaseCapacity(It->Hs, *It);
  }
}


/* Lookup a key */
template <typename k> typename hash_set<k>::iterator
Lookup(hash_set<k>* Hs, const k& Key)
{
  i64 H = Index(Hs, Hash(Key));
  i64 Start = H;
  bool Found = false;
  while (Hs->Stats[H] != hash_set<k>::Empty)
  { // either Occupied or Tombstone
    if (Hs->Keys[H] == Key)
    {
      Found = true;
      break;
    }
    ++H;
    if ((H &= Capacity(*Hs) - 1) == Start)
      break;
  }
  if (!Found)
  {
    while (Hs->Stats[H] == hash_set<k>::Occupied)
    {
      ++H;
      H &= Capacity(*Hs) - 1;
    }
  }
  auto Result = IterAt(*Hs, H);
  if (Found)
  {
    idx2_Assert(*Result.Key == Key);
  }
  return Result;
}


/* Count how long a chain is */
template <typename k> i64
Probe(hash_set<k>* Hs, const k& Key)
{
  i64 Length = 0;
  i64 H = Index(Hs, Hash(Key));
  i64 Start = H;
  while (Hs->Stats[H] != hash_set<k>::Empty)
  { // either Occupied or Tombstone
    if (Hs->Keys[H] == Key)
      break;
    ++H;
    if ((H &= Capacity(*Hs) - 1) == Start)
      break;
    ++Length;
  }
  return Length;
}


/* Delete a key */
template <typename k> typename hash_set<k>::iterator
Delete(hash_set<k>* Hs, const k& Key)
{
  i64 H = Index(Hs, Hash(Key));
  i64 Start = H;
  bool Found = false;
  while (Hs->Stats[H] != hash_set<k>::Empty)
  {
    if (Hs->Keys[H] == Key)
    {
      Hs->Stats[H] = hash_set<k>::Tombstone;
      --Hs->Size;
      idx2_Assert(Hs->Size >= 0);
      Found = true;
      return IterAt(*Hs, H);
    }
    ++H;
    if ((H &= Capacity(*Hs) - 1) == Start)
      break;
  }
  if (Found)
  {
    idx2_Assert(Hs->Keys[H] == Key);
  }
  idx2_Assert(Found);
  return End(*Hs);
}


/* Note: this does not do "deep" cloning */
template <typename k> void
Clone(const hash_set<k>& Src, hash_set<k>* Dst)
{
  Dealloc(Dst);
  Dst->Alloc = Src.Alloc;
  Dst->Size = Src.Size;
  Dst->LogCapacity = Src.LogCapacity;
  i64 Capacity = 1ll << Dst->LogCapacity;
  AllocPtr(&Dst->Keys, Capacity + 1, Dst->Alloc);
  AllocPtr(&Dst->Stats, Capacity + 1, Dst->Alloc);
  memcpy(Dst->Keys, Src.Keys, sizeof(k) * (Capacity + 1));
  memcpy(Dst->Stats, Src.Stats, sizeof(typename hash_set<k>::bucket_status) * (Capacity + 1));
}


} // end namespace idx2
