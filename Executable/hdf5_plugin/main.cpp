#include <iostream>
#include <hdf5.h>
#include <vector>

////////////////////////////////////////////////////////////////////////////
void __Check(bool cond, std::string file,int line)
{
  if (bool err = cond ? false : true) {

      std::cout << "Error at line " << line << " in file " << file << std::endl; \
      H5Eprint1(stdout); \
      throw "internal error"; \
  } 
}

#define CHECK(cond) __Check(cond,__FILE__,__LINE__)


////////////////////////////////////////////////////////////////////////////
void Test1(std::string filename)
{
  const int width = 6;
  const int height = 5;

  // CREATE a file
  hid_t file = H5Fcreate(filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

  // CREATE group
  hid_t group = H5Gcreate2(file, "MyGroup", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

  // CREATE dataset with datatype and dataspace
  std::string dataset_name = "IntArray";

  if (true)
  {
    // CREATE dataspace (i.e. dataset dimensions)
    hsize_t shape[] = { height , width };
    hid_t dataspace = H5Screate_simple(2, shape, nullptr);

    //CREATE datatype: INT little endian
    hid_t datatype = H5Tcopy(H5T_NATIVE_INT);

    //set little endian
    CHECK(H5Tset_order(datatype, H5T_ORDER_LE) == 0);

    hid_t dataset = H5Dcreate2(group, dataset_name.c_str(), datatype, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    H5Dclose(dataset);
    H5Tclose(datatype);
    H5Sclose(dataspace);
  }

  //        ******
  std::vector<int> BUFFER = {
    11, 12, 13, 14, 15, 16,
    21, 22, 23, 24, 25, 26,   //*
    31, 32, 33, 34, 35, 36,   //*
    41, 42, 43, 44, 45, 46,   //*
    51, 52, 53, 54, 55, 56,   //*
  };


  //Write ALL to dataset
  if (true)
  {
    hid_t dataset = H5Dopen(group, dataset_name.c_str(), H5P_DEFAULT);
    CHECK(dataset > 0);

    auto dataspace = H5Dget_space(dataset);
    auto rank = H5Sget_simple_extent_ndims(dataspace);
    CHECK(rank == 2);
    hsize_t dims_out[2];
    H5Sget_simple_extent_dims(dataspace, dims_out, NULL);
    printf("\nRank: %d\nDimensions: %lu x %lu \n", rank, (unsigned long)(dims_out[0]), (unsigned long)(dims_out[1]));

    auto buffer = BUFFER;
    CHECK(H5Dwrite(dataset, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &buffer[0]) == 0);
    H5Dclose(dataset);
  }

  //Read ALL to Dataset
  if (true)
  {
    hid_t dataset = H5Dopen(group, dataset_name.c_str(), H5P_DEFAULT);
    std::vector<int32_t> buffer(width * height, 0);
    CHECK(H5Dread(dataset, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &buffer[0]) == 0);
    H5Dclose(dataset);
    CHECK(buffer == BUFFER);
  }

  //Write SEL to dataset
  // see https://support.hdfgroup.org/HDF5/Tutor/selectsimple.html
  {
    hid_t dataset = H5Dopen(group, dataset_name.c_str(), H5P_DEFAULT);

    //dataset space (OUTPUT)
    const hsize_t dataset_offset[] = { 1,2 };
    const hsize_t dataset_count[] = { 1,1 }; //count array specifies the number of blocks.
    const size_t  dataset_block[] = { 4,2 }; //size of the block
    auto dataspace = H5Dget_space(dataset);
    CHECK(H5Sselect_hyperslab(dataspace, H5S_SELECT_SET, dataset_offset, NULL, dataset_count, dataset_block) == 0);

    //memspace
    const hsize_t memory_dims[2] = { 4,2 };
    auto memspace = H5Screate_simple(2, memory_dims, NULL);
    CHECK(H5Sselect_all(memspace) == 0);

    std::vector<int> buffer = {
      63,64,
      73,74,
      83,84,
      93,94
    };
    CHECK(H5Dwrite(dataset, H5T_NATIVE_INT, memspace, dataspace, H5P_DEFAULT, &buffer[0])==0);

    H5Dclose(dataset);
    H5Sclose(dataspace);
  }

  //Read ALL to Dataset
  if (true)
  {
    hid_t dataset = H5Dopen(group, dataset_name.c_str(), H5P_DEFAULT);
    std::vector<int32_t> buffer(width * height, 0);
    CHECK(H5Dread(dataset, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &buffer[0]) == 0);
    H5Dclose(dataset);

    for (auto val : buffer)
      std::cout << val << " ";
    std::cout << std::endl;
    //        ******            
    CHECK(buffer == std::vector<int32_t>({
      11, 12, 13, 14, 15, 16,
      21, 22, 63, 64, 25, 26,   //*
      31, 32, 73, 74, 35, 36,   //*
      41, 42, 83, 84, 45, 46,   //*
      51, 52, 93, 94, 55, 56,   //*
      }));
  }

  //read SEL to dataset
  {
    hid_t dataset = H5Dopen(group, dataset_name.c_str(), H5P_DEFAULT);

    //dataset space (input)
    const hsize_t offset[] = { 1,2 };
    const hsize_t count[] = { 1,1 };
    const hsize_t block[] = { 4,2 };
    auto dataspace = H5Dget_space(dataset);
    CHECK(H5Sselect_hyperslab(dataspace, H5S_SELECT_SET, offset, NULL, count, block) == 0);

    //memory space (output)
    const hsize_t memory_dims[2] = { 4,2 };
    auto memspace = H5Screate_simple(2, memory_dims, NULL);
    CHECK(H5Sselect_all(memspace) == 0);

    std::vector<int32_t> buffer(memory_dims[0] * memory_dims[1]);
    H5Dread(dataset, H5T_NATIVE_INT, memspace, dataspace, H5P_DEFAULT, &buffer[0]);

    CHECK(buffer == std::vector<int32_t>({
      63,64,
      73,74,
      83,84,
      93,94
      }));

    H5Dclose(dataset);
    H5Sclose(dataspace);
    H5Sclose(memspace);
  }

  H5Gclose(group);
  H5Fclose(file);
}

/////////////////////////////////////////////////////////////////////////////////////////
void Test2(std::string filename)
{
  //see https://github.com/mvanmoer/HDF5_examples/tree/master/orthonormalgrids/scalar

  // HDF5 convention is fastest is last, this is {zdim, ydim, xdim}.
  hsize_t W = 1024;
  hsize_t H = 768;
  hsize_t D = 20;
  hsize_t dims[3] = { D , H,  W };

  // arbitrary scalar data for writing.
  std::vector<float> scalars;
  for (int Z = 0; Z < D; Z++)
  {
    for (int Y = 0; Y < H; Y++)
    {
      for (int X = 0; X < W; X++)
      {
        auto x = 2 * (X / (double)(W - 1)) - 1;
        auto y = 2 * (Y / (double)(H - 1)) - 1;
        auto z = 2 * (Z / (double)(D - 1)) - 1;
        scalars.push_back((float)sqrt(x*x+y*y+z*z));
      }
    }
  }

  CHECK(scalars.size() == W * H * D);

  auto m = scalars[0], M = scalars[0];
  for (auto it : scalars)
  {
    m = std::min(m, it);
    M = std::max(M, it);
  }
  std::cout << "min=" << m << " max=" << M <<std::endl;

  std::cout << "Calling H5Fcreate..." << std::endl;
  hid_t file = H5Fcreate(filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

  std::cout << "Calling H5Screate..." << std::endl;
  hid_t dataspace = H5Screate_simple(3, dims, NULL);

  std::cout << "Calling H5Dcreate2..." << std::endl;
  hid_t datatype = H5Tcopy(H5T_NATIVE_FLOAT);
  hid_t dataset = H5Dcreate2(file, "scalars", datatype, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

  std::cout << "Calling H5Dwrite..." << std::endl;
  CHECK(H5Dwrite(dataset, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &scalars[0])==0);

  H5Dclose(dataset);
  H5Tclose(datatype);
  H5Sclose(dataspace);
  H5Fclose(file);
}


////////////////////////////////////////////////////////////////////
int main()
{
  // To debug in windows see ReadMe.md

  Test1("testfile1.h5");
  Test2("testfile2.h5");

  std::cout<<"all done"<<std::endl;

  //problem here, not getting the term for the VOL connector
  //or COULD IT BE a memory leak/bug? like memory corrupted... need to check
  return 0;
}
