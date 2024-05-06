#pragma once

#include "Array.h"
#include "Common.h"


#define idx2_Print(PrinterPtr, Format, ...)
#define idx2_PrintScratch(Format, ...)
#define idx2_PrintScratchN(N, Format, ...) // Print at most N characters


namespace idx2
{


/* Print formatted strings into a buffer */
struct printer
{
  char* Buf = nullptr;
  int Size = 0;
  FILE* File = nullptr; // either File == nullptr or Buf == nullptr
  printer();
  printer(char* BufIn, int SizeIn);
  printer(FILE* FileIn);
};

void Reset(printer* Pr, char* Buf, int Size);

void Reset(printer* Pr, FILE* File);

template <typename t> void
Print(printer* Pr, const array<t>& Array)
{
  idx2_Print(Pr, "[");
  for (i64 I = 0; I < Size(Array); ++I)
    idx2_Print(Pr, "%f, ", Array[I]); // TODO
  idx2_Print(Pr, "]");
}


} // namespace idx2



#undef idx2_Print
#define idx2_Print(PrinterPtr, Format, ...)                                                        \
  {                                                                                                \
    if ((PrinterPtr)->Buf && !(PrinterPtr)->File)                                                  \
    {                                                                                              \
      if ((PrinterPtr)->Size <= 1)                                                                 \
        assert(false && "buffer too small"); /* TODO: always abort */                              \
      int Written =                                                                                \
        snprintf((PrinterPtr)->Buf, size_t((PrinterPtr)->Size), Format, ##__VA_ARGS__);            \
      (PrinterPtr)->Buf += Written;                                                                \
      if (Written < (PrinterPtr)->Size)                                                            \
        (PrinterPtr)->Size -= Written;                                                             \
      else                                                                                         \
        assert(false && "buffer overflow?");                                                       \
    }                                                                                              \
    else if (!(PrinterPtr)->Buf && (PrinterPtr)->File)                                             \
    {                                                                                              \
      fprintf((PrinterPtr)->File, Format, ##__VA_ARGS__);                                          \
    }                                                                                              \
    else                                                                                           \
    {                                                                                              \
      assert(false && "unavailable or ambiguous printer destination");                             \
    }                                                                                              \
  }


#undef idx2_PrintScratch
#define idx2_PrintScratch(Format, ...)                                                             \
  (snprintf(ScratchBuf, sizeof(ScratchBuf), Format, ##__VA_ARGS__), ScratchBuf)


#undef idx2_PrintScratchN
#define idx2_PrintScratchN(N, Format, ...)                                                         \
  (snprintf(ScratchBuf, N + 1, Format, ##__VA_ARGS__), ScratchBuf)
