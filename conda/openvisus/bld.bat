
echo "CMAKE_GENERATOR %CMAKE_GENERATOR%"
echo "PYTHON          %PYTHON%"
echo "PY_VER          %PY_VER%"

mkdir build 
cd build 

REM Configure step
cmake -G "%CMAKE_GENERATOR%" ^
	-DPYTHON_EXECUTABLE=%PYTHON% ^
	-DPYTHON_VERSION=%PY_VER% ^
	-DDISABLE_OPENMP=1 ^
	-DVISUS_GUI=0 ^
	-DVISUS_INTERNAL_DEFAULT=1 ^
	..
	
if errorlevel 1 exit 1

REM Build step
set CMAKE_BUILD_TYPE=RelWithDebugInfo
cmake --build . --target ALL_BUILD --config %CMAKE_BUILD_TYPE%
if errorlevel 1 exit 1

REM Install step
cmake --build . --target INSTALL   --config %CMAKE_BUILD_TYPE%
if errorlevel 1 exit 1
 

