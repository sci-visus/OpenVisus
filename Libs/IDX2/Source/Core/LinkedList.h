#pragma once

#include "Common.h"
#include "Macros.h"
#include "Memory.h"


namespace idx2
{


template <typename t> struct list_node
{
  t Payload;
  list_node* Next = nullptr;
};


template <typename t> struct list
{
  list_node<t>* Head = nullptr;
  allocator* Alloc = nullptr;
  i64 Size = 0;
  list(allocator* Alloc = &Mallocator());
};


template <typename t> struct list_iterator
{
  list_node<t>* Node = nullptr;
  list_iterator& operator++();
  list_node<t>* operator->() const;
  t& operator*() const;
  bool operator!=(const list_iterator& Other);
  bool operator==(const list_iterator& Other);
};


template <typename t> list_iterator<t>
Begin(const list<t>& List);

template <typename t> list_iterator<t>
End(const list<t>& List);

template <typename t> list_iterator<t>
Insert(list<t>* List, const list_iterator<t>& Where, const t& Payload);

template <typename t> list_iterator<t>
PushBack(list<t>* List, const t& Payload);

template <typename t> void
Dealloc(list<t>* List);

template <typename t> i64
Size(const list<t>& List);


} // namespace idx2



#include "Assert.h"


namespace idx2
{


template <typename t> idx2_Inline
list<t>::list(allocator* Alloc)
  : Alloc(Alloc)
{
}


template <typename t> list_iterator<t>
Insert(list<t>* List, const list_iterator<t>& Where, const t& Payload)
{
  buffer Buf;
  List->Alloc->Alloc(&Buf, sizeof(list_node<t>));
  list_node<t>* NewNode = (list_node<t>*)Buf.Data;
  NewNode->Payload = Payload;
  NewNode->Next = nullptr;
  if (Where.Node)
  {
    NewNode->Next = Where->Next;
    Where->Next = NewNode;
  }
  ++List->Size;
  return list_iterator<t>{ NewNode };
}


template <typename t> list_iterator<t>
PushBack(list<t>* List, const t& Payload)
{
  auto Node = List->Head;
  list_node<t>* Prev = nullptr;
  while (Node)
  {
    Prev = Node;
    Node = Node->Next;
  }
  auto NewNode = Insert(List, list_iterator<t>{ Prev }, Payload);
  if (!Prev) // this new node is the first node in the list
    List->Head = NewNode.Node;
  return NewNode;
}


template <typename t> void
Dealloc(list<t>* List)
{
  auto Node = List->Head;
  while (Node)
  {
    buffer Buf((byte*)Node, sizeof(list_node<t>), List->Alloc);
    Node = Node->Next;
    List->Alloc->Dealloc(&Buf);
  }
  List->Head = nullptr;
  List->Size = 0;
}


template <typename t> idx2_Inline i64
Size(const list<t>& List)
{
  return List.Size;
}


template <typename t> idx2_Inline list_iterator<t>&
list_iterator<t>::operator++()
{
  idx2_Assert(Node);
  Node = Node->Next;
  return *this;
}


template <typename t> idx2_Inline list_node<t>*
list_iterator<t>::operator->() const
{
  idx2_Assert(Node);
  return const_cast<list_node<t>*>(Node);
}


template <typename t> idx2_Inline t&
list_iterator<t>::operator*() const
{
  idx2_Assert(Node);
  return const_cast<t&>(Node->Payload);
}


template <typename t> idx2_Inline bool
list_iterator<t>::operator!=(const list_iterator<t>& Other)
{
  return Node != Other.Node;
}


template <typename t> idx2_Inline bool
list_iterator<t>::operator==(const list_iterator<t>& Other)
{
  return Node == Other.Node;
}


template <typename t> idx2_Inline list_iterator<t>
Begin(const list<t>& List)
{
  return list_iterator<t>{ List.Head };
}


template <typename t> idx2_Inline list_iterator<t>
End(const list<t>& List)
{
  (void)List;
  return list_iterator<t>();
}


} // namespace idx2
