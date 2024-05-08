#pragma once

#include "Macros.h"
#include <float.h>
#include <inttypes.h>
#include <stdint.h>
//#include <crtdbg.h>


namespace idx2
{


#define idx2_NumberTypes int8, uint8, int16, uint16, int32, uint32, int64, uint64, float32, float64

using uint = unsigned int;
using byte = uint8_t;
using int8 = int8_t;
using i8 = int8;
using int16 = int16_t;
using i16 = int16;
using int32 = int32_t;
using i32 = int32;
using int64 = int64_t;
using i64 = int64;
using uint8 = uint8_t;
using u8 = uint8;
using uint16 = uint16_t;
using u16 = uint16;
using uint32 = uint32_t;
using u32 = uint32;
using uint64 = uint64_t;
using u64 = uint64;
using float32 = float;
using f32 = float32;
using float64 = double;
using f64 = float64;
using str = char*;
using cstr = const char*;

template <typename t> struct traits
{
  // using signed_t =
  // using unsigned_t =
  // using integral_t =
  // static constexpr uint NBinaryMask =
  // static constexpr int ExpBits
  // static constexpr int ExpBias
};

/* type traits stuffs */

struct true_type
{
  static constexpr bool Value = true;
};

struct false_type
{
  static constexpr bool Value = false;
};

template <typename t> struct remove_const
{
  typedef t type;
};

template <typename t> struct remove_const<const t>
{
  typedef t type;
};

template <typename t> struct remove_volatile
{
  typedef t type;
};

template <typename t> struct remove_volatile<volatile t>
{
  typedef t type;
};

template <typename t> struct remove_cv
{
  typedef typename remove_volatile<typename remove_const<t>::type>::type type;
};

template <typename t> struct remove_reference
{
  typedef t type;
};

template <typename t> struct remove_reference<t&>
{
  typedef t type;
};

template <typename t> struct remove_reference<t&&>
{
  typedef t type;
};

template <typename t> struct remove_cv_ref
{
  typedef typename remove_cv<typename remove_reference<t>::type>::type type;
};

template <typename t1, typename t2> struct is_same_type : false_type
{
};

template <typename t> struct is_same_type<t, t> : true_type
{
};

template <typename t> struct is_pointer_helper : false_type
{
};

template <typename t> struct is_pointer_helper<t*> : true_type
{
};

template <typename t> struct is_pointer : is_pointer_helper<typename remove_cv<t>::type>
{
};

template <typename t> auto& Value(t&& T);

template <typename t> struct is_integral : false_type
{
};

template <> struct is_integral<i8> : true_type
{
};

template <> struct is_integral<u8> : true_type
{
};

template <> struct is_integral<i16> : true_type
{
};

template <> struct is_integral<u16> : true_type
{
};

template <> struct is_integral<i32> : true_type
{
};

template <> struct is_integral<u32> : true_type
{
};

template <> struct is_integral<i64> : true_type
{
};

template <> struct is_integral<u64> : true_type
{
};

template <typename t> struct is_signed : false_type
{
};

template <> struct is_signed<i8> : true_type
{
};

template <> struct is_signed<i16> : true_type
{
};

template <> struct is_signed<i32> : true_type
{
};

template <> struct is_signed<i64> : true_type
{
};

template <typename t> struct is_unsigned : false_type
{
};

template <> struct is_unsigned<u8> : true_type
{
};

template <> struct is_unsigned<u16> : true_type
{
};

template <> struct is_unsigned<u32> : true_type
{
};

template <> struct is_unsigned<u64> : true_type
{
};

template <typename t> struct is_floating_point : false_type
{
};

template <> struct is_floating_point<f32> : true_type
{
};

template <> struct is_floating_point<f64> : true_type
{
};

/* Something to replace std::array */
template <typename t, int N> struct stack_array
{
  static_assert(N > 0);
  // constexpr stack_array() = default;
  t Arr[N] = {};
  u8 Len = 0;
  t& operator[](int Idx) const;
};

template <typename t, int N> t* Begin(const stack_array<t, N>& A);
template <typename t, int N> t* End(const stack_array<t, N>& A);
template <typename t, int N> t* RevBegin(const stack_array<t, N>& A);
template <typename t, int N> t* RevEnd(const stack_array<t, N>& A);
template <typename t, int N> constexpr int Size(const stack_array<t, N>& A);

template <int N> struct stack_string
{
  char Data[N] = {};
  u8 Len = 0;
  char& operator[](int Idx) const;
};

template <int N> int Size(const stack_string<N>& S);

template <typename t, typename u> struct t2
{
  t First;
  u Second;
  idx2_Inline bool
  operator<(const t2& Other) const
  {
    return First < Other.First;
  }
};

/* Vector in 2D, supports .X, .UV, and [] */
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

template <typename t> struct v2
{
  union
  {
    struct
    {
      t X, Y;
    };
    struct
    {
      t U, V;
    };
    struct
    {
      t Min, Max;
    };
    t E[2];
  };
  // inline static constexpr v2<t> Zero = v2<t>(0);
  // inline static constexpr v2<t> One  = v2<t>(1);
  v2();
  explicit constexpr v2(t V);
  constexpr v2(t X_, t Y_);
  template <typename u> v2(const v2<u>& Other);
  t& operator[](int Idx) const;
  template <typename u> v2& operator=(const v2<u>& Rhs);
};

using v2i = v2<i32>;
using v2u = v2<u32>;
using v2l = v2<i64>;
using v2ul = v2<u64>;
using v2f = v2<f32>;
using v2d = v2<f64>;

/* Vector in 3D, supports .X, .XY, .UV, .RGB and [] */
template <typename t> struct v3
{
  union
  {
    struct
    {
      t X, Y, Z;
    };
    struct
    {
      t U, V, __;
    };
    struct
    {
      t R, G, B;
    };
    struct
    {
      v2<t> XY;
      t Ignored0_;
    };
    struct
    {
      t Ignored1_;
      v2<t> YZ;
    };
    struct
    {
      v2<t> UV;
      t Ignored2_;
    };
    struct
    {
      t Ignored3_;
      v2<t> V__;
    };
    t E[3];
  };
  // inline static const v3 Zero = v3(0);
  // inline static const v3 One = v3(1);
  v3();
  explicit constexpr v3(t V);
  constexpr v3(t X_, t Y_, t Z_);
  v3(const v2<t>& V2, t Z_);
  template <typename u> v3(const v3<u>& Other);
  t& operator[](int Idx) const;
  template <typename u> v3& operator=(const v3<u>& Rhs);
};

#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

using v3i = v3<i32>;
using v3u = v3<u32>;
using v3l = v3<i64>;
using v3ul = v3<u64>;
using v3f = v3<f32>;
using v3d = v3<f64>;

#define idx2_PrStrV3i "(%d %d %d)"
#define idx2_PrV3i(P) (P).X, (P).Y, (P).Z

/* 3-level nested for loop */
#define idx2_BeginFor3(Counter, Begin, End, Step)
#define idx2_EndFor3
#define idx2_BeginFor3Lockstep(C1, B1, E1, S1, C2, B2, E2, S2)


template <typename t> struct v6
{
  union
  {
    struct
    {
      v3<t> XYZ;
      v3<t> UVW;
    };
    t E[6];
  };
  v6();
  explicit constexpr v6(t V);
  constexpr v6(t X_, t Y_, t Z_, t U_, t V_, t W_);
  template <typename u> v6(const v3<u>& P1, const v3<u>& P2);
  template <typename u> v6(const v6<u>& Other);
  t& operator[](int Idx) const;
  template <typename u> v6& operator=(const v6<u>& Rhs);
  idx2_Inline static i8 Dims() { return sizeof(E) / sizeof(E[0]); }
};

using v6i = v6<i32>;
using v6u = v6<u32>;
using v6l = v6<i64>;
using v6ul = v6<u64>;
using v6f = v6<f32>;
using v6d = v6<f64>;

using nd_string = v6<u8>;
using nd_index = v6i;
using nd_size = v6i;


} // namespace idx2


#include <assert.h>
#include <stdio.h>


namespace idx2
{


template <typename t> idx2_Inline auto&
Value(t&& T)
{
  if constexpr (is_pointer<typename remove_reference<t>::type>::Value)
    return *T;
  else
    return T;
}


template <> struct traits<i8>
{
  using signed_t = i8;
  using unsigned_t = u8;
  static constexpr u8 NBinaryMask = 0xaa;
  static constexpr i8 Min = -(1 << 7);
  static constexpr i8 Max = (1 << 7) - 1;
};


template <> struct traits<u8>
{
  using signed_t = i8;
  using unsigned_t = u8;
  static constexpr u8 NBinaryMask = 0xaa;
  static constexpr u8 Min = 0;
  static constexpr u8 Max = (1 << 8) - 1;
};


template <> struct traits<i16>
{
  using signed_t = i16;
  using unsigned_t = u16;
  static constexpr u16 NBinaryMask = 0xaaaa;
  static constexpr i16 Min = -(1 << 15);
  static constexpr i16 Max = (1 << 15) - 1;
};


template <> struct traits<u16>
{
  using signed_t = i16;
  using unsigned_t = u16;
  static constexpr u16 NBinaryMask = 0xaaaa;
  static constexpr u16 Min = 0;
  static constexpr u16 Max = (1 << 16) - 1;
};


template <> struct traits<i32>
{
  using signed_t = i32;
  using unsigned_t = u32;
  using floating_t = f32;
  static constexpr u32 NBinaryMask = 0xaaaaaaaa;
  static constexpr i32 Min = i32(0x80000000);
  static constexpr i32 Max = 0x7fffffff;
};


template <> struct traits<u32>
{
  using signed_t = i32;
  using unsigned_t = u32;
  static constexpr u32 NBinaryMask = 0xaaaaaaaa;
  static constexpr u32 Min = 0;
  static constexpr u32 Max = 0xffffffff;
};


template <> struct traits<i64>
{
  using signed_t = i64;
  using unsigned_t = u64;
  using floating_t = f64;
  static constexpr u64 NBinaryMask = 0xaaaaaaaaaaaaaaaaULL;
  static constexpr i64 Min = 0x8000000000000000ll;
  static constexpr i64 Max = 0x7fffffffffffffffull;
};


template <> struct traits<u64>
{
  using signed_t = i64;
  using unsigned_t = u64;
  static constexpr u64 NBinaryMask = 0xaaaaaaaaaaaaaaaaULL;
  static constexpr u64 Min = 0;
  static constexpr u64 Max = 0xffffffffffffffffull;
};


template <> struct traits<f32>
{
  using integral_t = i32;
  static constexpr int ExpBits = 8;
  static constexpr int ExpBias = (1 << (ExpBits - 1)) - 1;
  static constexpr f32 Min = -FLT_MAX;
  static constexpr f32 Max = FLT_MAX;
};


template <> struct traits<f64>
{
  using integral_t = i64;
  static constexpr int ExpBits = 11;
  static constexpr int ExpBias = (1 << (ExpBits - 1)) - 1;
  static constexpr f64 Min = -DBL_MAX;
  static constexpr f64 Max = DBL_MAX;
};


template <typename t, int N> idx2_Inline t&
stack_array<t, N>::operator[](int Idx) const
{
  assert(Idx < N);
  return const_cast<t&>(Arr[Idx]);
}


template <typename t, int N> idx2_Inline t*
Begin(const stack_array<t, N>& A)
{
  return const_cast<t*>(&A.Arr[0]);
}


template <typename t, int N> idx2_Inline t*
End(const stack_array<t, N>& A)
{
  return const_cast<t*>(&A.Arr[0]) + N;
}


template <typename t, int N> idx2_Inline t*
RevBegin(const stack_array<t, N>& A)
{
  return const_cast<t*>(&A.Arr[0]) + (N - 1);
}


template <typename t, int N> idx2_Inline t*
RevEnd(const stack_array<t, N>& A)
{
  return const_cast<t*>(&A.Arr[0]) - 1;
}


template <typename t, int N> idx2_Inline constexpr int
Size(const stack_array<t, N>&)
{
  return N;
}


template <int N> idx2_Inline char&
stack_string<N>::operator[](int Idx) const
{
  assert(Idx < N);
  return const_cast<char&>(Data[Idx]);
}


template <int N> idx2_Inline int
Size(const stack_string<N>& S)
{
  return S.Len;
}


/* v2 stuffs */
template <typename t> idx2_Inline
v2<t>::v2()
{
}


template <typename t> idx2_Inline constexpr v2<t>::v2(t V)
  : X(V)
  , Y(V)
{
}


template <typename t> idx2_Inline constexpr v2<t>::v2(t X_, t Y_)
  : X(X_)
  , Y(Y_)
{
}


template <typename t> template <typename u> idx2_Inline
v2<t>::v2(const v2<u>& Other)
  : X(Other.X)
  , Y(Other.Y)
{
}


template <typename t> idx2_Inline t&
v2<t>::operator[](int Idx) const
{
  assert(Idx < 2);
  return const_cast<t&>(E[Idx]);
}


template <typename t> template <typename u> idx2_Inline v2<t>&
v2<t>::operator=(const v2<u>& other)
{
  X = other.X;
  Y = other.Y;
  return *this;
}


/* v3 stuffs */
template <typename t> idx2_Inline
v3<t>::v3()
{
}


template <typename t> idx2_Inline constexpr v3<t>::v3(t V)
  : X(V)
  , Y(V)
  , Z(V)
{
}

template <typename t> idx2_Inline constexpr v3<t>::v3(t X_, t Y_, t Z_)
  : X(X_)
  , Y(Y_)
  , Z(Z_)
{
}

template <typename t> idx2_Inline
v3<t>::v3(const v2<t>& V2, t Z_)
  : X(V2.X)
  , Y(V2.Y)
  , Z(Z_)
{


}

template <typename t> template <typename u> idx2_Inline
v3<t>::v3(const v3<u>& Other)
  : X(Other.X)
  , Y(Other.Y)
  , Z(Other.Z)
{
}


template <typename t> idx2_Inline t&
v3<t>::operator[](int Idx) const
{
  assert(Idx < 3);
  return const_cast<t&>(E[Idx]);
}


template <typename t> template <typename u> idx2_Inline v3<t>&
v3<t>::operator=(const v3<u>& Rhs)
{
  X = Rhs.X;
  Y = Rhs.Y;
  Z = Rhs.Z;
  return *this;
}

/* v6 stuffs*/
template <typename t> idx2_Inline
v6<t>::v6()
  : XYZ(1)
  , UVW(1)
{
}


template <typename t> idx2_Inline constexpr v6<t>::v6(t V)
  : XYZ(V)
  , UVW(V)
{
}

template <typename t> idx2_Inline constexpr v6<t>::v6(t X_, t Y_, t Z_, t U_, t V_, t W_)
  : XYZ(X_, Y_, Z_)
  , UVW(U_, V_, W_)
{
}

template <typename t> template <typename u> idx2_Inline
v6<t>::v6(const v3<u>& XYZ_, const v3<u>& UVW_)
  : XYZ(XYZ_)
  , UVW(UVW_)
{
}

template <typename t> template <typename u> idx2_Inline
v6<t>::v6(const v6<u>& Other)
  : XYZ(Other.XYZ)
  , UVW(Other.UVW)
{
}

template <typename t> idx2_Inline t&
v6<t>::operator[](int Idx) const
{
  assert(Idx < 6);
  return const_cast<t&>(E[Idx]);
}


template <typename t> template <typename u> idx2_Inline v6<t>&
v6<t>::operator=(const v6<u>& Rhs)
{
  XYZ = Rhs.XYZ;
  UVW = Rhs.UVW;
  return *this;
}

// TODO: move the following to Macros.h?
#undef idx2_BeginFor3
#define idx2_BeginFor3(Counter, Begin, End, Step)                                                  \
  for (Counter.Z = (Begin).Z; Counter.Z < (End).Z; Counter.Z += (Step).Z)                          \
  {                                                                                                \
    for (Counter.Y = (Begin).Y; Counter.Y < (End).Y; Counter.Y += (Step).Y)                        \
    {                                                                                              \
      for (Counter.X = (Begin).X; Counter.X < (End).X; Counter.X += (Step).X)

#undef idx2_EndFor3
#define idx2_EndFor3                                                                               \
  }                                                                                                \
  }


#undef idx2_BeginFor3Lockstep
#define idx2_BeginFor3Lockstep(C1, B1, E1, S1, C2, B2, E2, S2)                                     \
  (void)E2;                                                                                        \
  for (C1.Z = (B1).Z, C2.Z = (B2).Z; C1.Z < (E1).Z; C1.Z += (S1).Z, C2.Z += (S2).Z)                \
  {                                                                                                \
    for (C1.Y = (B1).Y, C2.Y = (B2).Y; C1.Y < (E1).Y; C1.Y += (S1).Y, C2.Y += (S2).Y)              \
    {                                                                                              \
      for (C1.X = (B1).X, C2.X = (B2).X; C1.X < (E1).X; C1.X += (S1).X, C2.X += (S2).X)



} // namespace idx2

