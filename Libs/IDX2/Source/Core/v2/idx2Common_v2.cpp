#include "idx2Common_v2.h"
#include <ctype.h>


namespace idx2
{


bool
OptVal(i32 NArgs, cstr* Args, cstr Opt, array<dimension_info>* Dimensions)
{
  for (i32 I = 0; I < NArgs; ++I)
  {
    if (strncmp(Args[I], Opt, 32) == 0)
    {
      i32 J = I + 1;
      while (true)
      {
        dimension_info Dim;
        if (J >= NArgs || !isalpha(Args[J][0]) || !islower(Args[J][0]))
          return J > I + 1;
        Dim.ShortName = Args[J++][0];
        if (J >= NArgs || !ToInt(Args[J++], &Dim.Limit))
          return false;
        PushBack(Dimensions, Dim);
      }
      return true;
    }
  }
  return false;
}


idx2_file_v2::idx2_file_v2()
{
  memset(CharToIntMap, -1, sizeof(CharToIntMap));
}


void
Dealloc(idx2_file_v2* Idx2)
{
  // TODO_v2
  Dealloc(&Idx2->Dimensions);
}


static error<idx2_err_code>
CheckDimensions(idx2_file_v2* Idx2)
{
  idx2_ForEach (Dim, Idx2->Dimensions)
  {
    if (Size(Dim->Names) == 0 && Dim->Limit == 0)
      return idx2_Error(idx2_err_code::SizeZero, "Dimension %c is zero\n", Dim->ShortName);
    if (Size(Dim->Names) > 0 && Dim->Limit > 0)
      return idx2_Error(idx2_err_code::DimensionsRepeated, "Dimension %c is both numerical and categorical\n", Dim->ShortName);
    if (Size(Dim->Names) == 1 || Dim->Limit == 1)
      return idx2_Error(idx2_err_code::DimensionTooSmall, "Dimension %c is too small (1), consider removing it\n", Dim->ShortName);
  }

  return idx2_Error(idx2_err_code::NoError);
}

// TODO: check that the number of dimensions per level is at most the size of the brick
// TODO: check that brick size < chunk size < file size
// TODO: check that the indexing template matches the dimensions
// TODO: check that we have at most 6 dimensions

static error<idx2_err_code>
ParseIndexingTemplate(idx2_file_v2* Idx2)
{

  // TODO: check that each level has at most 3 dimensions (supported by compression)

  bool ParsingPostfix = true;
  array<i8> Part;
  for (auto I = Size(Idx2->IdxTemplate.Full) - 1; I >= 0; --I)
  {
    if (Idx2->IdxTemplate.Full[I] == ':') // push a new level
    {
      if (!ParsingPostfix)
        return idx2_Error(idx2_err_code::SyntaxError, ": cannot occur after | in the indexing template\n");

      PushBack(&Idx2->IdxTemplate.Suffix, Part);
      Part = array<i8>(); // reset
    }
    else if (Idx2->IdxTemplate.Full[I] == '|') // push a new level
    {
      if (!ParsingPostfix)
        return idx2_Error(idx2_err_code::SyntaxError, "Only one | allowed in the indexing template\n");

      PushBack(&Idx2->IdxTemplate.Suffix, Part);
      Part = array<i8>(); // reset
      ParsingPostfix = false; // switch to parsing the prefix
    }
    else if (Idx2->IdxTemplate.Full[I] != '|')
    {
      char C = Idx2->IdxTemplate.Full[I];
      if (!isalpha(C) || !islower(C))
        return idx2_Error(idx2_err_code::SyntaxError, "Unsupported character (%c) in the indexing template\n", C);
      //if (ParsingPostfix && C == 'f')
      //  return idx2_Error(idx2_err_code::SyntaxError, "Dimension f (fields) cannot appear in the postfix\n", C);

      i8& D = Idx2->CharToIntMap[C - 'a'];
      /* map this dimension to an int if it is not mapped */
      if (D == -1)
      {
        for (auto J = 0; J < Size(Idx2->Dimensions); ++J)
        {
          if (Idx2->IdxTemplate.Full[I] == Idx2->Dimensions[J].ShortName)
          {
            D = J;
            break;
          }
        }
      }
      /* check for error */
      if (D == -1)
        return idx2_Error(idx2_err_code::DimensionsTooMany,
                          "The indexing template contains an unknown dimension: %c\n", C);

      /* push the current dimension */
      if (ParsingPostfix)
        PushBack(&Part, D);
      else
        PushBack(&Idx2->IdxTemplate.Prefix, D);
    }
  }

  /* add the last part of the suffix if necessary */
  if (Size(Part) > 0) // left-over part at the far left
  {
    PushBack(&Idx2->IdxTemplate.Suffix, Part);
    if (Size(Idx2->IdxTemplate.Prefix) > 0)
      return idx2_Error(idx2_err_code::SyntaxError, "Invalid indexing template syntax\n");
  }

  /* check for unused dimensions */
  for (auto I = 0; I < Size(Idx2->Dimensions); ++I)
  {
    char C = Idx2->Dimensions[I].ShortName;
    if (Idx2->CharToIntMap[C - 'a'] == -1)
      return idx2_Error(idx2_err_code::DimensionsTooMany, "Dimension %c does not appear in the indexing template\n", C);
  }

  /* check for repeated dimensions on a level */
  for (auto L = 0; L < Size(Idx2->IdxTemplate.Suffix); ++L)
  {
    auto S = Size(Idx2->IdxTemplate.Suffix[L]);
    if (S > 3)
      return idx2_Error(idx2_err_code::DimensionsTooMany, "More than three dimensions in level %d\n", L);
    if (S == 0)
      return idx2_Error(idx2_err_code::SyntaxError, ": or | needs to be followed by a dimension in the indexing template\n");
    const auto& Suffix = Idx2->IdxTemplate.Suffix[L];
    if (S == 2 && (Suffix[0] == Suffix[1]))
      return idx2_Error(idx2_err_code::DimensionsRepeated, "Repeated dimensions on level %d\n", L);
      //printf("Repeated dimensions on level %d\n", L);
    if (S == 3 && (Suffix[0] == Suffix[1] || Suffix[0] == Suffix[2] || Suffix[1] == Suffix[2]))
      return idx2_Error(idx2_err_code::DimensionsRepeated, "Repeated dimensions on level %d\n", L);
      //printf("Repeated dimensions on level %d\n", L);
  }

  /* check that the indexing template agrees with the dimensions */
  nd_size Dims(1);
  for (auto I = 0; I < Size(Idx2->IdxTemplate.Full); ++I)
  {
    char C = Idx2->IdxTemplate.Full[I];
    if (!isalpha(C))
      continue;
    i8 D = Idx2->CharToIntMap[C - 'a'];
    Dims[D] *= 2;
  }
  for (auto I = 0; I < Size(Idx2->Dimensions); ++I)
  {
    const dimension_info& Dim = Idx2->Dimensions[I];
    i32 D = Size(Dim);
    D = (i32)NextPow2(D);
    if (D > Dims[I])
      return idx2_Error(idx2_err_code::DimensionMismatched,
                        "Dimension %c needs to appear %d more times in the indexing template\n",
                        Dim.ShortName,
                        Log2Floor(D) - Log2Floor(Dims[I]));
    if (D < Dims[I])
      return idx2_Error(idx2_err_code::DimensionMismatched,
                        "Dimension %c needs to appear %d fewer times in the indexing template\n",
                        Dim.ShortName,
                        Log2Floor(Dims[I]) - Log2Floor(D));
  }


  return idx2_Error(idx2_err_code::NoError);
}


error<idx2_err_code>
Finalize(idx2_file_v2* Idx2)
{
  idx2_PropagateIfError(CheckDimensions(Idx2));
  idx2_PropagateIfError(ParseIndexingTemplate(Idx2));
  /* print the parsed indexing template */

  return idx2_Error(idx2_err_code::NoError);
}


} // namespace idx2

