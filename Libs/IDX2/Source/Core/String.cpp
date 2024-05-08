#include "String.h"
#include "Algorithm.h"
#include "Math.h"
#include "Memory.h"
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>


namespace idx2
{


cstr
ToString(const stref& Str)
{
  idx2_Assert(Str.Size < (int)sizeof(ScratchBuf));
  if (Str.Ptr != ScratchBuf)
    snprintf(ScratchBuf, sizeof(ScratchBuf), "%.*s", Str.Size, Str.Ptr);
  return ScratchBuf;
}


bool
operator==(const stref& Lhs, const stref& Rhs)
{
  stref& LhsR = const_cast<stref&>(Lhs);
  stref& RhsR = const_cast<stref&>(Rhs);
  if (LhsR.Size != RhsR.Size)
    return false;
  for (int I = 0; I < LhsR.Size; ++I)
  {
    if (LhsR[I] != RhsR[I])
      return false;
  }
  return true;
}


bool
operator!=(const stref& Lhs, const stref& Rhs)
{
  return !(Lhs == Rhs);
}


stref
TrimLeft(const stref& Str)
{
  stref StrOut = Str;
  while (StrOut.Size && isspace(*StrOut.Ptr))
  {
    ++StrOut.Ptr;
    --StrOut.Size;
  }
  return StrOut;
}


stref
TrimRight(const stref& Str)
{
  stref StrOut = Str;
  while (StrOut.Size && isspace(StrOut[StrOut.Size - 1]))
    --StrOut.Size;
  return StrOut;
}


stref
Trim(const stref& Str)
{
  return TrimLeft(TrimRight(Str));
}


stref
SubString(const stref& Str, int Begin, int Size)
{
  if (!Str || Begin >= Str.Size)
    return stref();
  return stref(Str.Ptr + Begin, Min(Size, Str.Size));
}


void
Copy(const stref& Src, stref* Dst, bool AddNull)
{
  int NumBytes = Min(Dst->Size, Src.Size);
  memcpy(Dst->Ptr, Src.Ptr, size_t(NumBytes));
  if (AddNull)
    Dst->Ptr[NumBytes] = 0;
}


bool
ToInt(const stref& Str, int* Result)
{
  stref& StrR = const_cast<stref&>(Str);
  if (!StrR || StrR.Size <= 0)
    return false;

  int Mult = 1, Start = 0;
  if (StrR[0] == '-')
  {
    Mult = -1;
    Start = 1;
  }
  *Result = 0;
  for (int I = 0; I < Str.Size - Start; ++I)
  {
    int V = StrR[StrR.Size - I - 1] - '0';
    if (V >= 0 && V < 10)
      *Result += Mult * (V * power<int, 10>()[I]);
    else
      return false;
  }
  return true;
}


bool
ToInt64(const stref& Str, i64* Result)
{
  stref& StrR = const_cast<stref&>(Str);
  if (!StrR || StrR.Size <= 0)
    return false;

  i64 Mult = 1, Start = 0;
  if (StrR[0] == '-')
  {
    Mult = -1;
    Start = 1;
  }
  *Result = 0;
  for (int I = 0; I < Str.Size - Start; ++I)
  {
    int V = StrR[StrR.Size - I - 1] - '0';
    if (V >= 0 && V < 10)
      *Result +=
        Mult *
        (V *
         Pow(i64(10), I)); // TODO: precompute the pow table (somehow I can't use pow like for int)
    else
      return false;
  }

  return true;
}


bool
ToDouble(const stref& Str, f64* Result)
{
  if (!Str || Str.Size <= 0)
    return false;
  char* EndPtr = nullptr;
  *Result = strtod(Str.ConstPtr, &EndPtr);
  bool Failure = errno == ERANGE || EndPtr == Str.ConstPtr || !EndPtr ||
                 !(isspace(*EndPtr) || ispunct(*EndPtr) || (*EndPtr) == 0);
  return !Failure;
}

/* tokenizer stuff */

stref
Next(tokenizer* Tk)
{
  while (Tk->Pos < Tk->Input.Size && Contains(Tk->Delims, Tk->Input[Tk->Pos]))
    ++Tk->Pos;

  if (Tk->Pos < Tk->Input.Size)
  {
    int Length = 0;
    while (Tk->Pos < Tk->Input.Size && !Contains(Tk->Delims, Tk->Input[Tk->Pos]))
    {
      ++Tk->Pos;
      ++Length;
    }
    return SubString(Tk->Input, Tk->Pos - Length, Length);
  }
  return stref();
}


void
Reset(tokenizer* Tk)
{
  Tk->Pos = 0;
}


} // namespace idx2
