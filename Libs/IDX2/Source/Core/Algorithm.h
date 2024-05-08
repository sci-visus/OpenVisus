#pragma once

#include "Macros.h"


namespace idx2
{


template <typename t> t Min(const t& a, const t& b);
template <typename t> t Min(const t& a, const t& b, const t& c);

template <typename t> t Max(const t& a, const t& b);
template <typename t> t Max(const t& a, const t& b, const t& c);

template <typename i> i MaxElem(i Beg, i End);
template <typename i, typename f> i MaxElem(i Beg, i End, const f& Comp);

template <typename i> struct min_max
{
  i Min, Max;
};
template <typename i> min_max<i> MinMaxElem(i Beg, i End);
template <typename i, typename f> min_max<i> MinMaxElem(i Beg, i End, const f& Comp);

template <typename i, typename t> i Find(i Beg, i End, const t& Val);
template <typename i, typename t> i FindLast(i RevBeg, i RevEnd, const t& Val);
template <typename i, typename f> i FindIf(i Beg, i End, const f& Pred);

template <typename t1, typename t2> bool Contains(const t1& Collection, const t2& Elem);

template <typename i> void InsertionSort(i Beg, i End);

template <typename i> bool AreSame(i Beg1, i End1, i Beg2, i End2);

template <typename t> constexpr void Swap(t* A, t* B);
template <typename i> constexpr void IterSwap(i A, i B);

template <typename i, typename t> void Fill(i Beg, i End, const t& Val);

/* Only work with random access iterator */
template <typename i> void Reverse(i Beg, i End);

template <typename i> int FwdDist(i Beg, i End);


} // namespace idx2



namespace idx2
{


template <typename t> idx2_Inline t
Min(const t& a, const t& b)
{
  return b < a ? b : a;
}


template <typename t> idx2_Inline t
Max(const t& a, const t& b)
{
  return a < b ? b : a;
}


template <typename t> idx2_Inline t
Min(const t& a, const t& b, const t& c)
{
  return a < b ? Min(c, a) : Min(b, c);
}


template <typename t> idx2_Inline t
Max(const t& a, const t& b, const t& c)
{
  return a < b ? Max(b, c) : Max(a, c);
}


template <typename i> i
MaxElem(i Beg, i End)
{
  auto MaxElem = Beg;
  for (i Pos = Beg; Pos != End; ++Pos)
  {
    if (*MaxElem < *Pos)
      MaxElem = Pos;
  }
  return MaxElem;
}


template <typename i, typename f> i
MaxElem(i Beg, i End, f& Comp)
{
  auto MaxElem = Beg;
  for (i Pos = Beg; Pos != End; ++Pos)
  {
    if (Comp(*MaxElem, *Pos))
      MaxElem = Pos;
  }
  return MaxElem;
}


template <typename i> min_max<i>
MinMaxElem(i Beg, i End)
{
  auto MinElem = Beg;
  auto MaxElem = Beg;
  for (i Pos = Beg; Pos != End; ++Pos)
  {
    if (*Pos < *MinElem)
      MinElem = Pos;
    else if (*Pos > *MaxElem)
      MaxElem = Pos;
  }
  return min_max<i>{ MinElem, MaxElem };
}


template <typename i, typename f> min_max<i>
MinMaxElem(i Beg, i End, const f& Comp)
{
  auto MinElem = Beg;
  auto MaxElem = Beg;
  for (i Pos = Beg; Pos != End; ++Pos)
  {
    if (Comp(*Pos, *MinElem))
      MinElem = Pos;
    else if (Comp(*MaxElem, *Pos))
      MaxElem = Pos;
  }
  return min_max<i>{ MinElem, MaxElem };
}


template <typename i, typename t> i
Find(i Beg, i End, const t& Val)
{
  for (i Pos = Beg; Pos != End; ++Pos)
  {
    if (*Pos == Val)
      return Pos;
  }
  return End;
}


template <typename i, typename t> i
FindLast(i RevBeg, i RevEnd, const t& Val)
{
  for (i Pos = RevBeg; Pos != RevEnd; --Pos)
  {
    if (*Pos == Val)
      return Pos;
  }
  return RevEnd;
}


template <typename t1, typename t2> idx2_Inline bool
Contains(const t1& Collection, const t2& Elem)
{
  return Find(Begin(Collection), End(Collection), Elem) != End(Collection);
}


template <typename i, typename f> i
FindIf(i Beg, i End, const f& Pred)
{
  for (i Pos = Beg; Pos != End; ++Pos)
  {
    if (Pred(*Pos))
      return Pos;
  }
  return End;
}


/*
Return a position where the value is Val.
If there is no such position, return the first position k such that A[k] > Val
*/
template <typename t, typename i> i
BinarySearch(i Beg, i End, const t& Val)
{
  while (Beg < End)
  {
    i Mid = Beg + (End - Beg) / 2;
    if (*Mid < Val)
    {
      Beg = Mid + 1;
      continue;
    }
    else if (Val < *Mid)
    {
      End = Mid;
      continue;
    }
    return Mid;
  }
  return End;
}


template <typename i> void
InsertionSort(i Beg, i End)
{
  if (Beg == End)
    return;
  i Last = Beg + 1;
  while (Last != End)
  {
    i Pos = BinarySearch(Beg, Last, *Last);
    for (i It = Last; It != Pos; --It)
      Swap(It, It - 1);
    ++Last;
  }
}


template <typename i> bool
AreSame(i Beg1, i End1, i Beg2)
{
  bool Same = true;
  for (i It1 = Beg1, It2 = Beg2; It1 != End1; ++It1, ++It2)
  {
    if (!(*It1 == *It2))
      return false;
  }
  return Same;
}


template <typename t> idx2_Inline constexpr void
Swap(t* A, t* idx2_Restrict B)
{
  t T = *A;
  *A = *B;
  *B = T;
}


template <typename i> idx2_Inline constexpr void
IterSwap(i A, i B)
{
  Swap(&(*A), &(*B));
}


template <typename i, typename t> void
Fill(i Beg, i End, const t& Val)
{
  for (i It = Beg; It != End; ++It)
    *It = Val;
}


template <typename i> void
Reverse(i Beg, i End)
{
  auto It1 = Beg;
  auto It2 = End - 1;
  while (It1 < It2)
  {
    Swap(It1, It2);
    ++It1;
    --It2;
  }
}


template <typename i> int
FwdDist(i Beg, i End)
{
  int Dist = 0;
  while (Beg != End)
  {
    ++Dist;
    ++Beg;
  }
  return Dist;
}


} // namespace idx2
