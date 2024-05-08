#pragma once

#include "Macros.h"


namespace idx2
{


template <typename func_t> struct scope_guard
{
  func_t Func;
  bool Dismissed = false;
  // TODO: std::forward FuncIn?
  scope_guard(const func_t& FuncIn)
    : Func(FuncIn)
  {
  }

  ~scope_guard()
  {
    if (!Dismissed)
    {
      Func();
    }
  }
};


} // namespace idx2



#define idx2_BeginCleanUp(...) idx2_MacroOverload(idx2_BeginCleanUp, __VA_ARGS__)
#define idx2_BeginCleanUp_0() auto idx2_Cat(__CleanUpFunc__, __LINE__) = [&]()
#define idx2_BeginCleanUp_1(N) auto __CleanUpFunc__##N = [&]()
#define idx2_EndCleanUp(...) idx2_MacroOverload(idx2_EndCleanUp, __VA_ARGS__)
#define idx2_EndCleanUp_0()                                                                        \
  idx2::scope_guard idx2_Cat(__ScopeGuard__, __LINE__)(idx2_Cat(__CleanUpFunc__, __LINE__));
#define idx2_EndCleanUp_1(N) idx2::scope_guard __ScopeGuard__##N(__CleanUpFunc__##N);

//#define idx2_BeginCleanUp(n) auto __CleanUpFunc__##n = [&]()
#define idx2_CleanUp(...) idx2_MacroOverload(idx2_CleanUp, __VA_ARGS__)
#define idx2_CleanUp_1(...)                                                                        \
  idx2_BeginCleanUp_0() { __VA_ARGS__; };                                                          \
  idx2_EndCleanUp_0()
#define idx2_CleanUp_2(N, ...)                                                                     \
  idx2_BeginCleanUp_1(N) { __VA_ARGS__; };                                                         \
  idx2_EndCleanUp_1(N)

// #define idx2_CleanUp(n, ...) idx2_BeginCleanUp(n) { __VA_ARGS__; }; idx2_EndCleanUp(n)
#define idx2_DismissCleanUp(N)                                                                     \
  {                                                                                                \
    __ScopeGuard__##N.Dismissed = true;                                                            \
  }
