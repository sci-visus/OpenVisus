#include "FileSystem.h"
#include "Algorithm.h"
#include "Assert.h"
#include "Format.h"
#include <string.h>


#if defined(_WIN32)
#include "dirent_win.h"
#include <Windows.h>
#include <direct.h>
#include <io.h>
#include <sys/stat.h>
#include <sys/types.h>
#define GetCurrentDir _getcwd
#define MkDir(Dir) _mkdir(Dir)
#define Access(Dir) _access(Dir, 0)
#define Stat(Path, S) _stat64(Path, S)
#define stat _stat64
#elif defined(__CYGWIN__) || defined(__linux__) || defined(__APPLE__)
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#define GetCurrentDir getcwd
#define MkDir(Dir) mkdir(Dir, 0733)
#define Access(Dir) access(Dir, F_OK)
#define Stat(Path, S) stat(Path, S)
//#define stat struct stat
#endif


namespace idx2
{


path::path() = default;


path::path(const stref& Str)
{
  Init(this, Str);
}


bool
path::operator==(const path& Other) const
{
  if (this->NParts != Other.NParts)
    return false;
  for (int I = 0; I < this->NParts; ++I)
  {
    if (this->Parts[I] != Other.Parts[I])
      return false;
  }
  return true;
}


void
Init(path* Path, const stref& Str)
{
  Path->Parts[0] = Str;
  Path->NParts = 1;
}


void
Append(path* Path, const stref& Part)
{
  idx2_Assert(Path->NParts < Path->NPartsMax, "too many path parts");
  Path->Parts[Path->NParts++] = Part;
}


stref
GetFileName(const stref& Path)
{
  idx2_Assert(!Contains(Path, '\\'));
  cstr LastSlash = FindLast(RevBegin(Path), RevEnd(Path), '/');
  if (LastSlash != RevEnd(Path))
    return SubString(
      Path, int(LastSlash - Begin(Path) + 1), Path.Size - int(LastSlash - Begin(Path)));

  return Path;
}


stref
GetParentPath(const stref& Path)
{
  idx2_Assert(!Contains(Path, '\\'));
  cstr LastSlash = FindLast(RevBegin(Path), RevEnd(Path), '/');
  if (LastSlash != RevEnd(Path))
    return SubString(Path, 0, int(LastSlash - Begin(Path)));

  return Path;
}


cstr
ToString(const path& Path)
{
  printer Pr(ScratchBuf, sizeof(ScratchBuf));
  for (int I = 0; I < Path.NParts; ++I)
  {
    idx2_Print(&Pr, "%.*s", Path.Parts[I].Size, Path.Parts[I].Ptr);
    if (I + 1 < Path.NParts)
      idx2_Print(&Pr, "/");
  }
  return ScratchBuf;
}


bool
IsRelative(const stref& Path)
{
  stref& PathR = const_cast<stref&>(Path);
  if (PathR.Size > 0 && PathR[0] == '/') // e.g. /usr/local
    return false;
  if (PathR.Size > 2 && PathR[1] == ':' && PathR[2] == '/') // e.g. C:/Users
    return false;

  return true;
}


bool
CreateFullDir(const stref& Path)
{
  cstr PathCopy = ToString(Path);
  int Error = 0;
  str P = (str)PathCopy;

  for // loop through the components (between two '/')
  (
    P = (str)strchr(PathCopy, '/');
    P;
    P = (str)strchr(P + 1, '/')
  )
  {
    *P = '\0';
    Error = MkDir(PathCopy);
    *P = '/';
  }

  Error = MkDir(PathCopy);

  return (Error == 0);
}


bool
DirExists(const stref& Path)
{
  cstr PathCopy = ToString(Path);

  return Access(PathCopy) == 0;
}


void
RemoveDir(cstr Path)
{
  if (!DirExists(Path))
    return;

  struct dirent* Entry = nullptr;
  DIR* Dir = nullptr;
  Dir = opendir(Path);
  char AbsPath[257] = { 0 };

  while (Entry = readdir(Dir))
  {
    if (*(Entry->d_name) == '.')
      continue;

    DIR* SubDir = nullptr;
    FILE* File = nullptr;
    sprintf(AbsPath, "%s/%s", Path, Entry->d_name);
    if (SubDir = opendir(AbsPath))
    {
      closedir(SubDir);
      RemoveDir(AbsPath);
    }
    else if (File = fopen(AbsPath, "r"))
    {
      fclose(File);
      remove(AbsPath);
    }
  }

  remove(Path);
}


stref
GetExtension(const stref& Path)
{
  cstr LastDot = FindLast(RevBegin(Path), RevEnd(Path), '.');
  if (LastDot == RevEnd(Path))
    return stref();

  return SubString(Path, int(LastDot + 1 - Begin(Path)), int(End(Path) - 1 - LastDot));
}


i64
GetFileSize(const stref& Path)
{
  idx2_RAII(char, C = Path.Ptr[Path.Size], Path.Ptr[Path.Size] = '\0', Path.Ptr[Path.Size] = C);
  struct ::stat S;
  if (0 != ::Stat(Path.Ptr, &S))
    return -1;

  return (i64)S.st_size;
}


} // namespace idx2


#undef GetCurrentDir
#undef MkDir
#undef Access
#undef stat
#undef Stat
