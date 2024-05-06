#include "Format.h"


namespace idx2
{


printer::printer() = default;

printer::printer(char* BufIn, int SizeIn)
  : Buf(BufIn)
  , Size(SizeIn)
  , File(nullptr)
{
}

printer::printer(FILE* FileIn)
  : Buf(nullptr)
  , Size(0)
  , File(FileIn)
{
}


void
Reset(printer* Pr, char* Buf, int Size)
{
  Pr->Buf = Buf;
  Pr->Size = Size;
  Pr->File = nullptr;
}


void
Reset(printer* Pr, FILE* File)
{
  Pr->Buf = nullptr;
  Pr->Size = 0;
  Pr->File = File;
}


} // namespace idx2
