#include "MemoryMap.h"
#include "Assert.h"
#include "FileUtilities.h"


namespace idx2
{


error<mmap_err_code>
OpenFile(mmap_file* MMap, cstr Name, map_mode Mode)
{
#if defined(_WIN32)
  MMap->File = CreateFileA(Name,
                           Mode == map_mode::Read ? GENERIC_READ : GENERIC_READ | GENERIC_WRITE,
                           0,
                           NULL,
                           OPEN_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);
  if (MMap->File == INVALID_HANDLE_VALUE)
    return idx2_Error(mmap_err_code::FileCreateFailed, "file: %s\n", Name);
#elif defined(__CYGWIN__) || defined(__linux__) || defined(__APPLE__)
  MMap->File = open(Name, Mode == map_mode::Read ? O_RDONLY : O_RDWR | O_CREAT | O_TRUNC, 0600);
  if (MMap->File == -1)
    return idx2_Error(mmap_err_code::FileCreateFailed, "file: %s\n", Name);
#endif
  MMap->Mode = Mode;
  return idx2_Error(mmap_err_code::NoError);
}


#if defined(__APPLE__)
static bool
mac_fallocate(file_handle fd, i64 aLength)
{
  fstore_t store = { F_ALLOCATECONTIG, F_PEOFPOSMODE, 0, aLength };
  // Try to get a continous chunk of disk space
  int ret = fcntl(fd, F_PREALLOCATE, &store);
  if (-1 == ret)
  {
    // OK, perhaps we are too fragmented, allocate non-continuous
    store.fst_flags = F_ALLOCATEALL;
    ret = fcntl(fd, F_PREALLOCATE, &store);
    if (-1 == ret)
      return false;
  }
  return 0 == ftruncate(fd, aLength);

  return false;
}
#endif


/* Size is only used when Mode is Write or ReadWrite */
error<mmap_err_code>
MapFile(mmap_file* MMap, i64 Bytes)
{
#if defined(_WIN32)
  LARGE_INTEGER FileSize{ { 0, 0 } };
  if (!GetFileSizeEx(MMap->File, &FileSize) || Bytes != 0)
    FileSize.QuadPart = Bytes;
  MMap->FileMapping =
    CreateFileMapping(MMap->File,
                      NULL,
                      MMap->Mode == map_mode::Read ? PAGE_READONLY : PAGE_READWRITE,
                      FileSize.HighPart,
                      FileSize.LowPart,
                      0);
  if (MMap->FileMapping == NULL)
    return idx2_Error(mmap_err_code::MappingFailed);

  LPVOID MapAddress = MapViewOfFile(MMap->FileMapping,
                                    MMap->Mode == map_mode::Read    ? FILE_MAP_READ
                                    : MMap->Mode == map_mode::Write ? FILE_MAP_WRITE
                                                                    : FILE_MAP_ALL_ACCESS,
                                    0,
                                    0,
                                    0);
  if (MapAddress == NULL)
    return idx2_Error(mmap_err_code::MapViewFailed);
  MMap->Buf.Data = (byte*)MapAddress;
  MMap->Buf.Bytes = FileSize.QuadPart;
#elif defined(__CYGWIN__) || defined(__linux__) || defined(__APPLE__)
  size_t FileSize;
  struct ::stat Stat;
  if (Bytes != 0)
    FileSize = Bytes;
  else if (fstat(MMap->File, &Stat) == 0)
    FileSize = Stat.st_size;
  if (MMap->Mode == map_mode::Write)
#if defined(__APPLE__)
    if (!mac_fallocate(MMap->File, FileSize))
      return idx2_Error(mmap_err_code::AllocateFailed);
#else
    if (posix_fallocate(MMap->File, 0, FileSize) == -1)
      return idx2_Error(mmap_err_code::AllocateFailed);
#endif
  void* MapAddress = mmap(0,
                          FileSize,
                          MMap->Mode == map_mode::Read ? PROT_READ : PROT_READ | PROT_WRITE,
                          MAP_SHARED,
                          MMap->File,
                          0);
  if (MapAddress == MAP_FAILED)
    return idx2_Error(mmap_err_code::MapViewFailed);
  MMap->Buf.Data = (byte*)MapAddress;
  MMap->Buf.Bytes = FileSize;
#endif
  return idx2_Error(mmap_err_code::NoError);
}


/* (Non-blocking) flush dirty pages */
error<mmap_err_code>
FlushFile(mmap_file* MMap, byte* Start, i64 Bytes)
{
#if defined(_WIN32)
  bool Result =
    Start ? FlushViewOfFile(Start, (size_t)Bytes) : FlushViewOfFile(MMap->Buf.Data, (size_t)Bytes);
  if (!Result)
    return idx2_Error(mmap_err_code::FlushFailed);
#elif defined(__CYGWIN__) || defined(__linux__) || defined(__APPLE__)
  int Result;
  if (Start)
  {
    idx2_Assert(MMap->Buf.Data <= Start && Start < MMap->Buf.Data + MMap->Buf.Bytes);
    idx2_Assert(Bytes <= (MMap->Buf.Data + MMap->Buf.Bytes) - Start);
    Result = Bytes ? msync(Start, Bytes, MS_ASYNC)
                   : msync(Start, MMap->Buf.Bytes - (Start - MMap->Buf.Data), MS_ASYNC);
  }
  else
  {
    idx2_Assert(Bytes <= MMap->Buf.Bytes);
    Result = Bytes ? msync(MMap->Buf.Data, Bytes, MS_ASYNC)
                   : msync(MMap->Buf.Data, MMap->Buf.Bytes, MS_ASYNC);
  }
  if (Result == -1)
    return idx2_Error(mmap_err_code::FlushFailed);
#endif
  return idx2_Error(mmap_err_code::NoError);
}


/*
(Blocking) flush file metadata and ensure file is physically written. Meant to be called at the very
end */
error<mmap_err_code>
SyncFile(mmap_file* MMap)
{
#if defined(_WIN32)
  if (!FlushFileBuffers(MMap->File))
    return idx2_Error(mmap_err_code::SyncFailed);
#elif defined(__CYGWIN__) || defined(__linux__) || defined(__APPLE__)
  if (msync(MMap->Buf.Data, MMap->Buf.Bytes, MS_SYNC) == -1)
    return idx2_Error(mmap_err_code::SyncFailed);
#endif
  return idx2_Error(mmap_err_code::NoError);
}


/* Unmap the file and close all handles */
error<mmap_err_code>
UnmapFile(mmap_file* MMap)
{
#if defined(_WIN32)
  if (!UnmapViewOfFile(MMap->Buf.Data))
  {
    CloseHandle(MMap->FileMapping);
    return idx2_Error(mmap_err_code::UnmapFailed);
  }
  if (!CloseHandle(MMap->FileMapping))
    return idx2_Error(mmap_err_code::UnmapFailed);
#elif defined(__CYGWIN__) || defined(__linux__) || defined(__APPLE__)
  if (munmap(MMap->Buf.Data, MMap->Buf.Bytes) == -1)
    return idx2_Error(mmap_err_code::UnmapFailed);
#endif
  MMap->Buf.Data = nullptr;
  MMap->Buf.Bytes = 0;
  return idx2_Error(mmap_err_code::NoError);
}


/* Close the file */
error<mmap_err_code>
CloseFile(mmap_file* MMap)
{
#if defined(_WIN32)
  if (!CloseHandle(MMap->File))
    return idx2_Error(mmap_err_code::FileCloseFailed);
#elif defined(__CYGWIN__) || defined(__linux__) || defined(__APPLE__)
  if (close(MMap->File) == -1)
    return idx2_Error(mmap_err_code::FileCloseFailed);
#endif
  MMap->Buf.Data = nullptr;
  MMap->Buf.Bytes = 0;
  return idx2_Error(mmap_err_code::NoError);
}


} // namespace idx2
