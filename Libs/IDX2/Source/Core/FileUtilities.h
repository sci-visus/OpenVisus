#pragma once

#if defined(__APPLE__)
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


inline int // TODO: move to file_utils.cpp
fallocate(int fd, int mode, off_t offset, off_t len)
{
  (void)mode; // TODO: what to do with mode?
  // int fd = PR_FileDesc2NativeHandle(aFD);
  fstore_t store = { F_ALLOCATECONTIG, F_PEOFPOSMODE, offset, len, 0 };
  // Try to get a continous chunk of disk space
  int ret = fcntl(fd, F_PREALLOCATE, &store);
  if (-1 == ret)
  {
    // OK, perhaps we are too fragmented, allocate non-continuous
    store.fst_flags = F_ALLOCATEALL;
    ret = fcntl(fd, F_PREALLOCATE, &store);
    if (-1 == ret)
      return -1;
  }
  if (0 == ftruncate(fd, len))
    return 0;
  return -1;
}
#endif
