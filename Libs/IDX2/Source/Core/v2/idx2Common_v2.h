#pragma once


#include "../idx2Common.h"
#include "../Common.h"
#include "../Array.h"
#include "../Error.h"


namespace idx2
{


struct dimension_info
{
  /* A dimension can either be numerical or categorical.
  In the latter case, each category is given a name (e.g., a field).
  In the former case, the dimension starts at 0 and has an upper limit. */
  array<stref> Names;
  i32 Limit = 0; // exclusive upper limit
  char ShortName = '?';
};

idx2_Inline i32
Size(const dimension_info& Dim)
{
  return Size(Dim.Names) > 0 ? i32(Size(Dim.Names)) : Dim.Limit;
}

bool
OptVal(i32 NArgs, cstr* Args, cstr Opt, array<dimension_info>* Dimensions);


struct indexing_template
{
  /* A string that looks like
  "fffffttttttttttzzz|zyx:zyx:zyx:zyx:tyx:tyx:tyx:tx:yx:yx:yx:yx:yx:yx"
  The '|' separates the static prefix from the dynamic suffix.
  Only the suffix is used for multiresolution indexing, the prefix is kept constant. */
  stref Full;
  /* The zyx:zyx:zyx:zyx:tyx:tyx:tyx:tx:yx:yx:yx:yx:yx:yx part; each : denotes a new level. */
  array<array<i8>> Suffix;
  /* The fffffttttttttttzzz part. */
  array<i8> Prefix;
};

struct idx2_file_v2
{
  static constexpr int MaxNameLength_ = 64;
  static constexpr int MaxNLevels_ = 32;
  static constexpr int MaxNDimsPerLevel_ = 3; // for now our zfp compression only supports up to 3 dimensions

  /* For example, xyztf */
  array<dimension_info> Dimensions;
  indexing_template IdxTemplate;
  i8 CharToIntMap['z' - 'a' + 1]; // map from ['a' - 'a', 'z' - 'a'] -> [0, Size(Idx2->Dimensions)]
  char Name[MaxNameLength_] = {};
  i8 BitsPerBrick      = 15; // (2^15) equivalent to 32^3
  i8 BrickBitsPerChunk = 12; // (2^12) equivalent to 16^3
  i8 ChunkBitsPerFile  = 0; // TODO_v2: choose a default
  i8 FileBitsPerDir    = 0; // TODO_v2: choose a default
  i8 FileBitsPerMeta   = 0; // TODO_v2: choose a default

  idx2_file_v2();
};

error<idx2_err_code>
Finalize(idx2_file_v2* Idx2);


void
Dealloc(idx2_file_v2* Idx2);


} // namespace idx2

