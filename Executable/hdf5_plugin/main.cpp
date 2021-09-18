#include <iostream>
#include "hdf5.h"

#define CHECK(cond) \
	{ \
		if (bool err = cond?false:true) { \
			std::cout<<"Error at line "<<__LINE__<<" in file "<<__FILE__<<std::endl; \
			H5Eprint1 (stdout); \
      throw "internal error";\
		} \
	}\
/*--*/


////////////////////////////////////////////////////////////////////
int main()
{
  const int NX = 5;
  const int NY = 6;
  

  // CREATE a file
  std::cout << "H5Fcreate..." << std::endl;
  hid_t file = H5Fcreate("testfile.h5", H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

  // CREATE group
  std::cout << "H5Gcreate2..." << std::endl;
  hid_t group = H5Gcreate2(file, "MyGroup", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

  // CREATE dataspace (i.e. dataset dimensions)
  std::cout << "H5Screate_simple..." << std::endl;
  hsize_t dimsf[2] = { NX , NY };
  hid_t dataspace = H5Screate_simple(2, dimsf, nullptr);

  //CREATE datatype: INT little endian
  std::cout << "H5Tcopy..." << std::endl;
  hid_t datatype = H5Tcopy(H5T_NATIVE_INT);

  //set little endian
  std::cout << "H5Tset_order..." << std::endl;
  CHECK(H5Tset_order(datatype, H5T_ORDER_LE)==0);

  // CREATE dataset with datatype and dataspace
  std::cout << "H5Dcreate2..." << std::endl;
  hid_t dataset = H5Dcreate2(group, "IntArray", datatype, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

  //Write to dataset
  if (true)
  {
    std::cout << "H5Dwrite..." << std::endl;

    int buffer[NX][NY];
    for (int j = 0; j < NX; j++)
    {
      for (int i = 0; i < NY; i++)
        buffer[j][i] = i + j;
    }
    /*
     * 0 1 2 3 4 5
     * 1 2 3 4 5 6
     * 2 3 4 5 6 7
     * 3 4 5 6 7 8
     * 4 5 6 7 8 9
     */

    CHECK(H5Dwrite(dataset, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, buffer) == 0);
  }


  std::cout << "H5Sclose..." << std::endl;
  H5Sclose(dataspace);
  std::cout << "H5Tclose..." << std::endl;
  H5Tclose(datatype);
  std::cout << "H5Dclose..." << std::endl;
  H5Dclose(dataset);
  std::cout << "H5Gclose..." << std::endl;
  H5Gclose(group);
  std::cout << "H5Fclose..." << std::endl;
  H5Fclose(file);

  return 0;
}
