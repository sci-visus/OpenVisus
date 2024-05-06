#pragma once

#include "Common.h"


namespace idx2
{


struct timer;
void
StartTimer(timer* Timer);
i64
ElapsedTime(timer* Timer); // return nanoseconds
i64
ResetTimer(timer* Timer); // return nanoseconds
f64
Milliseconds(i64 Nanosecs);
f64
Seconds(i64 Nanosecs);


} // namespace idx2



#include "Macros.h"
#if defined(idx2_CTimer)
#include <time.h>


namespace idx2
{


struct timer
{
  clock_t Start = 0;
};

idx2_Inline void
StartTimer(timer* Timer)
{
  Timer->Start = clock();
}

idx2_Inline i64
ElapsedTime(timer* Timer)
{
  auto End = clock();
  auto Seconds = (double)(End - Timer->Start) / CLOCKS_PER_SEC;
  return Seconds * 1e9;
}


} // namespace idx2


#elif defined(_WIN32)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>


namespace idx2
{


struct timer
{
  inline const static i64 PCFreq = []()
  {
    LARGE_INTEGER Li;
    bool Ok = QueryPerformanceFrequency(&Li);
    return Ok ? Li.QuadPart : 0;
  }();
  i64 CounterStart = 0;
};


idx2_Inline void
StartTimer(timer* Timer)
{
  LARGE_INTEGER Li;
  QueryPerformanceCounter(&Li);
  Timer->CounterStart = Li.QuadPart;
}


// TODO: take a const reference
idx2_Inline i64
ElapsedTime(timer* Timer)
{
  LARGE_INTEGER Li;
  QueryPerformanceCounter(&Li);
  return (Li.QuadPart - Timer->CounterStart) * 1000000000 / Timer->PCFreq;
}


} // namespace idx2

#elif defined(__CYGWIN__) || defined(__linux__) || defined(__APPLE__)
#include <time.h>

namespace idx2
{


struct timer
{
  timespec Start;
};


idx2_Inline void
StartTimer(timer* Timer)
{
  clock_gettime(CLOCK_MONOTONIC, &Timer->Start);
}


idx2_Inline i64
ElapsedTime(timer* Timer)
{
  timespec End;
  clock_gettime(CLOCK_MONOTONIC, &End);
  return 1e9 * (End.tv_sec - Timer->Start.tv_sec) + (End.tv_nsec - Timer->Start.tv_nsec);
}


} // namespace idx2

#endif


namespace idx2
{

idx2_Inline i64
ResetTimer(timer* Timer)
{
  i64 Elapsed = ElapsedTime(Timer);
  StartTimer(Timer);
  return Elapsed;
}


idx2_Inline f64
Milliseconds(i64 Nanosecs)
{
  return f64(Nanosecs) / 1e6;
}


idx2_Inline f64
Seconds(i64 Nanosecs)
{
  return f64(Nanosecs) / 1e9;
}


} // namespace idx2
