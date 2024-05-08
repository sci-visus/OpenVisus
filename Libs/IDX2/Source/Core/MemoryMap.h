// TODO: implement a sliding-window approach, since a file mapping view can be limited in size
// TODO: add MAP_NORESERVE on Linux

#pragma once

#include "Common.h"
#include "Enum.h"
#include "Error.h"


#if defined(_WIN32)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>
#elif defined(__CYGWIN__) || defined(__linux__) || defined(__APPLE__)
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

// TODO: create a mapping that is not backed by a file

idx2_Enum(mmap_err_code,
          int,
          idx2_CommonErrs,
          MappingFailed,
          MapViewFailed,
          AllocateFailed,
          FlushFailed,
          SyncFailed,
          UnmapFailed);


namespace idx2
{


enum class map_mode
{
  Read,
  Write
};

#if defined(_WIN32)
using file_handle = HANDLE;
#elif defined(__CYGWIN__) || defined(__linux__) || defined(__APPLE__)
using file_handle = int;
#endif

struct mmap_file
{
  file_handle File;
  file_handle FileMapping;
  map_mode Mode;
  buffer Buf;
};

error<mmap_err_code>
OpenFile(mmap_file* MMap, cstr Name, map_mode Mode);

error<mmap_err_code>
MapFile(mmap_file* MMap, i64 Bytes = 0);

error<mmap_err_code>
FlushFile(mmap_file* MMap, byte* Start = nullptr, i64 Bytes = 0);

error<mmap_err_code>
SyncFile(mmap_file* MMap);

error<mmap_err_code>
UnmapFile(mmap_file* MMap);

error<mmap_err_code>
CloseFile(mmap_file* MMap);

template <typename t> void
Write(mmap_file* MMap, const t* Data);

template <typename t> void
Write(mmap_file* MMap, const t* Data, i64 Size);

template <typename t> void
Write(mmap_file* MMap, t Val);


} // namespace idx2



#include <string.h>


namespace idx2
{


template <typename t> void
Write(mmap_file* MMap, const t* Data, i64 Size)
{
  memcpy(MMap->Buf.Data + MMap->Buf.Bytes, Data, Size);
  MMap->Buf.Bytes += Size;
}


template <typename t> idx2_Inline void
Write(mmap_file* MMap, const t* Data)
{
  Write(MMap, Data, sizeof(t));
}


template <typename t> idx2_Inline void
Write(mmap_file* MMap, t Val)
{
  Write(MMap, &Val, sizeof(t));
}


} // namespace idx2
