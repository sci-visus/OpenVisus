/*
 * Tiny self-contained version of the PCG Random Number Generation for C++
 * put together from pieces of the much larger C/C++ codebase.
 * Wenzel Jakob, February 2015
 *
 * The PCG random number generator was developed by Melissa O'Neill
 * <oneill@pcg-random.org>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * For additional information about the PCG random number generation scheme,
 * including its license and other licensing options, visit
 *
 *     http://www.pcg-random.org
 */

#pragma once

#include "Common.h"
#include "Macros.h"


namespace idx2
{


// PCG32 Pseudorandom number generator
struct pcg32
{
  static constexpr auto Pcg32DefaultState = 0x853c49e6748fea9bull;
  static constexpr auto Pcg32DefaultStream = 0xda3e39cb94b95bdbull;
  static constexpr auto Pcg32Mult = 0x5851f42d4c957f2dull;
  u64 State; // RNG state.  All values are possible.
  u64 Inc;   // Controls which RNG sequence (stream) is selected. Must *always* be odd.
  // Initialize the pseudorandom number generator with default seed
  pcg32();
  // Initialize the pseudorandom number generator with the Seed() function
  pcg32(u64 Initstate, u64 Initseq = 1);
};


/*
Seed the pseudorandom number generator
Specified in two parts: a state initializer and a sequence selection constant (a.k.a. stream id)
*/
void
Seed(pcg32* Pcg, u64 InitState, u64 InitSeq = 1);

// Generate a uniformly distributed unsigned 32-bit random number
u32
NextUInt(pcg32* Pcg);

// Generate a uniformly distributed number, r, where 0 <= r < bound
u32
NextUInt(pcg32* Pcg, u32 Bound);

// Generate a single precision floating point value on the interval [0, 1)
f32
NextFloat(pcg32* Pcg);

/* Generate a double precision floating point value on the interval [0, 1)
 *
 * Since the underlying random number generator produces 32 bit output,
 * only the first 32 mantissa bits will be filled (however, the resolution is
 * still finer than in NextFloat(), which only uses 23 mantissa bits) */
f64
NextDouble(pcg32* Pcg);

/* Multi-step advance function (jump-ahead, jump-back)
 *
 * The method used here is based on Brown, "Random Number Generation
 * with Arbitrary Stride", Transactions of the American Nuclear
 * Society (Nov. 1994). The algorithm is very similar to fast exponentiation. */
void
Advance(i64 Delta_);

/* Draw uniformly distributed permutation and permute the given STL container
 *
 * From: Knuth, TAoCP Vol. 2 (3rd 3d), Section 3.4.2 */
template <typename it> void
Shuffle(it Begin, it End);

// Compute the distance between two PCG32 pseudorandom number generators
i64
operator-(const pcg32& First, const pcg32& Second);

// Equality operator
bool
operator==(const pcg32& First, const pcg32& Second);

// Inequality operator
bool
operator!=(const pcg32& First, const pcg32& Second);


} // namespace idx2



/*
 * Tiny self-contained version of the PCG Random Number Generation for C++
 * put together from pieces of the much larger C/C++ codebase.
 * Wenzel Jakob, February 2015
 *
 * The PCG random number generator was developed by Melissa O'Neill
 * <oneill@pcg-random.org>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * For additional information about the PCG random number generation scheme,
 * including its license and other licensing options, visit
 *
 *     http://www.pcg-random.org
 */

#include "Algorithm.h"
#include "Assert.h"
#include "Random.h"
#include <math.h>


namespace idx2
{


idx2_Inline
pcg32::pcg32()
  : State(Pcg32DefaultState)
  , Inc(Pcg32DefaultStream)
{
}


idx2_Inline
pcg32::pcg32(u64 InitState, u64 InitSeq)
  : pcg32()
{
  Seed(this, InitState, InitSeq);
}


idx2_Inline void
Seed(pcg32* Pcg, u64 InitState, u64 InitSeq)
{
  Pcg->State = 0u;
  Pcg->Inc = (InitSeq << 1u) | 1u;
  NextUInt(Pcg);
  Pcg->State += InitState;
  NextUInt(Pcg);
}


idx2_Inline u32
NextUInt(pcg32* Pcg)
{
  u64 OldState = Pcg->State;
  Pcg->State = OldState * pcg32::Pcg32Mult + Pcg->Inc;
  u32 XorShifted = (u32)(((OldState >> 18u) ^ OldState) >> 27u);
  u32 Rot = (u32)(OldState >> 59u);
  return (XorShifted >> Rot) | (XorShifted << ((~Rot + 1u) & 31));
}


idx2_Inline u32
NextUInt(pcg32* Pcg, u32 Bound)
{
  // To avoid bias, we need to make the range of the RNG a multiple of
  // bound, which we do by dropping output less than a threshold.
  // A naive scheme to calculate the threshold would be to do
  //
  //     u32 threshold = 0x100000000ull % bound;
  //
  // but 64-bit div/mod is slower than 32-bit div/mod (especially on
  // 32-bit platforms).  In essence, we do
  //
  //     u32 threshold = (0x100000000ull-bound) % bound;
  //
  // because this version will calculate the same modulus, but the LHS
  // value is less than 2^32.

  u32 Threshold = (~Bound + 1u) % Bound;

  // Uniformity guarantees that this loop will terminate.  In practice, it
  // should usually terminate quickly; on average (assuming all bounds are
  // equally likely), 82.25% of the time, we can expect it to require just
  // one iteration. In the worst case, someone passes a bound of 2^31 + 1
  // (i.e., 2147483649), which invalidates almost 50% of the range. In
  // practice, bounds are typically small and only a tiny amount of the range
  // is eliminated.
  for (;;)
  {
    u32 R = NextUInt(Pcg);
    if (R >= Threshold)
      return R % Bound;
  }
}


idx2_Inline f32
NextFloat(pcg32* Pcg)
{
  /* Trick from MTGP: generate an uniformly distributed single precision number
  in [1,2) and subtract 1. */
  union
  {
    u32 U;
    f32 F;
  } X;
  X.U = (NextUInt(Pcg) >> 9) | 0x3f800000u;
  return X.F - 1.0f;
}


idx2_Inline f64
NextDouble(pcg32* Pcg)
{
  /* Trick from MTGP: generate an uniformly distributed double precision number
  in [1,2) and subtract 1. */
  union
  {
    u64 U;
    f64 D;
  } X;
  X.U = ((u64)NextUInt(Pcg) << 20) | 0x3ff0000000000000ull;
  return X.D - 1.0;
}


inline void
Advance(pcg32* Pcg, i64 Delta_)
{
  u64 CurMult = pcg32::Pcg32Mult;
  u64 CurPlus = Pcg->Inc;
  u64 AccMult = 1u;
  u64 AccPlus = 0u;

  /* Even though delta is an unsigned integer, we can pass a signed integer to
  go backwards, it just goes "the long way round". */
  u64 Delta = (u64)Delta_;

  while (Delta > 0)
  {
    if (Delta & 1)
    {
      AccMult *= CurMult;
      AccPlus = AccPlus * CurMult + CurPlus;
    }
    CurPlus = (CurMult + 1) * CurPlus;
    CurMult *= CurMult;
    Delta /= 2;
  }
  Pcg->State = AccMult * Pcg->State + AccPlus;
}


template <typename it> idx2_Inline void
Shuffle(pcg32* Pcg, it Begin, it End)
{
  for (it It = End - 1; It > Begin; --It)
    IterSwap(It, Begin + NextUInt(Pcg, (u32)(It - Begin + 1)));
}


// Compute the distance between two PCG32 pseudorandom number generators
inline i64
operator-(const pcg32& First, const pcg32& Second)
{
  idx2_Assert(First.Inc == Second.Inc);

  u64 CurMult = pcg32::Pcg32Mult;
  u64 CurPlus = First.Inc;
  u64 CurState = Second.State;
  u64 TheBit = 1u;
  u64 Distance = 0u;

  while (First.State != CurState)
  {
    if ((First.State & TheBit) != (CurState & TheBit))
    {
      CurState = CurState * CurMult + CurPlus;
      Distance |= TheBit;
    }
    idx2_Assert((First.State & TheBit) == (CurState & TheBit));
    TheBit <<= 1;
    CurPlus = (CurMult + 1ull) * CurPlus;
    CurMult *= CurMult;
  }

  return (i64)Distance;
}


// Equality operator
idx2_Inline bool
operator==(const pcg32& First, const pcg32& Second)
{
  return First.State == Second.State && First.Inc == Second.Inc;
}


// Inequality operator
idx2_Inline bool
operator!=(const pcg32& First, const pcg32& Second)
{
  return First.State != Second.State || First.Inc != Second.Inc;
}


} // namespace idx2
