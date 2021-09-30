# OpenVisus HD5 plugin

NOTE Compatible with HDF 1.12 (1.13 in in GitHub)

See https://github.com/hpc-io/vol-external-passthrough
See https://github.com/DataLib-ECP/vol-log-based/blob/b13778efd9e0c79135a9d7352104985408078d45/src/H5VL_log_obj.cpp as an example

To Debug in Windows, Debugging/Environment:

```
HDF5_PLUGIN_PATH=C:\projects\OpenVisus\build\RelWithDebInfo\OpenVisus\bin
HDF5_VOL_CONNECTOR=openvisus_vol under_vol=0;under_info={};
PATH=D:\hdf5-1.12.1\install\bin;C:\projects\OpenVisus\build\RelWithDebInfo\OpenVisus\bin;c:\python38
PYTHONPATH=C:\projects\OpenVisus\build\RelWithDebInfo
```

We have tested so far:
- [OK]  HDF5 C Api 
- [OK]  python h5py library
- [OK] jupyter h5py library
- [ERR] hdf5view (HDF5_PLUGIN_PATH seems to be ignored)
- [?] Paraview. It is using the plugin but I don't have no idea how to test it
 
```

set HDF5_PLUGIN_PATH=C:\projects\OpenVisus\build\RelWithDebInfo\OpenVisus\bin
set HDF5_VOL_CONNECTOR=openvisus_vol under_vol=0;under_info={};
set PATH=C:\projects\OpenVisus\build\RelWithDebInfo\OpenVisus\bin;%PATH%
set PYTHONPATH=C:\projects\OpenVisus\build\RelWithDebInfo

"D:\ParaView 5.9.1-Windows-Python3.8-msvc2017-64bit\bin\paraview"
```

