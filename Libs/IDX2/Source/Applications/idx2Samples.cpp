//#define idx2_Implementation
//#include "../idx2.hpp"
#include "../idx2.h"
#include "stdio.h"


idx2::error<idx2::idx2_err_code>
Decode1()
{
  idx2::params P;
  P.InputFile = "D:/Datasets/nasa/llc_2160_32/llc2160/u-face-2-depth-88-time-0-32.idx2";
  P.InDir = "D:/Datasets/nasa/llc_2160_32";
  //P.InputFile = "MIRANDA/VISCOSITY.idx2"; // name of data set and field
  //P.InDir = ".";                          // the directory containing the InputFile
  idx2::idx2_file Idx2;
  idx2_CleanUp(Dealloc(&Idx2)); // clean up Idx2 automatically in case of error
  P.DownsamplingFactor3 = idx2::v3i(2, 2, 0); // Downsample x by 2^1, y by 2^1, z by 2^1
  P.DecodeTolerance = 0.01;
  P.DecodeExtent = idx2::extent(idx2::v3i(1400, 0, 0), idx2::v3i(1, 6480, 1)); // uncomment if getting only a portion of the volume

  idx2_PropagateIfError(Init(&Idx2, P));

  idx2::grid OutGrid = idx2::GetOutputGrid(Idx2, P);

  idx2::buffer OutBuf;               // buffer to store the output
  idx2_CleanUp(DeallocBuf(&OutBuf)); // deallocate OutBuf automatically in case of error
  idx2::AllocBuf(&OutBuf, idx2::Prod<idx2::i64>(idx2::Dims(OutGrid)) * idx2::SizeOf(Idx2.DType));
  idx2_PropagateIfError(idx2::Decode(&Idx2, P, &OutBuf));

  // uncomment the following lines to write the output to a file
  // FILE* Fp = fopen("out.raw", "wb");
  // idx2_CleanUp(if (Fp) fclose(Fp));
  // fwrite(OutBuf.Data, OutBuf.Bytes, 1, Fp);

  return idx2_Error(idx2::idx2_err_code::NoError);
}


int
main()
{
  auto Ok = Decode1();
  if (!Ok)
  {
    fprintf(stderr, "%s\n", ToString(Ok));
    return 1;
  }

  return 0;
}
