#pragma once

#include "Array.h"
#include "Common.h"
#include "Error.h"
#include "Memory.h"


#define idx2_FSeek
#define idx2_FTell


namespace idx2
{


/*
Read a text file from disk into a buffer. The buffer can be nullptr or it can be
initialized in advance, in which case the existing memory will be reused if the
file can fit in it. The caller is responsible to deallocate the memory. */
error<>
ReadFile(cstr FileName, buffer* Buf);

error<>
WriteBuffer(cstr FileName, const buffer& Buf);

/* Dump a range of stuffs into a text file */
template <typename i> error<>
DumpText(cstr FileName, i Begin, i End, cstr Format);


} // namespace idx2


#include "ScopeGuard.h"
#include <assert.h>
#include <stdio.h>

#undef idx2_FSeek
#undef idx2_FTell
/* Enable support for reading large files */
#if defined(_WIN32)
#define idx2_FSeek _fseeki64
#define idx2_FTell _ftelli64
#elif defined(__CYGWIN__) || defined(__linux__) || defined(__APPLE__)
#define _FILE_OFFSET_BITS 64
#define idx2_FSeek fseeko
#define idx2_FTell ftello
#endif



namespace idx2
{


#define idx2_OpenExistingFile(Fp, FileName, Mode) FILE* Fp = fopen(FileName, Mode)

#define idx2_OpenMaybeExistingFile(Fp, FileName, Mode)                                             \
  idx2_RAII(FILE*, Fp = fopen(FileName, Mode), , if (Fp) fclose(Fp));                              \
  if (!Fp)                                                                                         \
  {                                                                                                \
    CreateFullDir(GetParentPath(FileName));                                                           \
    Fp = fopen(FileName, Mode);                                                                    \
    idx2_Assert(Fp);                                                                               \
  }


template <typename i> error<>
DumpText(cstr FileName, i Begin, i End, cstr Format)
{
  FILE* Fp = fopen(FileName, "w");
  idx2_CleanUp(0, if (Fp) fclose(Fp));
  if (!Fp)
    return idx2_Error(err_code::FileCreateFailed, "%s", FileName);
  for (i It = Begin; It != End; ++It)
  {
    if (fprintf(Fp, Format, *It) < 0)
      return idx2_Error(err_code::FileWriteFailed, "");
  }
  return idx2_Error(err_code::NoError, "");
}


template <typename t> void
WritePOD(FILE* Fp, const t Var)
{
  fwrite(&Var, sizeof(Var), 1, Fp);
}


template <typename t> idx2_Inline void
ReadPOD(FILE* Fp, t* Val)
{
  fread(Val, sizeof(t), 1, Fp);
}


template <typename t> idx2_Inline void
ReadBackwardPOD(FILE* Fp, t* Val)
{
  auto Where = idx2_FTell(Fp);
  idx2_FSeek(Fp, Where -= sizeof(t), SEEK_SET);
  fread(Val, sizeof(t), 1, Fp);
  idx2_FSeek(Fp, Where, SEEK_SET);
}


idx2_Inline void
WriteBuffer(FILE* Fp, const buffer& Buf)
{
  fwrite(Buf.Data, Size(Buf), 1, Fp);
}


idx2_Inline void
WriteBuffer(FILE* Fp, const buffer& Buf, i64 Sz)
{
  fwrite(Buf.Data, Sz, 1, Fp);
}


idx2_Inline void
ReadBuffer(FILE* Fp, buffer* Buf)
{
  fread(Buf->Data, Size(*Buf), 1, Fp);
}


idx2_Inline void
ReadBuffer(FILE* Fp, buffer* Buf, i64 Sz)
{
  fread(Buf->Data, Sz, 1, Fp);
}


template <typename t> idx2_Inline void
ReadBuffer(FILE* Fp, buffer_t<t>* Buf)
{
  fread(Buf->Data, Bytes(*Buf), 1, Fp);
}


idx2_Inline void
ReadBackwardBuffer(FILE* Fp, buffer* Buf)
{
  auto Where = idx2_FTell(Fp);
  idx2_FSeek(Fp, Where -= Size(*Buf), SEEK_SET);
  fread(Buf->Data, Size(*Buf), 1, Fp);
  idx2_FSeek(Fp, Where, SEEK_SET);
}


idx2_Inline void
ReadBackwardBuffer(FILE* Fp, buffer* Buf, i64 Sz)
{
  assert(Sz <= Size(*Buf));
  auto Where = idx2_FTell(Fp);
  idx2_FSeek(Fp, Where -= Sz, SEEK_SET);
  fread(Buf->Data, Sz, 1, Fp);
  idx2_FSeek(Fp, Where, SEEK_SET);
}


template <int N> void
ReadLines(FILE* Fp, array<stack_array<char, N>>* Lines)
{
  while (true)
  {
    char Temp[N];
    if (!fgets(Temp, N, Fp))
      return;

    stref Src(Temp);
    if (Temp[Src.Size - 1] == '\n')
      Temp[Src.Size - 2] = 0; // override the new line character
    PushBack(Lines);
    stref Dst(Back(*Lines).Arr, N);
    Copy(Src, &Dst, true);
  }
}


} // namespace idx2
