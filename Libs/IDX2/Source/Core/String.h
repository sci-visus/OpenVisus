/* String processing utilities */

#pragma once

#include "Common.h"


namespace idx2
{


/* Useful to create a string_ref out of a literal string */
#define idx2_StRef(x) idx2::stref((x), sizeof(x) - 1)

/*
A "view" into a (usually bigger) null-terminated string. A string_ref itself is not null-terminated.
There are two preferred ways to construct a string_ref from a char[] array:
  - Use the idx2_StringRef macro to make string_ref refer to the entire array
  - Use the string_ref(const char*) constructor to refer up to the first NULL
*/
struct stref
{
  union
  {
    str Ptr = nullptr;
    cstr ConstPtr;
  };
  int Size = 0;

  stref();
  stref(cstr PtrIn, int SizeIn);
  stref(cstr PtrIn);
  char& operator[](int Idx) const;
  operator bool() const;
}; // struct string_ref


int
Size(const stref& Str);

cstr
ToString(const stref& Str);

str
Begin(stref Str);

str
End(stref Str);

str
RevBegin(stref Str);

str
RevEnd(stref Str);

bool
operator==(const stref& Lhs, const stref& Rhs);

bool
operator!=(const stref& Lhs, const stref& Rhs);

/* Remove spaces at the start of a string */
stref
TrimLeft(const stref& Str);

stref
TrimRight(const stref& Str);

stref
Trim(const stref& Str);

/*
Return a substring of a given string. The substring starts at Begin and has
length Size. Return the empty string if no proper substring can be constructed
(e.g. Begin >= Str.Size).
*/
stref
SubString(const stref& Str, int Begin, int Size);

/*
Copy the underlying buffer referred to by Src to the one referred to by Dst.
AddNull should be true whenever dst represents a whole string (as opposed to a
substring). If Src is larger than Dst, we copy as many characters as we can. We
always assume that the null character can be optionally added without
overflowing the memory of Dst.
*/
void
Copy(const stref& Src, stref* Dst, bool AddNull = true);

/* Parse a string_ref and return a number */
bool
ToInt(const stref& Str, int* Result);

bool
ToInt64(const stref& Str, i64* Result);

bool
ToDouble(const stref& Str, f64* Result);

/* Tokenize strings without allocating memory */
struct tokenizer
{
  stref Input;
  stref Delims;
  int Pos = 0;

  tokenizer();
  tokenizer(const stref& InputIn, const stref& DelimsIn = " \n\t");
}; // struct tokenizer

void
Init(tokenizer* Tk, const stref& Input, const stref& Delims = " \n\t");
stref
Next(tokenizer* Tk);
void
Reset(tokenizer* Tk);


} // namespace idx2



#include "Macros.h"
#include <assert.h>
#include <string.h>


namespace idx2
{


idx2_Inline
stref::stref() = default;


idx2_Inline
stref::stref(cstr PtrIn, int SizeIn)
  : ConstPtr(PtrIn)
  , Size(SizeIn)
{
}


idx2_Inline
stref::stref(cstr PtrIn)
  : ConstPtr(PtrIn)
  , Size(int(strlen(PtrIn)))
{
}


idx2_Inline char&
stref::operator[](int Idx) const
{
  assert(Idx < Size);
  return const_cast<char&>(Ptr[Idx]);
}


idx2_Inline stref::operator bool() const
{
  return Ptr != nullptr;
}


idx2_Inline int
Size(const stref& Str)
{
  return Str.Size;
}


idx2_Inline str
Begin(stref Str)
{
  return Str.Ptr;
}


idx2_Inline str
End(stref Str)
{
  return Str.Ptr + Str.Size;
}


idx2_Inline str
RevBegin(stref Str)
{
  return Str.Ptr + Str.Size - 1;
}


idx2_Inline str
RevEnd(stref Str)
{
  return Str.Ptr - 1;
}

idx2_Inline
tokenizer::tokenizer() = default;


idx2_Inline
tokenizer::tokenizer(const stref& InputIn, const stref& DelimsIn)
  : Input(InputIn)
  , Delims(DelimsIn)
  , Pos(0)
{
}


idx2_Inline void
Init(tokenizer* Tk, const stref& Input, const stref& Delims)
{
  Tk->Input = Input;
  Tk->Delims = Delims;
  Tk->Pos = 0;
}


} // namespace idx2
