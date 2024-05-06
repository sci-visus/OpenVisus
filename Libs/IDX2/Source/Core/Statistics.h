#pragma once

#include "Algorithm.h"
#include "Common.h"
#include <math.h>


namespace idx2
{


struct stat
{
  f64 MinV = traits<f64>::Max;
  f64 MaxV = traits<f64>::Min;
  f64 S = 0;
  i64 N = 0;
  f64 SSq = 0;

  idx2_Inline void Add(f64 Val)
  {
    MinV = Min(MinV, Val);
    MaxV = Max(MaxV, Val);
    S += Val;
    ++N;
    SSq += Val * Val;
  }

  idx2_Inline f64 GetMin() const { return MinV; }
  idx2_Inline f64 GetMax() const { return MaxV; }
  idx2_Inline f64 Sum() const { return S; }
  idx2_Inline i64 Count() const { return N; }
  idx2_Inline f64 SumSq() const { return SSq; }
  idx2_Inline f64 Avg() const { return S / N; }
  idx2_Inline f64 AvgSq() const { return SSq / N; }
  idx2_Inline f64 SqAvg() const
  {
    f64 A = Avg();
    return A * A;
  }
  idx2_Inline f64 Var() const { return AvgSq() - SqAvg(); }
  idx2_Inline f64 StdDev() const { return sqrt(Var()); }
};


} // namespace idx2
