#pragma once

#include "Common.h"
#include "Error.h"
#include "Macros.h"


namespace idx2
{


/* Store either a value or an error */
template <typename t, typename u = err_code> struct expected
{
  union
  {
    t Val;
    error<u> Err;
  };
  bool Ok = false;
  expected();
  expected(const t& ValIn);
  expected(const error<u>& ErrIn);

  /* Get the value through the pointer syntax */
  t& operator*();
  /* Mimic pointer semantics */
  t* operator->();
  /* Bool operator */
  explicit operator bool() const;
  virtual ~expected()
  {
    if (!Ok)
      Val.~t();
  }
}; // struct expected

template <typename t, typename u> t& Value(expected<t, u>& E);
template <typename t, typename u> error<u>& Error(expected<t, u>& E);


} // namespace idx2



#include "Assert.h"


namespace idx2
{


template <typename t, typename u> idx2_Inline
expected<t, u>::expected()
  : Ok(false)
{
}

template <typename t, typename u> idx2_Inline
expected<t, u>::expected(const t& ValIn)
  : Val(ValIn)
  , Ok(true)
{
}

template <typename t, typename u> idx2_Inline
expected<t, u>::expected(const error<u>& ErrIn)
  : Err(ErrIn)
  , Ok(false)
{
}

template <typename t, typename u> idx2_Inline t&
expected<t, u>::operator*()
{
  return Val;
}

/* (Safely) get the value with the added check */
template <typename t, typename u> idx2_Inline t&
Value(expected<t, u>& E)
{
  idx2_Assert(E.Ok);
  return E.Val;
}

template <typename t, typename u> idx2_Inline t*
expected<t, u>::operator->()
{
  return &**this;
}

/* Get the error */
template <typename t, typename u> idx2_Inline error<u>&
Error(expected<t, u>& E)
{
  idx2_Assert(!E.Ok);
  return E.Err;
}

/* Bool operator */
template <typename t, typename u> idx2_Inline expected<t, u>::operator bool() const
{
  return Ok;
}


} // namespace idx2
