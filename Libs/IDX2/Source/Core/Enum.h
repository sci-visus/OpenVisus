/*
An enum type that knows how to convert from and to strings.
There is always a special __Invalid__ enum item at the end.
NOTE: No checking is done for duplicate values.
*/

#pragma once

#include "Macros.h"
#include "String.h"


/* Example usage: idx2_Enum(error_type, u8, OutOfMemory, FileNotFound) */
#define idx2_Enum(enum_name, type, ...)                                                            \
                                                                                                   \
  namespace idx2                                                                                   \
  {                                                                                                \
                                                                                                   \
                                                                                                   \
  struct enum_name                                                                                 \
  {                                                                                                \
    enum : type                                                                                    \
    {                                                                                              \
      __Invalid__,                                                                                 \
      __VA_ARGS__                                                                                  \
    };                                                                                             \
    type Val;                                                                                      \
    enum_name();                                                                                   \
    enum_name(type Val);                                                                           \
    enum_name& operator=(type Val);                                                                \
    explicit enum_name(stref Name);                                                                \
    explicit operator bool() const;                                                                \
  }; /* struct enum_name */                                                                        \
                                                                                                   \
                                                                                                   \
  stref ToString(enum_name Enum);                                                                  \
                                                                                                   \
                                                                                                   \
  } // namespace idx2



namespace idx2
{


/* Construct an enum from a string */
template <typename t> struct StringTo
{
  t operator()(stref Name);
};


} // namespace idx2



#include "Common.h"
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>


#undef idx2_Enum
#define idx2_Enum(enum_name, type, ...)                                                            \
                                                                                                   \
  namespace idx2                                                                                   \
  {                                                                                                \
                                                                                                   \
                                                                                                   \
  enum class enum_name : type                                                                      \
  {                                                                                                \
    __VA_ARGS__,                                                                                   \
    __Invalid__                                                                                    \
  };                                                                                               \
                                                                                                   \
  struct idx2_Cat(enum_name, _s)                                                                   \
  {                                                                                                \
    enum_name Val;                                                                                 \
    struct enum_item                                                                               \
    {                                                                                              \
      stref Name;                                                                                  \
      enum_name ItemVal;                                                                           \
    };                                                                                             \
                                                                                                   \
    using name_map = stack_array<enum_item, idx2_NumArgs(__VA_ARGS__)>;                            \
                                                                                                   \
    inline static name_map NameMap = []()                                                          \
    {                                                                                              \
      name_map MyNameMap;                                                                          \
      tokenizer Tk1(idx2_Str(__VA_ARGS__), ",");                                                   \
      type CurrentVal = 0;                                                                         \
      for (int I = 0;; ++I, ++CurrentVal)                                                          \
      {                                                                                            \
        stref Token = Next(&Tk1);                                                                  \
        if (!Token)                                                                                \
          break;                                                                                   \
        tokenizer Tk2(Token, " =");                                                                \
        stref EnumStr = Next(&Tk2);                                                                \
        stref EnumVal = Next(&Tk2);                                                                \
        if (EnumVal)                                                                               \
        {                                                                                          \
          char* EndPtr = nullptr;                                                                  \
          errno = 0;                                                                               \
          enum_name MyVal = enum_name(strtol(EnumVal.Ptr, &EndPtr, 10));                           \
          if (errno == ERANGE || EndPtr == EnumVal.Ptr || !EndPtr ||                               \
              !(isspace(*EndPtr) || *EndPtr == ',' || *EndPtr == '\0'))                            \
            assert(false && " non-integer enum values");                                           \
          else if (MyVal < static_cast<enum_name>(CurrentVal))                                     \
            assert(false && " non-increasing enum values");                                        \
          else                                                                                     \
            CurrentVal = static_cast<type>(MyVal);                                                 \
        }                                                                                          \
        assert(I < Size(MyNameMap));                                                               \
        MyNameMap[I] = enum_item{ EnumStr, static_cast<enum_name>(CurrentVal) };                   \
      }                                                                                            \
      return MyNameMap;                                                                            \
    }();                                                                                           \
                                                                                                   \
                                                                                                   \
    idx2_Cat(enum_name, _s)()                                                                      \
      : idx2_Cat(enum_name, _s)(enum_name::__Invalid__)                                            \
    {                                                                                              \
    }                                                                                              \
                                                                                                   \
                                                                                                   \
    idx2_Cat(enum_name, _s)(enum_name Value)                                                       \
    {                                                                                              \
      auto* It = Begin(NameMap);                                                                   \
      while (It != End(NameMap))                                                                   \
      {                                                                                            \
        if (It->ItemVal == Value)                                                                  \
          break;                                                                                   \
        ++It;                                                                                      \
      }                                                                                            \
      this->Val = (It != End(NameMap)) ? It->ItemVal : enum_name::__Invalid__;                     \
    }                                                                                              \
                                                                                                   \
                                                                                                   \
    explicit idx2_Cat(enum_name, _s)(stref Name)                                                   \
    {                                                                                              \
      auto* It = Begin(NameMap);                                                                   \
      while (It != End(NameMap))                                                                   \
      {                                                                                            \
        if (It->Name == Name)                                                                      \
          break;                                                                                   \
        ++It;                                                                                      \
      }                                                                                            \
      Val = (It != End(NameMap)) ? It->ItemVal : enum_name::__Invalid__;                           \
    }                                                                                              \
                                                                                                   \
                                                                                                   \
    explicit operator bool() const { return Val != enum_name::__Invalid__; }                       \
  };                                                                                               \
                                                                                                   \
                                                                                                   \
  inline stref                                                                                     \
  ToString(enum_name Enum)                                                                         \
  {                                                                                                \
    idx2_Cat(enum_name, _s) EnumS(Enum);                                                           \
    auto* It = Begin(EnumS.NameMap);                                                               \
    while (It != End(EnumS.NameMap))                                                               \
    {                                                                                              \
      if (It->ItemVal == EnumS.Val)                                                                \
        break;                                                                                     \
      ++It;                                                                                        \
    }                                                                                              \
    assert(It != End(EnumS.NameMap));                                                              \
    return It->Name;                                                                               \
  }                                                                                                \
                                                                                                   \
                                                                                                   \
  template <> struct StringTo<enum_name>                                                           \
  {                                                                                                \
    enum_name                                                                                      \
    operator()(stref Name)                                                                         \
    {                                                                                              \
      idx2_Cat(enum_name, _s) EnumS(Name);                                                         \
      return EnumS.Val;                                                                            \
    }                                                                                              \
  };                                                                                               \
                                                                                                   \
                                                                                                   \
  inline bool                                                                                      \
  IsValid(enum_name Enum)                                                                          \
  {                                                                                                \
    idx2_Cat(enum_name, _s) EnumS(Enum);                                                           \
    return EnumS.Val != enum_name::__Invalid__;                                                    \
  }                                                                                                \
                                                                                                   \
                                                                                                   \
  } // namespace idx2
