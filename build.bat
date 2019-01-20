@echo on

set THIS_DIR=%cd%

if "%CMAKE_EXECUTABLE%"=="" (
	set CMAKE_EXECUTABLE=cmake
)

if "%CMAKE_GENERATOR%"=="" (
	set CMAKE_GENERATOR=Visual Studio 15 2017 Win64
)

if NOT "%CMAKE_TOOLCHAIN_FILE%"=="" (
	cd c:\tools\vcpkg 
	vcpkg.exe install zlib:x64-windows lz4:x64-windows tinyxml:x64-windows freeimage:x64-windows openssl:x64-windows curl:x64-windows
)

if not defined DevEnvDir (
 	call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat"
)

REM install numpy
cd %THIS_DIR%
"%PYTHON_EXECUTABLE%" -m pip install --user numpy

cd %THIS_DIR%
mkdir build 
cd build 

REM setup step
"%CMAKE_EXECUTABLE%" ^
	-G "%CMAKE_GENERATOR%" ^
	-DQt5_DIR="%QT5_DIR%" ^
	-DPYTHON_EXECUTABLE=%PYTHON_EXECUTABLE% ^
	-DCMAKE_TOOLCHAIN_FILE="%CMAKE_TOOLCHAIN_FILE%" ^
	-DVCPKG_TARGET_TRIPLET="%VCPKG_TARGET_TRIPLET%" ^
	../

REM build step
"%CMAKE_EXECUTABLE%" --build . --target ALL_BUILD --config RelWithDebInfo

REM run test step
"%CMAKE_EXECUTABLE%" --build . --target RUN_TESTS --config RelWithDebInfo

REM install step
"%CMAKE_EXECUTABLE%" --build . --target INSTALL   --config RelWithDebInfo
 

