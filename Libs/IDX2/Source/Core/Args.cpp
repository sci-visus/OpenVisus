#include "Args.h"
#include "Enum.h"
#include "Error.h"
#include "String.h"
#include <string.h>


namespace idx2
{


void
CheckForUnsupportedOpt(int NArgs, cstr* Args, cstr Opts)
{
  for (int I = 1; I < NArgs; ++I)
  {
    if (Args[I][0] == '-' && strstr(Opts, Args[I]) == nullptr)
      idx2_Exit("Option %s not supported\n", Args[I]);
  }
}


/* Copy the input string into Val */
bool
OptVal(int NArgs, cstr* Args, cstr Opt, str Val)
{
  for (int I = 1; I + 1 < NArgs; ++I)
  {
    if (strncmp(Args[I], Opt, 32) == 0)
    {
      int J = 0;
      while (Val[J] = Args[I + 1][J])
      {
        ++J;
      }
      return true;
    }
  }

  return false;
}


/* Point Val to the input string */
bool
OptVal(int NArgs, cstr* Args, cstr Opt, cstr* Val)
{
  for (int I = 1; I + 1 < NArgs; ++I)
  {
    if (strncmp(Args[I], Opt, 32) == 0)
    {
      *Val = Args[I + 1];
      return true;
    }
  }

  return false;
}


bool
OptVal(int NArgs, cstr* Args, cstr Opt, stref* Val)
{
  cstr Temp;
  if (!OptVal(NArgs, Args, Opt, &Temp))
    return false;
  *Val = stref(Temp);
  return true;
}


bool
OptVal(int NArgs, cstr* Args, cstr Opt, int* Val)
{
  for (int I = 1; I + 1 < NArgs; ++I)
  {
    if (strncmp(Args[I], Opt, 32) == 0)
      return ToInt(Args[I + 1], Val);
  }

  return false;
}


bool
OptVal(int NArgs, cstr* Args, cstr Opt, i64* V)
{
  for (int I = 1; I + 1 < NArgs; ++I)
  {
    if (strncmp(Args[I], Opt, 32) == 0)
    {
      return ToInt64(Args[I + 1], V);
    }
  }

  return false;
}


bool
OptVal(int NArgs, cstr* Args, cstr Opt, u8* Val)
{
  int IntVal;
  for (int I = 1; I + 1 < NArgs; ++I)
  {
    if (strncmp(Args[I], Opt, 32) == 0)
    {
      bool Success = ToInt(Args[I + 1], &IntVal);
      *Val = (u8)IntVal;
      return Success;
    }
  }

  return false;
}


bool
OptVal(int NArgs, cstr* Args, cstr Opt, t2<char, int>* Val)
{
  for (int I = 1; I + 2 < NArgs; ++I)
  {
    if (strncmp(Args[I], Opt, 32) == 0)
    {
      Val->First = Args[I + 1][0];
      return ToInt(Args[I + 2], &Val->Second);
    }
  }

  return false;
}


bool
OptVal(int NArgs, cstr* Args, cstr Opt, v3i* Val)
{
  for (int I = 1; I + 3 < NArgs; ++I)
  {
    if (strncmp(Args[I], Opt, 32) == 0)
    {
      return ToInt(Args[I + 1], &Val->X) && ToInt(Args[I + 2], &Val->Y) &&
             ToInt(Args[I + 3], &Val->Z);
    }
  }

  return false;
}


bool
OptVal(int NArgs, cstr* Args, cstr Opt, v3<i64>* Val)
{
  for (int I = 1; I + 3 < NArgs; ++I)
  {
    if (strncmp(Args[I], Opt, 32) == 0)
    {
      return ToInt64(Args[I + 1], &Val->X) && ToInt64(Args[I + 2], &Val->Y) &&
             ToInt64(Args[I + 3], &Val->Z);
    }
  }

  return false;
}


bool
OptVal(int NArgs, cstr* Args, cstr Opt, array<int>* Vals)
{
  Clear(Vals);
  for (int I = 1; I < NArgs; ++I)
  {
    if (strncmp(Args[I], Opt, 32) == 0)
    {
      int J = I;
      while (true)
      {
        ++J;
        int X;
        if (J < NArgs && ToInt(Args[J], &X))
          PushBack(Vals, X);
        else
          break;
      }
      return J > I + 1;
    }
  }

  return false;
}


bool
OptVal(int NArgs, cstr* Args, cstr Opt, v2i* Val)
{
  for (int I = 1; I + 2 < NArgs; ++I)
  {
    if (strncmp(Args[I], Opt, 32) == 0)
    {
      return ToInt(Args[I + 1], &Val->X) && ToInt(Args[I + 2], &Val->Y);
    }
  }

  return false;
}


bool
OptVal(int NArgs, cstr* Args, cstr Opt, v3<t2<char, int>>* Val)
{
  for (int I = 1; I + 5 < NArgs; ++I)
  {
    if (strncmp(Args[I], Opt, 32) == 0)
    {
      bool Success = true;
      (*Val)[0].First = Args[I + 1][0];
      Success = Success && ToInt(Args[I + 2], &(*Val)[0].Second);
      (*Val)[1].First = Args[I + 2][0];
      Success = Success && ToInt(Args[I + 3], &(*Val)[1].Second);
      (*Val)[2].First = Args[I + 4][0];
      Success = Success && ToInt(Args[I + 5], &(*Val)[2].Second);
      return Success;
    }
  }

  return false;
}


bool
OptVal(int NArgs, cstr* Args, cstr Opt, f64* Val)
{
  for (int I = 1; I + 1 < NArgs; ++I)
  {
    if (strncmp(Args[I], Opt, 32) == 0)
      return ToDouble(Args[I + 1], Val);
  }

  return false;
}


bool
OptExists(int NArgs, cstr* Args, cstr Opt)
{
  for (int I = 1; I < NArgs; ++I)
  {
    if (strcmp(Args[I], Opt) == 0)
      return true;
  }

  return false;
}


bool
OptVal(int NArgs, cstr* Args, cstr Opt, array<stref>* Vals)
{
  for (i32 I = 1; I < NArgs; ++I)
  {
    if (strncmp(Args[I], Opt, 32) == 0)
    {
      i32 J = I + 1;
      while (J < NArgs && Args[J][0] != '-')
      {
        cstr Temp;
        Temp = Args[J];
        stref Str = stref(Temp);
        PushBack(Vals, Str);
        ++J;
      }
      return J > I + 1;
    }
  }
  return false;
}


} // namespace idx2

