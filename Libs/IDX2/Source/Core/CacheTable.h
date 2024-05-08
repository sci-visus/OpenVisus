#if (0) // this is not fully written -- we are using a third-party library instead

#pragma once

#include "Algorithm.h"
#include "Assert.h"
#include "Common.h"
#include "Macros.h"
#include "Math.h"
#include "Memory.h"


namespace idx2
{


template <typename k, typename v> struct cache_table
{
  struct item_key
  {
    k Key;
    i8 LastUsed = 0; // will be increased every time the item is used
  };

  struct slot
  {
    item_key* Keys = nullptr;
    i8 LastUsed = 0; // the last used for the entire slot (row)
    i8 NItems = 0; // number of items in this slot (row)
  };

  slot* Keys = nullptr; // should be of size 1 << (LogCapacity - LogAssociativity)
  v* Vals = nullptr; // should be of size 1 << LogCapacity
  i64 Size = 0;
  i8 LogCapacity = 0;
  i8 LogAssociativity = 0;
  allocator* Alloc = nullptr;

  struct iterator
  {
    k* Key;
    v* Val;
    cache_table* Ht;
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


template <typename k, typename v> idx2_Inline bool
cache_table<k, v>::iterator::operator!=(const cache_table<k, v>::iterator& Other) const
{
  return Ht != Other.Ht || Idx != Other.Idx;
}


template <typename k, typename v> idx2_Inline bool
cache_table<k, v>::iterator::operator==(const cache_table<k, v>::iterator& Other) const
{
  return Ht == Other.Ht && Idx == Other.Idx;
}


template <typename k, typename v> idx2_Inline cache_table<k, v>::iterator::operator bool() const
{
  return Ht->Stats[Idx] == Occupied;
}


template <typename k, typename v> idx2_Inline typename cache_table<k, v>::iterator
IterAt(const cache_table<k, v>& Ht, i64 H)
{
  return typename cache_table<k, v>::iterator{
    &(Ht.Keys[H]), &(Ht.Vals[H]), &(const_cast<cache_table<k, v>&>(Ht)), H
  };
}


template <typename k, typename v> typename cache_table<k, v>::iterator
Begin(const cache_table<k, v>& Ht)
{
  i64 C = Capacity(Ht);
  for (i64 I = 0; I <= C; ++I)
  {
    if (Ht.Stats[I] == cache_table<k, v>::Occupied)
      return IterAt(Ht, I);
  }
  return IterAt(Ht, C);
}


template <typename k, typename v> typename cache_table<k, v>::iterator
End(const cache_table<k, v>& Ht)
{
  i64 C = Capacity(Ht);
  idx2_Assert((Ht.Stats[C] == cache_table<k, v>::Occupied));
  return IterAt(Ht, C);
}


template <typename k, typename v> idx2_Inline typename cache_table<k, v>::iterator&
cache_table<k, v>::iterator::operator++()
{
  do
  {
    ++Idx;
  } while (Ht->Stats[Idx] != cache_table<k, v>::Occupied);
  Key = &(Ht->Keys[Idx]);
  Val = &(Ht->Vals[Idx]);
  return *this;
}


template <typename k, typename v> void
Init(cache_table<k, v>* Ht, i8 LogCapacity, i8 LogAssociativity, allocator* AllocIn = &Mallocator())
{
  idx2_Assert(LogAssociativity <= LogCapacity);
  Ht->Alloc = AllocIn;
  Ht->LogCapacity = LogCapacity;
  i64 Capacity = 1ll << LogCapacity;
  AllocPtr(&Ht->Keys, Capacity + 1, AllocIn);
  Fill(Ht->Keys, Ht->Keys + Capacity + 1, cache_table<k, v>::slot());
  AllocPtr(&Ht->Vals, Capacity + 1, AllocIn);
}


template <typename k, typename v> void
Clear(cache_table<k, v>* Ht)
{
  Ht->Size = 0;
  i64 Capacity = 1ll << Ht->LogCapacity;
  for (i64 I = 0; I < Capacity; ++I)
  {
    Ht->Stats[I] = cache_table<k, v>::Empty;
  }
}


template <typename k, typename v> void
Dealloc(cache_table<k, v>* Ht)
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
Size(const cache_table<k, v>& Ht)
{
  return Ht.Size;
}


template <typename k, typename v> idx2_Inline i64
Capacity(const cache_table<k, v>& Ht)
{
  return 1ll << Ht.LogCapacity;
}


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


// This gives the row (slot) to put the item
template <typename k, typename v> idx2_Inline u64
Index(const cache_table<k, v>& Ht, u64 Key)
{ // Fibonacci hashing
  // take the first LogCapacity bits after the multiplication
  return (Key * 11400714819323198485llu) >> (64 - (Ht.LogCapacity - Ht.LogAssociativity));
}


template <typename k, typename v> typename cache_table<k, v>::iterator
Insert(cache_table<k, v>* Ht, const k& Key, const v& Val)
{
  // TODO: handle swapping to disk (or just evict)
  i64 H = Index(*Ht, Hash(Key));
  cache_table<k, v>::slot& Slot = Ht->Keys[H];
  i8 IndexToInsert = 0;
  /* if the slot is full */
  if (Slot.NItems == (1 << Ht->LogAssociativity))
  {
    // find the least recently used (LRU) item (the one with LastUsed most further from the slot's LastUsed)
    i8 LRU = 0;
    int MaxDiff = 0;
    for (i8 I = 0; I <  Slot.NItems; ++I)
    {
      int Diff = abs(int(Slot.Keys[I].LastUsed) - int(Slot.LastUsed));
      if (Diff > MaxDiff)
      {
        MaxDiff = Diff;
        LRU = I;
      }
    }
    IndexToInsert = LRU;
    // TODO: evict the item to disk?
  }
  else // slot is not full, find a place to put the item
  {
    i8 I = 0;
    while (!(Slot.Keys[I].Key == Key) && (I < Slot.NItems))
      ++I;
    IndexToInsert = I;
  }

  Slot.Keys[IndexToInsert] = Key;
  Ht->Vals[H * (1 << cache_table<k, v>::LogAssociativity) + IndexToInsert] = Val;

  return IterAt(*Ht, H);
}



} // end namespace idx2

#endif
