if "%CMAKE_BUILD_TYPE%"=="" (
 	set CMAKE_BUILD_TYPE=RelWithDebInfo
)

mkdir build 
cd build 

cmake ^
   -G "%CMAKE_GENERATOR%" ^
   -DVISUS_GUI=0 ^
   -DPYTHON_VERSION="%PY_VER%" ^
   -DPYTHON_EXECUTABLE="%PYTHON%" ^
   ..
if errorlevel 1 exit 1

cmake --build . --target ALL_BUILD --config %CMAKE_BUILD_TYPE%
if errorlevel 1 exit 1

cmake --build . --target INSTALL   --config %CMAKE_BUILD_TYPE%
if errorlevel 1 exit 1

cmake --build . --target dist      --config %CMAKE_BUILD_TYPE%
if errorlevel 1 exit 1

cd %CMAKE_BUILD_TYPE%\OpenVisus

"%PYTHON%" setup.py install --single-version-externally-managed --record=record.txt
if errorlevel 1 exit 1

