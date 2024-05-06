#pragma once

#include "Array.h"
#include "Common.h"
#include "DataTypes.h"
#include "Error.h"


namespace idx2
{


/*
In string form:
file = C:/My Data/my file.raw
name = combustion
field = o2
dimensions = 512 512 256
data type = float32
*/
struct metadata
{
  // char File[256] = "";
  char Name[64] = "";
  char Field[64] = "";
  v3i Dims3 = v3i(0);
  dtype DType = dtype(dtype::__Invalid__);
  inline thread_local static char String[384];
}; // struct metadata

cstr ToString(const metadata& Meta);
cstr ToRawFileName(const metadata& Meta);
error<> ReadMetaData(cstr FileName, metadata* Meta);
error<> StrToMetaData(stref FilePath, metadata* Meta);


} // namespace idx2
