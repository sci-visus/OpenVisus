
# Example of producing a minimal static library 

Minimal means:
- no python
- no FreeImage
- no CURL network service
- no Gui
- just flat OpenVisus IDX (i.e. Kernel and DB)


(OPTIONAL) Set the path to find cmake:

```
set PATH="C:\Program Files\CMake\bin";%PATH
```

Choose the configuration you want to try (DO NOT mix Debug with Release and viceversa):

```
set CONFIG=Release
```

Compile OpenVisus static library (NOTE I am compiling/installing all configurations just in case):

```
mkdir build_minimal 
cd build_minimal
cmake -G "Visual Studio 16 2019" -A "x64" -DVISUS_MINIMAL=1 ../ 
cmake --build . --target ALL_BUILD --config %CONFIG%
cmake --build . --target INSTALL   --config %CONFIG%
set OPENVISUS_DIR=%cd%\%CONFIG%\OpenVisus
cd ..
```

Then consume the library linking:

```
cd Executable\test_static_linking
mkdir build
cd build
cmake -G "Visual Studio 16 2019" -A "x64" -DOPENVISUS_DIR=%OPENVISUS_DIR% ../
cmake --build . --target ALL_BUILD --config %CONFIG%
```






