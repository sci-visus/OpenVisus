/* Command-line argument processing */
// TODO: encapsulate Argc and Argv into a struct so that we can easily pass it around
#pragma once

#include "Array.h"
#include "Common.h"
#include "Enum.h"
#include "String.h"

namespace idx2
{


/* Search the Args for unsupported options (e.g., a typo from the user).
Opts is a string containing all the supported options, separated by whitespaces. */
void CheckForUnsupportedOpt(int NArgs, cstr* Args, cstr Opts);
bool OptVal(int NArgs, cstr* Args, cstr Opt, str Val);
bool OptVal(int NArgs, cstr* Args, cstr Opt, cstr* Val);
bool OptVal(int NArgs, cstr* Args, cstr Opt, stref* Val);
bool OptVal(int NArgs, cstr* Args, cstr Opt, int* Val);
bool OptVal(int NArgs, cstr* Args, cstr Opt, i64* Val);
bool OptVal(int NArgs, cstr* Args, cstr Opt, u8* Val);
bool OptVal(int NArgs, cstr* Args, cstr Opt, f64* Val);
bool OptVal(int NArgs, cstr* Args, cstr Opt, v3i* Val);
bool OptVal(int NArgs, cstr* Args, cstr Opt, v3<i64>* Val);
bool OptVal(int NArgs, cstr* Args, cstr Opt, v2i* Val);
bool OptVal(int NArgs, cstr* Args, cstr Opt, t2<char, int>* Val);
bool OptVal(int NArgs, cstr* Args, cstr Opt, v3<t2<char, int>>* Val);
bool OptVal(int NArgs, cstr* Args, cstr Opt, array<int>* Vals);
bool OptVal(int NArgs, cstr* Args, cstr Opt, array<stref>* Vals);
bool OptExists(int NArgs, cstr* Args, cstr Opt);

template <typename e> bool OptVal(int NArgs, cstr* Args, cstr Opt, e* Val); // output to an Enum

#undef idx2_RequireOption
#define idx2_RequireOption(Argc, Argv, Str, Var, Err)                                              \
  if (!OptVal(Argc, Argv, Str, Var))                                                               \
  {                                                                                                \
    fprintf(stderr, Err);                                                                          \
    exit(1);                                                                                       \
  }


} // namespace idx2



namespace idx2
{


template <typename e> bool
OptVal(int NArgs, cstr* Args, cstr Opt, e* Val)
{
  cstr BufPtr = nullptr;
  if (!OptVal(NArgs, Args, Opt, &BufPtr))
    return false;
  *Val = StringTo<e>()(BufPtr);

  return true;
}


} // namespace idx2
