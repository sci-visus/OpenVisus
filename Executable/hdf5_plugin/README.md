# OpenVisus HDF5 plugin

Generic links:

-   [HDF5Intro.pdf (hdfgroup.org)](https://support.hdfgroup.org/HDF5/Tutor/HDF5Intro.pdf)
-   [Introduction to HDF5 (hdfgroup.org)](https://support.hdfgroup.org/HDF5/doc/H5.intro.html)
-   [Tutorial] <https://support.hdfgroup.org/HDF5/Tutor/>
-   [Learning HDF5 (hdfgroup.org)](https://portal.hdfgroup.org/display/HDF5/Learning+HDF5)

VOL connector links and documentation

-   [HDF5 VOL Connector Authors Guide](https://portal.hdfgroup.org/display/HDF5/HDF5+VOL+Connector+Authors+Guide?preview=/53610813/59903039/vol_connector_author_guide.pdf)
-   [HDF5 VOL User's Guide](https://portal.hdfgroup.org/display/HDF5/HDF5+VOL+User%27s+Guide?preview=/53610801/59903036/vol_user_guide.pdf)
-   [Pass-Through Connector ](http://github.com)
-   [Log-based VOL ](https://github.com/DataLib-ECP/vol-log-based/tree/b13778efd9e0c79135a9d7352104985408078d45)
-   [NETCDF Connector ](https://github.com/DataLib-ECP/ncmpi_vol/blob/master/src/ncmpi_vol_dataset.cpp)
-   [Log VOL ](https://github.com/DataLib-ECP/vol-log-based/tree/b13778efd9e0c79135a9d7352104985408078d45)
-   [DAOS VOL ](http://github.com)
-   [NCMPI VOL](http://github.com)
-   https://github.com/hpc-io/vol-external-passthrough
-   https://github.com/DataLib-ECP/vol-log-based/blob/b13778efd9e0c79135a9d7352104985408078d45/src/H5VL_log_obj.cpp 



The OpenVisus/HDF5 plugin replaces HDF5 Dataset with IdxDataset and 
leave all other objects unmodified (attributes, file, groups, request, etc.). 

The HDF5 hierarchy should continue to work as usual, apart from that datasets are stored in 
our IDX file format

**TODO** :

-   [ ] Set the dataset to be chunked for better parallelism (ref `H5Pset_layout` )
-   [ ] Supports only read and write at full resolution. I don't think it will be difficult to add other resolutions, maybe using some tricks like using HDF5 ad-hoc links to the same data
-   [ ] Support complex memory space (ref HDF5 memory dataset)
-   [ ] support for array-like datatype (example uint8[3] could be an HDF5 array datatype)
-   [ ] Analyze the constraints for 2D, 2D multi-channel, 3D, 3d multichannel and how they map to HDF5. Right now I am supposing the datasets are only 2D with atomic dtype, or 3D with one or multiple channels
-   [ ] multi hyperslab selection. Now assuming��`H5Sget_regular_hyperslab`
-   [ ] Tested only in Windows, need to check OSX and Linux
-   [ ] on windows, I have a crash at exit

Extend tests:

- [x] HDF5 C Api 
- [x] python h5py library
- [x] jupyter h5py library
- [ ] Error: hdf5view (HDF5_PLUGIN_PATH seems to be ignored)
- [ ] Paraview. Not tested yet (




# Install Paraview

Download Windows binaries from [ParaView-5.10.0-Windows-Python3.9-msvc2017-AMD64.exe](https://www.paraview.org/paraview-downloads/download.php?submit=Download&version=v5.10&type=binary&os=Windows&downloadFile=ParaView-5.10.0-Windows-Python3.9-msvc2017-AMD64.exe)

Looking at the source code of VTK it seems they are using *HDF5 1.12.1* 
(see [CMakeLists.txt](https://gitlab.kitware.com/vtk/vtk/-/blob/master/ThirdParty/hdf5/CMakeLists.txt>))

so below I am using a compatible version

# Build HDF5 on Windows

Clone the GitHub repo and switch to the latest release tag (NOTE: **using a version compatible with Paraview**):

```shell
git clone https://github.com/HDFGroup/hdf5.git
cd hdf5
git checkout tags/hdf5-1_12_1
```

Open the `config/cmake/ConfigureChecks.cmake` and apply this patch:

```shell
git diff
diff --git a/config/cmake/ConfigureChecks.cmake b/config/cmake/ConfigureChecks.cmake
index 263cedf90c..3ddb5703c9 100644
--- a/config/cmake/ConfigureChecks.cmake
+++ b/config/cmake/ConfigureChecks.cmake
@@ -308,8 +308,10 @@ set (PROG_SRC
      "
 )

-C_RUN ("maximum decimal precision for C" ${PROG_SRC} PROG_RES)
-file (READ "${RUN_OUTPUT_PATH_DEFAULT}/pac_Cconftest.out" PROG_OUTPUT4)
+# scrgiorgio PROBLEM here with FILE(WRITE...) so I combiled by hand and took the result
+#C_RUN ("maximum decimal precision for C" ${PROG_SRC} PROG_RES)
+#file (READ "${RUN_OUTPUT_PATH_DEFAULT}/pac_Cconftest.out" PROG_OUTPUT4)
+set(PROG_OUTPUT4 "15;0;")
 message (STATUS "Testing maximum decimal precision for C - ${PROG_OUTPUT4}")

 # dnl The output from the above program will be:
```

In order to get the value of `PROG_OUTPUT4` I created and run a file `testCCompiler1.c` with the content of `PROG_SRC` just a few lines above the `C_RUN` command.

Run cmake and Configure/Generate/Configure/Generate as usual (**change values as needed)**:

-   Where the source code: `D:/hdf5`
-   Where to build the binaries `D:/hdf5/build`
-   Compiler: `Visual Studio 17 2022`
-   Platform: `x64`
-   Change the `CMAKE_INSTALL_PREFIX` to `d:/hdf5/install`

Push the CMake `Open Project` button, from `Project/Configurmation manager` choose the `RelWithDebInfo` configuration.

Right-click on `ALL_BUILD` and click on `Build`

Right-click on `INSTALL` project and click on `Build`


# Build OpenVisus HD5 


Clone the OpenVisus repository as usual, and:

- set the build directory to `build_paraview`_
- enable the `VISUS_HDF5` flag.

**Disable all VISUS_ checkbox** apart from `VISUS_HDF5` which must be enabled.

NOTE: I am disabling Python and Gui to avoid conflicts with Paraview Python and Qt._

Set (**change as needed)** :

-   `HDF5_DIR=D:/hdf5/install/share/cmake` 

Open the OpenVisus project.

Set:
- `Project/Configuration manager` the *RelWithDebInfo*

For both `openvisus_vol_cli` and `openvisus_vol_cli` set (**change as needed**)
- set *Working directory* : `C:\projects\OpenVisus`
- set *Environment*: 

```
HDF5_PLUGIN_PATH=C:\projects\OpenVisus\build_paraview\RelWithDebInfo\OpenVisus\bin
HDF5_VOL_CONNECTOR=openvisus_vol under_vol=0;under_info={};
PATH=D:\hdf5\install\bin;C:\projects\OpenVisus\build_paraview\RelWithDebInfo\OpenVisus\bin
PYTHONPATH=C:\projects\OpenVisus\build_paraview\RelWithDebInfo
VISUS_HDF5_ENABLE_LOGGING=1
VISUS_HDF5_ENABLE_PYTHON=0
```

## Test C++ API

Right-click `HDF5/openvisus_vol_cli` project and *"Set as Startup Project"*. 
This project will test the plugin either with IDX backend and default backend.

Set:
-  For `openvisus_vol_cli` set *Command Arguments* to: `--test`

## Test Paraview

Right-click `HDF5/openvisus_vol_cli`  project and *"Set as Startup Project"*

And (**change as needed**):
- set *Command* to `C:\Program Files\ParaView 5.10.0-Windows-Python3.9-msvc2017-AMD64\bin\paraview.exe`

Run and open one of the file generated by the preview step.

As a reader (what is the difference) the one that seems to work is with **Pixie in the name**.
 

