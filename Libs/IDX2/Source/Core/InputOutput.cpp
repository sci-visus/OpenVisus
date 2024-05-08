#include "InputOutput.h"
#include "Assert.h"
#include "Memory.h"
#include "ScopeGuard.h"


namespace idx2
{


error<>
ReadFile(cstr FileName, buffer* Buf)
{
  idx2_Assert((Buf->Data && Buf->Bytes) || (!Buf->Data && !Buf->Bytes));

  FILE* Fp = fopen(FileName, "rb");
  idx2_CleanUp(if (Fp) fclose(Fp));
  if (!Fp)
    return idx2_Error(err_code::FileOpenFailed, "%s", FileName);

  /* Determine the file size */
  if (idx2_FSeek(Fp, 0, SEEK_END))
    return idx2_Error(err_code::FileSeekFailed, "%s", FileName);
  i64 Size = 0;
  if ((Size = idx2_FTell(Fp)) == -1)
    return idx2_Error(err_code::FileTellFailed, "%s", FileName);
  if (idx2_FSeek(Fp, 0, SEEK_SET))
    return idx2_Error(err_code::FileSeekFailed, "%s", FileName);
  if (Buf->Bytes < Size)
    AllocBuf(Buf, Size);

  /* Read file contents */
  idx2_CleanUp(1, DeallocBuf(Buf));
  if (fread(Buf->Data, size_t(Size), 1, Fp) != 1)
    return idx2_Error(err_code::FileReadFailed, "%s", FileName);

  idx2_DismissCleanUp(1);
  return idx2_Error(err_code::NoError);
}


error<>
WriteBuffer(cstr FileName, const buffer& Buf)
{
  idx2_Assert(Buf.Data && Buf.Bytes);

  FILE* Fp = fopen(FileName, "wb");
  idx2_CleanUp(if (Fp) fclose(Fp));
  if (!Fp)
    return idx2_Error(err_code::FileCreateFailed, "%s", FileName);

  /* Read file contents */
  if (fwrite(Buf.Data, size_t(Buf.Bytes), 1, Fp) != 1)
    return idx2_Error(err_code::FileWriteFailed, "%s", FileName);
  return idx2_Error(err_code::NoError);
}


} // namespace idx2
