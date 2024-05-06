#pragma once

#include "Common.h"
#include "Macros.h"


namespace idx2
{


constexpr f64 Pi = 3.14159265358979323846;

/* Generate a power table for a particular base and type */
template <int N> int (&Power(int Base))[N];

bool
IsEven(i64 X);

bool
IsOdd(i64 X);

v3i
IsEven(const v3i& P);

v3i
IsOdd(const v3i& P);

template <typename t> bool
IsBetween(t Val, t A, t B);

bool
IsPrime(i64 X);

i64
NextPrime(i64 X);

template <typename t> int
Exponent(t Val);

template <typename u, typename t = u> t
Prod(const v2<u>& Vec);

template <typename u, typename t = u> t
Prod(const v3<u>& Vec);

template <typename u, typename t = u> t
Prod(const v6<u>& Vec);

template <typename u, typename t = u> t
Sum(const v2<u>& Vec);

template <typename u, typename t = u> t
Sum(const v3<u>& Vec);


/* 2D vector math */
template <typename t> v2<t>
operator+(const v2<t>& Lhs, const v2<t>& Rhs);

template <typename t> v2<t>
operator-(const v2<t>& Lhs, const v2<t>& Rhs);

template <typename t> v2<t>
operator/(const v2<t>& Lhs, const v2<t>& Rhs);

template <typename t> v2<t>
operator*(const v2<t>& Lhs, const v2<t>& Rhs);

template <typename t> v2<t>
operator+(const v2<t>& Lhs, t Val);

template <typename t> v2<t>
operator-(const v2<t>& Lhs, t Val);

template <typename t> v2<t>
operator/(const v2<t>& Lhs, t Rhs);

template <typename t> v2<t>
operator*(const v2<t>& Lhs, t Val);

template <typename t> bool
operator>(const v2<t>& Lhs, t Val);

template <typename t> bool
operator<(const v2<t>& Lhs, t Val);

template <typename t> bool
operator==(const v2<t>& Lhs, const v2<t>& Rhs);

template <typename t> bool
operator!=(const v2<t>& Lhs, const v2<t>& Rhs);

template <typename t> bool
operator<(t Val, const v2<t>& Rhs);

template <typename t> bool
operator<=(t Val, const v2<t>& Rhs);

template <typename t> bool
operator<=(const v2<t>& Lhs, t Val);

template <typename t> v2<t>
Min(const v2<t>& Lhs, const v2<t>& Rhs);

template <typename t> v2<t>
Max(const v2<t>& Lhs, const v2<t>& Rhs);


// TODO: generalize the t parameter to u for the RHS
/* 3D vector math */
template <typename t> v3<t>
operator+(const v3<t>& Lhs, const v3<t>& Rhs);

template <typename t> v3<t>
operator-(const v3<t>& Lhs, const v3<t>& Rhs);

template <typename t> v3<t>
operator*(const v3<t>& Lhs, const v3<t>& Rhs);

template <typename t> v3<t>
operator/(const v3<t>& Lhs, const v3<t>& Rhs);

template <typename t> v3<t>
operator&(const v3<t>& Lhs, const v3<t>& Rhs);

template <typename t> v3<t>
operator%(const v3<t>& Lhs, const v3<t>& Rhs);

template <typename t> v3<t>
operator+(const v3<t>& Lhs, t Val);

template <typename t> v3<t>
operator-(const v3<t>& Lhs, t Val);

template <typename t> v3<t>
operator-(t Val, const v3<t>& Lhs);

template <typename t> v3<t>
operator*(const v3<t>& Lhs, t Val);

template <typename t> v3<t>
operator/(const v3<t>& Lhs, t Val);

template <typename t> v3<t>
operator&(const v3<t>& Lhs, t Val);

template <typename t> v3<t>
operator%(const v3<t>& Lhs, t Val);

template <typename t> bool
operator==(const v3<t>& Lhs, const v3<t>& Rhs);

template <typename t> bool
operator==(const v6<t>& Lhs, const v6<t>& Rhs);

template <typename t> bool
operator!=(const v3<t>& Lhs, const v3<t>& Rhs);

template <typename t> bool
operator<=(const v3<t>& Lhs, const v3<t>& Rhs);

template <typename t> bool
operator<(const v3<t>& Lhs, const v3<t>& Rhs);

template <typename t> bool
operator>(const v3<t>& Lhs, const v3<t>& Rhs);

template <typename t> bool
operator>(const v3<t>& Lhs, t Val);

template <typename t> bool
operator>=(const v3<t>& Lhs, const v3<t>& Rhs);

template <typename t> bool
operator==(const v3<t>& Lhs, t Val);

template <typename t> bool
operator!=(const v3<t>& Lhs, t Val);

template <typename t> bool
operator<(t Val, const v3<t>& Rhs);

template <typename t> bool
operator<(const v3<t>& Lhs, t Val);

template <typename t> bool
operator<=(t Val, const v3<t>& Rhs);

template <typename t> bool
operator<=(const v3<t>& Lhs, t Val);

template <typename t, typename u> v3<t>
operator>>(const v3<t>& Lhs, const v3<u>& Rhs);

template <typename t, typename u> v3<t>
operator>>(const v3<t>& Lhs, u Val);

template <typename t, typename u> v3<u>
operator>>(u Val, const v3<t>& Rhs);

template <typename t, typename u> v3<t>
operator<<(const v3<t>& Lhs, const v3<u>& Rhs);

template <typename t, typename u> v3<t>
operator<<(const v3<t>& Lhs, u Val);

template <typename t, typename u> v3<u>
operator<<(u Val, const v3<t>& Rhs);

template <typename t> v3<t>
Min(const v3<t>& Lhs, const v3<t>& Rhs);

template <typename t> v3<t>
Max(const v3<t>& Lhs, const v3<t>& Rhs);

i8
Log2Floor(i64 Val);

i8
Log2Ceil(i64 Val);

i8
Log8Floor(i64 Val);

template <typename t> v3<t>
Ceil(const v3<t>& Vec);

template <typename t> v2<t>
Ceil(const v2<t>& Vec);

i64
RoundDown(i64 Val, i64 M); // Round Val down to a multiple of M

i64
RoundUp(i64 Val, i64 M); // Round Val up to a multiple of M

i64
Pow(i64 Base, int Exp);

i64
NextPow2(i64 Val);

int
GeometricSum(int Base, int N);

template <typename t> t
Lerp(t V0, t V1, f64 T);

// bilinear interpolation
template <typename t> t
BiLerp(t V00, t V10, t V01, t V11, const v2d& T);

// trilinear interpolation
template <typename t> t
TriLerp(t V000, t V100, t V010, t V110, t V001, t V101, t V011, t V111, const v3d& T);

/* V6 stuffs */
template <typename t> v6<t>
operator+(const v6<t>& Lhs, const v6<t>& Rhs);

template <typename t> v6<t>
operator-(const v6<t>& Lhs, const v6<t>& Rhs);

template <typename t> v6<t>
operator*(const v6<t>& Lhs, const v6<t>& Rhs);

template <typename t> v6<t>
operator/(const v6<t>& Lhs, const v6<t>& Rhs);

template <typename t> v6<t>
operator&(const v6<t>& Lhs, const v6<t>& Rhs);

template <typename t> v6<t>
operator%(const v6<t>& Lhs, const v6<t>& Rhs);

template <typename t> v6<t>
operator+(const v6<t>& Lhs, t Val);

template <typename t> v6<t>
operator-(const v6<t>& Lhs, t Val);

template <typename t> v6<t>
operator-(t Val, const v6<t>& Lhs);

template <typename t> v6<t>
operator*(const v6<t>& Lhs, t Val);

template <typename t> v6<t>
operator/(const v6<t>& Lhs, t Val);

template <typename t> v6<t>
operator&(const v6<t>& Lhs, t Val);

template <typename t> v6<t>
operator%(const v6<t>& Lhs, t Val);

template <typename t> bool
operator==(const v6<t>& Lhs, const v6<t>& Rhs);

template <typename t> bool
operator!=(const v6<t>& Lhs, const v6<t>& Rhs);

template <typename t> bool
operator<=(const v6<t>& Lhs, const v6<t>& Rhs);

template <typename t> bool
operator<(const v6<t>& Lhs, const v6<t>& Rhs);

template <typename t> bool
operator>(const v6<t>& Lhs, const v6<t>& Rhs);

template <typename t> bool
operator>(const v6<t>& Lhs, t Val);

template <typename t> bool
operator>=(const v6<t>& Lhs, const v6<t>& Rhs);

template <typename t> bool
operator==(const v6<t>& Lhs, t Val);

template <typename t> bool
operator!=(const v6<t>& Lhs, t Val);

template <typename t> bool
operator<(t Val, const v6<t>& Rhs);

template <typename t> bool
operator<(const v6<t>& Lhs, t Val);

template <typename t> bool
operator<=(t Val, const v6<t>& Rhs);

template <typename t> bool
operator<=(const v6<t>& Lhs, t Val);

template <typename t, typename u> v6<t>
operator>>(const v6<t>& Lhs, const v6<u>& Rhs);

template <typename t, typename u> v6<t>
operator>>(const v6<t>& Lhs, u Val);

template <typename t, typename u> v6<u>
operator>>(u Val, const v3<t>& Rhs);

template <typename t, typename u> v6<t>
operator<<(const v6<t>& Lhs, const v6<u>& Rhs);

template <typename t, typename u> v6<t>
operator<<(const v6<t>& Lhs, u Val);

template <typename t, typename u> v6<u>
operator<<(u Val, const v6<t>& Rhs);

template <typename t> v6<t>
Min(const v6<t>& Lhs, const v6<t>& Rhs);

template <typename t> v6<t>
Max(const v6<t>& Lhs, const v6<t>& Rhs);

template <typename t> v6<t>
Ceil(const v6<t>& Vec);

} // namespace idx2


#include "Algorithm.h"
#include "Assert.h"
#include "BitOps.h"
#include "Common.h"
#include <math.h>


namespace idx2
{


idx2_Inline bool
IsEven(i64 X)
{
  return (X & 1) == 0;
}


idx2_Inline bool
IsOdd(i64 X)
{
  return (X & 1) != 0;
}


idx2_Inline v3i
IsEven(const v3i& P)
{
  return v3i(IsEven(P.X), IsEven(P.Y), IsEven(P.Z));
}


idx2_Inline v3i
IsOdd(const v3i& P)
{
  return v3i(IsOdd(P.X), IsOdd(P.Y), IsOdd(P.Z));
}


idx2_Inline int
Max(const v3i& P)
{
  return Max(Max(P.X, P.Y), P.Z);
}


idx2_Inline v3i
Abs(const v3i& P)
{
  return v3i(abs(P.X), abs(P.Y), abs(P.Z));
}


idx2_Inline v3i
Equals(const v3i& P, const v3i& Q)
{
  return v3i(P.X == Q.X, P.Y == Q.Y, P.Z == Q.Z);
}


idx2_Inline v3i
NotEquals(const v3i& P, const v3i& Q)
{
  return v3i(P.X != Q.X, P.Y != Q.Y, P.Z != Q.Z);
}


template <typename t> idx2_Inline bool
IsBetween(t Val, t A, t B)
{
  return (A <= Val && Val <= B) || (B <= Val && Val <= A);
}


idx2_Inline bool
IsPow2(int X)
{
  idx2_Assert(X > 0);
  return X && !(X & (X - 1));
}


idx2_Inline bool
IsPow2(const v3i& P)
{
  idx2_Assert(P > 0);
  return IsPow2(P.X) && IsPow2(P.Y) && IsPow2(P.Z);
}


idx2_Inline bool
IsPow2(const v6i& P)
{
  idx2_Assert(P > 0);
  return IsPow2(P.XYZ) && IsPow2(P.UVW);
}


idx2_Inline bool
IsPrime(i64 X)
{
  i64 S = (i64)sqrt((f64)X);
  for (i64 I = 2; I <= S; ++I)
  {
    if (X % I == 0)
      return false;
  }
  return true;
}


idx2_Inline i64
NextPrime(i64 X)
{
  while (IsPrime(X))
  {
    ++X;
  }
  return X;
}


idx2_Inline constexpr int
LogFloor(i64 Base, i64 Val)
{
  int Log = 0;
  i64 S = Base;
  while (S <= Val)
  {
    ++Log;
    S *= Base;
  }
  return Log;
}


template <typename t, int N> struct power
{
  static inline const stack_array<t, LogFloor(N, traits<t>::Max)> Table = []()
  {
    stack_array<t, LogFloor(N, traits<t>::Max)> Result;
    t Base = N;
    t Pow = 1;
    for (int I = 0; I < Size(Result); ++I)
    {
      Result[I] = Pow;
      Pow *= Base;
    }
    return Result;
  }();
  t operator[](int I) const { return Table[I]; }
};


/*
For double-precision, the returned exponent is between [-1023, 1024] (-1023 if 0, -1022 if denormal, the bias is 1023).
For single-precision, the returned exponent is between [-127, 128] (-127 if 0, -126 if denormal, the bias is 127) */
template <typename t> idx2_Inline int
Exponent(t Val)
{
  if (Val != 0)
  {
    int E;
    frexp(Val, &E); // for non denormal float32, return a number in [-125, 128]
    /* clamp exponent in case Val is denormal */
    return Max(E, 1 - traits<t>::ExpBias); // return -126 (for float32) if denormal (which can return an exponent as small as -148)
  }
  return -traits<t>::ExpBias;
}


template <typename t> idx2_Inline v2<t>
operator+(const v2<t>& Lhs, const v2<t>& Rhs)
{
  return v2<t>(Lhs.X + Rhs.X, Lhs.Y + Rhs.Y);
}


template <typename t> idx2_Inline v2<t>
operator+(const v2<t>& Lhs, t Val)
{
  return v2<t>(Lhs.X + Val, Lhs.Y + Val);
}


template <typename t> idx2_Inline v2<t>
operator-(const v2<t>& Lhs, const v2<t>& Rhs)
{
  return v2<t>(Lhs.X - Rhs.X, Lhs.Y - Rhs.Y);
}


template <typename t> idx2_Inline v2<t>
operator-(const v2<t>& Lhs, t Val)
{
  return v2<t>(Lhs.X - Val, Lhs.Y - Val);
}


template <typename t> idx2_Inline v2<t>
operator*(const v2<t>& Lhs, const v2<t>& Rhs)
{
  return v2<t>(Lhs.X * Rhs.X, Lhs.Y * Rhs.Y);
}


template <typename t> idx2_Inline v2<t>
operator*(const v2<t>& Lhs, t Val)
{
  return v2<t>(Lhs.X * Val, Lhs.Y * Val);
}


template <typename t> idx2_Inline v2<t>
operator/(const v2<t>& Lhs, const v2<t>& Rhs)
{
  return v2<t>(Lhs.X / Rhs.X, Lhs.Y / Rhs.Y);
}


template <typename t> idx2_Inline v2<t>
operator/(const v2<t>& Lhs, t Val)
{
  return v2<t>(Lhs.X / Val, Lhs.Y / Val);
}


template <typename t> idx2_Inline bool
operator==(const v2<t>& Lhs, const v2<t>& Rhs)
{
  return Lhs.X == Rhs.X && Lhs.Y == Rhs.Y;
}


template <typename t> idx2_Inline bool
operator!=(const v2<t>& Lhs, const v2<t>& Rhs)
{
  return Lhs.X != Rhs.X || Lhs.Y != Rhs.Y;
}


template <typename t> bool
operator>(const v2<t>& Lhs, t Val)
{
  return Lhs.X > Val && Lhs.Y > Val;
}


template <typename t> bool
operator<(t Val, const v2<t>& Rhs)
{
  return Val < Rhs.X && Val < Rhs.Y;
}


template <typename t> bool
operator<(const v2<t>& Lhs, t Val)
{
  return Lhs.X < Val && Lhs.Y < Val;
}


template <typename t> bool
operator<=(t Val, const v2<t>& Rhs)
{
  return Val <= Rhs.X && Val <= Rhs.Y;
}


template <typename t> bool
operator<=(const v2<t>& Lhs, t Val)
{
  return Lhs.X <= Val && Lhs.Y <= Val;
}


template <typename t> idx2_Inline v2<t>
Min(const v2<t>& Lhs, const v2<t>& Rhs)
{
  return v2<t>(Min(Lhs.X, Rhs.X), Min(Lhs.Y, Rhs.Y));
}


template <typename t> idx2_Inline v2<t>
Max(const v2<t>& Lhs, const v2<t>& Rhs)
{
  return v2<t>(Max(Lhs.X, Rhs.X), Max(Lhs.Y, Rhs.Y));
}


template <typename u, typename t> idx2_Inline t
Prod(const v2<u>& Vec)
{
  return t(Vec.X) * t(Vec.Y);
}


template <typename u, typename t> idx2_Inline t
Prod(const v3<u>& Vec)
{
  return t(Vec.X) * t(Vec.Y) * t(Vec.Z);
}


template <typename u, typename t> idx2_Inline t
Sum(const v2<u>& Vec)
{
  return t(Vec.X) + t(Vec.Y);
}


template <typename u, typename t> idx2_Inline t
Sum(const v3<u>& Vec)
{
  return t(Vec.X) + t(Vec.Y) + t(Vec.Z);
}


template <typename t> idx2_Inline v3<t>
operator+(const v3<t>& Lhs, const v3<t>& Rhs)
{
  return v3<t>(Lhs.X + Rhs.X, Lhs.Y + Rhs.Y, Lhs.Z + Rhs.Z);
}


template <typename t> idx2_Inline v3<t>
operator+(const v3<t>& Lhs, t Val)
{
  return v3<t>(Lhs.X + Val, Lhs.Y + Val, Lhs.Z + Val);
}


template <typename t> idx2_Inline v3<t>
operator-(const v3<t>& Lhs, const v3<t>& Rhs)
{
  return v3<t>(Lhs.X - Rhs.X, Lhs.Y - Rhs.Y, Lhs.Z - Rhs.Z);
}


template <typename t> idx2_Inline v3<t>
operator-(const v3<t>& Lhs, t Val)
{
  return v3<t>(Lhs.X - Val, Lhs.Y - Val, Lhs.Z - Val);
}


template <typename t> idx2_Inline v3<t>
operator-(t Val, const v3<t>& Lhs)
{
  return v3<t>(Val - Lhs.X, Val - Lhs.Y, Val - Lhs.Z);
}


template <typename t> idx2_Inline v3<t>
operator*(const v3<t>& Lhs, const v3<t>& Rhs)
{
  return v3<t>(Lhs.X * Rhs.X, Lhs.Y * Rhs.Y, Lhs.Z * Rhs.Z);
}


template <typename t> idx2_Inline v3<t>
operator*(const v3<t>& Lhs, t Val)
{
  return v3<t>(Lhs.X * Val, Lhs.Y * Val, Lhs.Z * Val);
}


template <typename t> idx2_Inline v3<t>
operator/(const v3<t>& Lhs, const v3<t>& Rhs)
{
  return v3<t>(Lhs.X / Rhs.X, Lhs.Y / Rhs.Y, Lhs.Z / Rhs.Z);
}


template <typename t> idx2_Inline v3<t>
operator/(const v3<t>& Lhs, t Val)
{
  return v3<t>(Lhs.X / Val, Lhs.Y / Val, Lhs.Z / Val);
}


template <typename t> idx2_Inline v3<t>
operator&(const v3<t>& Lhs, const v3<t>& Rhs)
{
  return v3<t>(Lhs.X & Rhs.X, Lhs.Y & Rhs.Y, Lhs.Z & Rhs.Z);
}


template <typename t> idx2_Inline v3<t>
operator&(const v3<t>& Lhs, t Val)
{
  return v3<t>(Lhs.X & Val, Lhs.Y & Val, Lhs.Z & Val);
}


template <typename t> idx2_Inline v3<t>
operator%(const v3<t>& Lhs, const v3<t>& Rhs)
{
  return v3<t>(Lhs.X % Rhs.X, Lhs.Y % Rhs.Y, Lhs.Z % Rhs.Z);
}


template <typename t> idx2_Inline v3<t>
operator%(const v3<t>& Lhs, t Val)
{
  return v3<t>(Lhs.X % Val, Lhs.Y % Val, Lhs.Z % Val);
}


template <typename t> idx2_Inline bool
operator==(const v3<t>& Lhs, const v3<t>& Rhs)
{
  return Lhs.X == Rhs.X && Lhs.Y == Rhs.Y && Lhs.Z == Rhs.Z;
}


template <typename t> idx2_Inline bool
operator!=(const v3<t>& Lhs, const v3<t>& Rhs)
{
  return Lhs.X != Rhs.X || Lhs.Y != Rhs.Y || Lhs.Z != Rhs.Z;
}


template <typename t> idx2_Inline bool
operator==(const v3<t>& Lhs, t Val)
{
  return Lhs.X == Val && Lhs.Y == Val && Lhs.Z == Val;
}


template <typename t> idx2_Inline bool
operator!=(const v3<t>& Lhs, t Val)
{
  return Lhs.X != Val || Lhs.Y != Val || Lhs.Z != Val;
}


template <typename t> idx2_Inline bool
operator<=(const v3<t>& Lhs, const v3<t>& Rhs)
{
  return Lhs.X <= Rhs.X && Lhs.Y <= Rhs.Y && Lhs.Z <= Rhs.Z;
}


template <typename t> idx2_Inline bool
operator<(const v3<t>& Lhs, const v3<t>& Rhs)
{
  return Lhs.X < Rhs.X && Lhs.Y < Rhs.Y && Lhs.Z < Rhs.Z;
}


template <typename t> idx2_Inline bool
operator>(const v3<t>& Lhs, const v3<t>& Rhs)
{
  return Lhs.X > Rhs.X && Lhs.Y > Rhs.Y && Lhs.Z > Rhs.Z;
}


template <typename t> idx2_Inline bool
operator>(const v3<t>& Lhs, t Val)
{
  return Lhs.X > Val && Lhs.Y > Val && Lhs.Z > Val;
}


template <typename t> idx2_Inline bool
operator>=(const v3<t>& Lhs, const v3<t>& Rhs)
{
  return Lhs.X >= Rhs.X && Lhs.Y >= Rhs.Y && Lhs.Z >= Rhs.Z;
}


template <typename t> bool
operator<(t Val, const v3<t>& Rhs)
{
  return Val < Rhs.X && Val < Rhs.Y && Val < Rhs.Z;
}


template <typename t> bool
operator<(const v3<t>& Lhs, t Val)
{
  return Lhs.X < Val && Lhs.Y < Val && Lhs.Z < Val;
}


template <typename t> bool
operator<=(const v3<t>& Lhs, t Val)
{
  return Lhs.X <= Val && Lhs.Y <= Val && Lhs.Z <= Val;
}


template <typename t> bool
operator<=(t Val, const v3<t>& Rhs)
{
  return Val <= Rhs.X && Val <= Rhs.Y && Val <= Rhs.Z;
}


template <typename t, typename u> idx2_Inline v3<t>
operator>>(const v3<t>& Lhs, const v3<u>& Rhs)
{
  return v3<t>(Lhs.X >> Rhs.X, Lhs.Y >> Rhs.Y, Lhs.Z >> Rhs.Z);
}


template <typename t, typename u> idx2_Inline v3<u>
operator>>(u Val, const v3<t>& Lhs)
{
  return v3<u>(Val >> Lhs.X, Val >> Lhs.Y, Val >> Lhs.Z);
}


template <typename t, typename u> idx2_Inline v3<t>
operator>>(const v3<t>& Lhs, u Val)
{
  return v3<t>(Lhs.X >> Val, Lhs.Y >> Val, Lhs.Z >> Val);
}


template <typename t, typename u> idx2_Inline v3<t>
operator<<(const v3<t>& Lhs, const v3<u>& Rhs)
{
  return v3<t>(Lhs.X << Rhs.X, Lhs.Y << Rhs.Y, Lhs.Z << Rhs.Z);
}


template <typename t, typename u> idx2_Inline v3<t>
operator<<(const v3<t>& Lhs, u Val)
{
  return v3<t>(Lhs.X << Val, Lhs.Y << Val, Lhs.Z << Val);
}


template <typename t, typename u> idx2_Inline v3<u>
operator<<(u Val, const v3<t>& Lhs)
{
  return v3<u>(Val << Lhs.X, Val << Lhs.Y, Val << Lhs.Z);
}


template <typename t> idx2_Inline v3<t>
Min(const v3<t>& Lhs, const v3<t>& Rhs)
{
  return v3<t>(Min(Lhs.X, Rhs.X), Min(Lhs.Y, Rhs.Y), Min(Lhs.Z, Rhs.Z));
}


template <typename t> idx2_Inline v3<t>
Max(const v3<t>& Lhs, const v3<t>& Rhs)
{
  return v3<t>(Max(Lhs.X, Rhs.X), Max(Lhs.Y, Rhs.Y), Max(Lhs.Z, Rhs.Z));
}


template <typename t> idx2_Inline v3<t>
Ceil(const v3<t>& Vec)
{
  return v3<t>(ceil(Vec.X), ceil(Vec.Y), ceil(Vec.Z));
}


template <typename t> idx2_Inline v2<t>
Ceil(const v2<t>& Vec)
{
  return v2<t>(ceil(Vec.X), ceil(Vec.Y));
}


/* v6 stuffs */

template <typename u, typename t> idx2_Inline t
Prod(const v6<u>& Vec)
{
  return Prod<t>(Vec.XYZ) * Prod<t>(Vec.UVW);
}


template <typename u, typename t> idx2_Inline t
Sum(const v6<u>& Vec)
{
  return Sum<t>(Vec.XYZ) + Sum<t>(Vec.UVW);
}


template <typename t> idx2_Inline v6<t>
operator+(const v6<t>& Lhs, const v6<t>& Rhs)
{
  return v6<t>(Lhs.XYZ + Rhs.XYZ, Lhs.UVW + Rhs.UVW);
}


template <typename t> idx2_Inline v6<t>
operator+(const v6<t>& Lhs, t Val)
{
  return v6<t>(Lhs.XYZ + Val, Lhs.UVW + Val);
}


template <typename t> idx2_Inline v6<t>
operator-(const v6<t>& Lhs, const v6<t>& Rhs)
{
  return v6<t>(Lhs.XYZ - Rhs.XYZ, Lhs.UVW - Rhs.UVW);
}


template <typename t> idx2_Inline v6<t>
operator-(const v6<t>& Lhs, t Val)
{
  return v6<t>(Lhs.XYZ - Val, Lhs.UVW - Val);
}


template <typename t> idx2_Inline v6<t>
operator-(t Val, const v6<t>& Lhs)
{
  return v6<t>(Val - Lhs.XYZ, Val - Lhs.UVW);
}


template <typename t> idx2_Inline v6<t>
operator*(const v6<t>& Lhs, const v6<t>& Rhs)
{
  return v6<t>(Lhs.XYZ * Rhs.XYZ, Lhs.UVW * Rhs.UVW);
}


template <typename t> idx2_Inline v6<t>
operator*(const v6<t>& Lhs, t Val)
{
  return v6<t>(Lhs.XYZ * Val, Lhs.UVW * Val);
}


template <typename t> idx2_Inline v6<t>
operator/(const v6<t>& Lhs, const v6<t>& Rhs)
{
  return v6<t>(Lhs.XYZ / Rhs.XYZ, Lhs.UVW / Rhs.UVW);
}


template <typename t> idx2_Inline v6<t>
operator/(const v6<t>& Lhs, t Val)
{
  return v6<t>(Lhs.XYZ / Val, Lhs.UVW / Val);
}


template <typename t> idx2_Inline v6<t>
operator&(const v6<t>& Lhs, const v6<t>& Rhs)
{
  return v6<t>(Lhs.XYZ & Rhs.XYZ, Lhs.UVW & Rhs.UVW);
}


template <typename t> idx2_Inline v6<t>
operator&(const v6<t>& Lhs, t Val)
{
  return v6<t>(Lhs.XYZ & Val, Lhs.UVW & Val);
}


template <typename t> idx2_Inline v6<t>
operator%(const v6<t>& Lhs, const v6<t>& Rhs)
{
  return v6<t>(Lhs.XYZ % Rhs.XYZ, Lhs.UVW % Rhs.UVW);
}


template <typename t> idx2_Inline v6<t>
operator%(const v6<t>& Lhs, t Val)
{
  return v6<t>(Lhs.XYZ % Val, Lhs.UVW % Val);
}


template <typename t> idx2_Inline bool
operator==(const v6<t>& Lhs, const v6<t>& Rhs)
{
  return (Lhs.XYZ == Rhs.XYZ) && (Lhs.UVW == Rhs.UVW);
}


template <typename t> idx2_Inline bool
operator!=(const v6<t>& Lhs, const v6<t>& Rhs)
{
  return (Lhs.XYZ != Rhs.XYZ) || (Lhs.UVW != Rhs.UVW);
}


template <typename t> idx2_Inline bool
operator==(const v6<t>& Lhs, t Val)
{
  return (Lhs.XYZ == Val) && (Lhs.UVW == Val);
}


template <typename t> idx2_Inline bool
operator!=(const v6<t>& Lhs, t Val)
{
  return (Lhs.XYZ != Val) || (Lhs.UVW != Val);
}


template <typename t> idx2_Inline bool
operator<=(const v6<t>& Lhs, const v6<t>& Rhs)
{
  return (Lhs.XYZ <= Rhs.XYZ) && (Lhs.UVW <= Rhs.UVW);
}


template <typename t> idx2_Inline bool
operator<(const v6<t>& Lhs, const v6<t>& Rhs)
{
  return (Lhs.XYZ < Rhs.XYZ) && (Lhs.UVW < Rhs.UVW);
}


template <typename t> idx2_Inline bool
operator>(const v6<t>& Lhs, const v6<t>& Rhs)
{
  return (Lhs.XYZ > Rhs.XYZ) && (Lhs.UVW > Rhs.UVW);
}


template <typename t> idx2_Inline bool
operator>(const v6<t>& Lhs, t Val)
{
  return (Lhs.XYZ > Val) && (Lhs.UVW > Val);
}


template <typename t> idx2_Inline bool
operator>=(const v6<t>& Lhs, const v6<t>& Rhs)
{
  return (Lhs.XYZ >= Rhs.XYZ) && (Lhs.UVW >= Rhs.UVW);
}


template <typename t> bool
operator<(t Val, const v6<t>& Rhs)
{
  return (Val < Rhs.XYZ) && (Val < Rhs.UVW);
}


template <typename t> bool
operator<(const v6<t>& Lhs, t Val)
{
  return (Lhs.XYZ < Val) && (Lhs.UVW < Val);
}


template <typename t> bool
operator<=(const v6<t>& Lhs, t Val)
{
  return (Lhs.XYZ <= Val) && (Lhs.UVW <= Val);
}


template <typename t> bool
operator<=(t Val, const v6<t>& Rhs)
{
  return (Val <= Rhs.XYZ) && (Val <= Rhs.UVW);
}


template <typename t, typename u> idx2_Inline v6<t>
operator>>(const v6<t>& Lhs, const v6<u>& Rhs)
{
  return v6<t>(Lhs.XYZ >> Rhs.XYZ, Lhs.UVW >> Rhs.UVW);
}


template <typename t, typename u> idx2_Inline v6<u>
operator>>(u Val, const v6<t>& Rhs)
{
  return v6<u>(Val >> Rhs.XYZ, Val >> Rhs.UVW);
}


template <typename t, typename u> idx2_Inline v6<t>
operator>>(const v6<t>& Lhs, u Val)
{
  return v6<t>(Lhs.XYZ >> Val, Lhs.UVW >> Val);
}


template <typename t, typename u> idx2_Inline v6<t>
operator<<(const v6<t>& Lhs, const v6<u>& Rhs)
{
  return v6<t>(Lhs.XYZ << Rhs.XYZ, Lhs.UVW << Rhs.UVW);
}


template <typename t, typename u> idx2_Inline v6<t>
operator<<(const v6<t>& Lhs, u Val)
{
  return v6<t>(Lhs.XYZ << Val, Lhs.UVW << Val);
}


template <typename t, typename u> idx2_Inline v6<u>
operator<<(u Val, const v6<t>& Rhs)
{
  return v6<u>(Val << Rhs.XYZ, Val << Rhs.UVW);
}


template <typename t> idx2_Inline v6<t>
Min(const v6<t>& Lhs, const v6<t>& Rhs)
{
  return v6<t>(Min(Lhs.XYZ, Rhs.XYZ), Min(Lhs.UVW, Rhs.UVW));
}


template <typename t> idx2_Inline v6<t>
Max(const v6<t>& Lhs, const v6<t>& Rhs)
{
  return v6<t>(Max(Lhs.XYZ, Rhs.XYZ), Max(Lhs.UVW, Rhs.UVW));
}


template <typename t> idx2_Inline v6<t>
Ceil(const v6<t>& Vec)
{
  return v6<t>(Ceil(Vec.XYZ), Ceil(Vec.UVW));
}

idx2_Inline i64
RoundDown(i64 Val, i64 M)
{
  return (Val / M) * M;
}


idx2_Inline i64
RoundUp(i64 Val, i64 M)
{
  return ((Val + M - 1) / M) * M;
}


idx2_Inline i8
Log2Floor(i64 Val)
{
  idx2_Assert(Val > 0);
  return Msb((u64)Val);
}


idx2_Inline i8
Log2Ceil(i64 Val)
{
  idx2_Assert(Val > 0);
  auto I = Msb((u64)Val);
  return I + i8(Val != (1ll << I));
}


idx2_Inline i8
Log8Floor(i64 Val)
{
  idx2_Assert(Val > 0);
  return Log2Floor(Val) / 3;
}


idx2_Inline int
GeometricSum(int Base, int N)
{
  idx2_Assert(N >= 0);
  return int((Pow(Base, N + 1) - 1) / (Base - 1));
}


// TODO: when n is already a power of two plus one, do not increase n
idx2_Inline i64
NextPow2(i64 Val)
{
  idx2_Assert(Val >= 0);
  if (Val == 0)
    return 1;
  return 1ll << (Msb((u64)(Val - 1)) + 1);
}


idx2_Inline i64
Pow(i64 Base, int Exp)
{
  idx2_Assert(Exp >= 0);
  i64 Result = 1;
  for (int I = 0; I < Exp; ++I)
    Result *= Base;
  return Result;
}


idx2_Inline v3i
Pow(const v3i& Base, int Exp)
{
  idx2_Assert(Exp >= 0);
  v3i Result(1);
  for (int I = 0; I < Exp; ++I)
    Result = Result * Base;
  return Result;
}


template <typename t> idx2_Inline t
Lerp(t V0, t V1, f64 T)
{
  idx2_Assert(0 <= T && T <= 1);
  return V0 + (V1 - V0) * T;
}


template <typename t> idx2_Inline t
BiLerp(t V00, t V10, t V01, t V11, const v2d& T)
{
  idx2_Assert(0.0 <= T && T <= 1.0);
  t V0 = Lerp(V00.X, V10.X, T.X);
  t V1 = Lerp(V01.X, V11.X, T.X);
  return Lerp(V0, V1, T.Y);
}


template <typename t> t
TriLerp(t V000, t V100, t V010, t V110, t V001, t V101, t V011, t V111, const v3d& T)
{
  idx2_Assert(0.0 <= T && T <= 1.0);
  t V0 = BiLerp(V000, V100, V010, V110, T.XY);
  t V1 = BiLerp(V001, V101, V011, V111, T.XY);
  return Lerp(V0, V1, T.Z);
}


template <typename t>
constexpr t GetMachineEpsilon()
{
  t Eps = t(0.5);
  t PrevEps;
  while ((1 + Eps) != 1)
  {
    PrevEps = Eps;
    Eps /= 2;
  }
  return Eps;
}


} // namespace idx2
