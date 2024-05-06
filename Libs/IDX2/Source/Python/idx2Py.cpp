#include "../idx2.h"
#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/tuple.h>
#include <nanobind/tensor.h>
#include <string>


namespace nb = nanobind;

using namespace nb::literals;


/* Return a 3D tensor storing an extent */
nb::tensor<nb::numpy, float, nb::shape<nb::any, nb::any, nb::any>>
DecodeExtent3f32(const std::string& InputFile,
                 const std::string& InputPath,
                 const std::tuple<int, int, int, int, int, int>& Extent,
                 const std::tuple<int, int, int>& DownsamplingFactor3,
                 double Accuracy) // 0 is "lossless"
{
  using namespace idx2;
  using namespace std;

  v3i From3(get<0>(Extent), get<1>(Extent), get<2>(Extent));
  v3i To3(get<3>(Extent), get<4>(Extent), get<5>(Extent));
  params P;
  P.InputFile = InputFile.c_str(); // name of data set and field
  P.InDir = InputPath.c_str();     // the directory containing the InputFile
  P.DownsamplingFactor3 = v3i(get<0>(DownsamplingFactor3), get<1>(DownsamplingFactor3), get<2>(DownsamplingFactor3));
  P.DecodeTolerance = Accuracy;
  P.DecodeExtent = idx2::extent(From3, To3);

  idx2_file Idx2;
  idx2_CleanUp(Dealloc(&Idx2)); // clean up Idx2 automatically in case of error
  auto InitOk = Init(&Idx2, P); // TODO: throw exception
  if (!InitOk)
    printf("ERROR: %s\n", ToString(InitOk));

  //printf("decode extent = " idx2_PrStrExt "\n", idx2_PrExt(P.DecodeExtent));

  grid OutGrid = idx2::GetOutputGrid(Idx2, P);

  buffer OutBuf; // buffer to store the output
  AllocBuf(&OutBuf, Prod<i64>(Dims(OutGrid)) * SizeOf(Idx2.DType)); // TODO: who is going to deallocate this buffer?
  idx2::Decode(&Idx2, P, &OutBuf); // TODO: throw exception

  v3i D3 = Dims(OutGrid);
  size_t Shape[3] = { (size_t)D3.Z, (size_t)D3.Y, (size_t)D3.X };
  nb::capsule Owner(OutBuf.Data, [](void* Data) noexcept { free(Data); });
  return nb::tensor<nb::numpy, float, nb::shape<nb::any, nb::any, nb::any>>(OutBuf.Data, 3, Shape, Owner);
}


/* Return a 3D tensor */
nb::tensor<nb::numpy, float, nb::shape<nb::any, nb::any, nb::any>>
Decode3f32(const std::string& InputFile,
           const std::string& InputPath,
           const std::tuple<int, int, int>& DownsamplingFactor3,
           double Accuracy) // 0 is "lossless"
{
  using namespace idx2;
  using namespace std;

  params P;
  P.InputFile = InputFile.c_str(); // name of data set and field
  P.InDir = InputPath.c_str();     // the directory containing the InputFile
  P.DownsamplingFactor3 = v3i(get<0>(DownsamplingFactor3), get<1>(DownsamplingFactor3), get<2>(DownsamplingFactor3));
  P.DecodeTolerance = Accuracy;

  idx2_file Idx2;
  idx2_CleanUp(Dealloc(&Idx2)); // clean up Idx2 automatically in case of error
  auto InitOk = Init(&Idx2, P); // TODO: throw exception
  if (!InitOk)
    printf("ERROR: %s\n", ToString(InitOk));

  //P.DecodeExtent = idx2::extent(Idx2.Dims3); // get the whole volume

  grid OutGrid = GetOutputGrid(Idx2, P);

  buffer OutBuf; // buffer to store the output
  AllocBuf(&OutBuf, Prod<i64>(Dims(OutGrid)) * SizeOf(Idx2.DType));
  idx2::Decode(&Idx2, P, &OutBuf); // TODO: throw exception

  v3i D3 = Dims(OutGrid);
  size_t Shape[3] = { (size_t)D3.Z, (size_t)D3.Y, (size_t)D3.X };
  nb::capsule Owner(OutBuf.Data, [](void* Data) noexcept { free(Data); });
  return nb::tensor<nb::numpy, float, nb::shape<nb::any, nb::any, nb::any>>(OutBuf.Data, 3, Shape, Owner);
}


/* Return a 3D tensor */
nb::tensor<nb::numpy, double, nb::shape<nb::any, nb::any, nb::any>>
Decode3f64(const std::string& InputFile,
           const std::string& InputPath,
           const std::tuple<int, int, int>& DownsamplingFactor3,
           double Accuracy) // 0 is "lossless"
{
  using namespace idx2;
  using namespace std;

  params P;
  P.InputFile = InputFile.c_str(); // name of data set and field
  P.InDir = InputPath.c_str();     // the directory containing the InputFile
  P.DownsamplingFactor3 = v3i(get<0>(DownsamplingFactor3), get<1>(DownsamplingFactor3), get<2>(DownsamplingFactor3));
  P.DecodeTolerance = Accuracy;

  idx2_file Idx2;
  idx2_CleanUp(Dealloc(&Idx2)); // clean up Idx2 automatically in case of error
  auto InitOk = Init(&Idx2, P); // TODO: throw exception
  if (!InitOk)
    printf("ERROR: %s\n", ToString(InitOk));

  //P.DecodeExtent = idx2::extent(Idx2.Dims3); // get the whole volume

  grid OutGrid = GetOutputGrid(Idx2, P);

  buffer OutBuf; // buffer to store the output
  // idx2_CleanUp(DeallocBuf(&OutBuf)); // deallocate OutBuf automatically in case of error
  AllocBuf(&OutBuf, Prod<i64>(Dims(OutGrid)) * SizeOf(Idx2.DType));

  idx2::Decode(&Idx2, P, &OutBuf); // TODO: throw exception

  v3i D3 = Dims(OutGrid);
  size_t Shape[3] = { (size_t)D3.Z, (size_t)D3.Y, (size_t)D3.X };
  nb::capsule Owner(OutBuf.Data, [](void* Data) noexcept { free(Data); });
  return nb::tensor<nb::numpy, double, nb::shape<nb::any, nb::any, nb::any>>(OutBuf.Data, 3, Shape, Owner);
}


/* Return a 2D tensor */
nb::tensor<nb::numpy, float, nb::shape<nb::any, nb::any>>
Decode2f32(const std::string& InputFile,
           const std::string& InputPath,
           const std::tuple<int, int>& DownsamplingFactor2,
           double Accuracy) // 0 is "lossless"
{
  using namespace idx2;
  using namespace std;

  params P;
  P.InputFile = InputFile.c_str(); // name of data set and field
  P.InDir = InputPath.c_str();     // the directory containing the InputFile
  P.DownsamplingFactor3 = v3i(get<0>(DownsamplingFactor2), get<1>(DownsamplingFactor2), 0);
  P.DecodeTolerance = Accuracy;

  idx2_file Idx2;
  idx2_CleanUp(Dealloc(&Idx2)); // clean up Idx2 automatically in case of error
  auto InitOk = Init(&Idx2, P); // TODO: throw exception
  if (!InitOk)
    printf("ERROR: %s\n", ToString(InitOk));

  //P.DecodeExtent = idx2::extent(Idx2.Dims3); // get the whole volume

  grid OutGrid = GetOutputGrid(Idx2, P);

  buffer OutBuf; // buffer to store the output
  // idx2_CleanUp(DeallocBuf(&OutBuf)); // deallocate OutBuf automatically in case of error
  AllocBuf(&OutBuf, Prod<i64>(Dims(OutGrid)) * SizeOf(Idx2.DType));
  idx2::Decode(&Idx2, P, &OutBuf); // TODO: throw exception

  v3i D3 = Dims(OutGrid);
  size_t Shape[2] = { (size_t)D3.Y, (size_t)D3.X };
  nb::capsule Owner(OutBuf.Data, [](void* Data) noexcept { free(Data); });
  return nb::tensor<nb::numpy, float, nb::shape<nb::any, nb::any>>(OutBuf.Data, 2, Shape, Owner);
}


/* Return a 2D tensor of double-precision floating-points */
nb::tensor<nb::numpy, double, nb::shape<nb::any, nb::any>>
Decode2f64(const std::string& InputFile,
           const std::string& InputPath,
           const std::tuple<int, int>& DownsamplingFactor2,
           double Accuracy) // 0 is "lossless"
{
  using namespace idx2;
  using namespace std;

  params P;
  P.InputFile = InputFile.c_str(); // name of data set and field
  P.InDir = InputPath.c_str();     // the directory containing the InputFile
  P.DownsamplingFactor3 = v3i(get<0>(DownsamplingFactor2), get<1>(DownsamplingFactor2), 0);
  P.DecodeTolerance = Accuracy;

  idx2_file Idx2;
  idx2_CleanUp(Dealloc(&Idx2)); // clean up Idx2 automatically in case of error
  auto InitOk = Init(&Idx2, P); // TODO: throw exception
  if (!InitOk)
    printf("ERROR: %s\n", ToString(InitOk));

  //P.DecodeExtent = idx2::extent(Idx2.Dims3); // get the whole volume

  grid OutGrid = GetOutputGrid(Idx2, P);

  buffer OutBuf; // buffer to store the output
  // idx2_CleanUp(DeallocBuf(&OutBuf)); // deallocate OutBuf automatically in case of error
  AllocBuf(&OutBuf, Prod<i64>(Dims(OutGrid)) * SizeOf(Idx2.DType));
  idx2::Decode(&Idx2, P, &OutBuf); // TODO: throw exception

  v3i D3 = Dims(OutGrid);
  size_t Shape[2] = { (size_t)D3.Y, (size_t)D3.X };
  nb::capsule Owner(OutBuf.Data, [](void* Data) noexcept { free(Data); });
  return nb::tensor<nb::numpy, double, nb::shape<nb::any, nb::any>>(OutBuf.Data, 2, Shape, Owner);
}


NB_MODULE(idx2Py, M)
{
  M.def("DecodeExtent3f32", DecodeExtent3f32, nb::call_guard<nb::gil_scoped_release>());
  M.def("Decode3f32", Decode3f32, nb::call_guard<nb::gil_scoped_release>());
  M.def("Decode2f32", Decode2f32, nb::call_guard<nb::gil_scoped_release>());
  M.def("Decode3f64", Decode3f64, nb::call_guard<nb::gil_scoped_release>());
  M.def("Decode2f64", Decode2f64, nb::call_guard<nb::gil_scoped_release>());
}
