@echo on

set THIS_DIR=%cd%

if "%CMAKE_EXECUTABLE%"=="" (
	set CMAKE_EXECUTABLE=cmake
)

if "%CMAKE_GENERATOR%"=="" (
	set CMAKE_GENERATOR=Visual Studio 15 2017 Win64
)

if not defined DevEnvDir (
 	call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat"
)

cd c:\tools\vcpkg 
vcpkg.exe install zlib:x64-windows lz4:x64-windows tinyxml:x64-windows freeimage:x64-windows openssl:x64-windows curl:x64-windows
cd %THIS_DIR%

"%PYTHON_EXECUTABLE%" -m pip install --user numpy

cd %THIS_DIR%
mkdir build 
cd build 
"%CMAKE_EXECUTABLE%" ^
	-G "%CMAKE_GENERATOR%" ^
	-DCMAKE_TOOLCHAIN_FILE="c:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake" ^
	-DVCPKG_TARGET_TRIPLET="x64-windows" ^
	-DQt5_DIR="%QT5_DIR%/lib/cmake/Qt5" ^
	-DPYPI_USERNAME=%PYPI_USERNAME% ^
	-DPYPI_PASSWORD=%PYPI_PASSWORD% ^
	-DPYTHON_EXECUTABLE=%PYTHON_EXECUTABLE% ^
	../

"%CMAKE_EXECUTABLE%" --build . --target ALL_BUILD --config RelWithDebInfo
"%CMAKE_EXECUTABLE%" --build . --target RUN_TESTS --config RelWithDebInfo
"%CMAKE_EXECUTABLE%" --build . --target INSTALL   --config RelWithDebInfo

