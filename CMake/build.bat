@echo on

set THIS_DIR=%cd%

if "%CMAKE_BUILD_TYPE%"=="" (
	set CMAKE_BUILD_TYPE=Release
)

if "%CMAKE_EXECUTABLE%"=="" (

	IF EXIST "C:\Program Files\CMake\bin\cmake.exe" (
		set CMAKE_EXECUTABLE=C:\Program Files\CMake\bin\cmake.exe
	) ELSE (
		set CMAKE_EXECUTABLE=cmake.exe
	)
)

if "%CMAKE_GENERATOR%"=="" (
	set CMAKE_GENERATOR=Visual Studio 15 2017 Win64
)

if NOT "%CMAKE_TOOLCHAIN_FILE%"=="" (
	cd c:\tools\vcpkg 
	vcpkg.exe install zlib:x64-windows lz4:x64-windows tinyxml:x64-windows freeimage:x64-windows openssl:x64-windows curl:x64-windows
)

if "%VCPKG_TARGET_TRIPLET%"=="" (
	set VCPKG_TARGET_TRIPLET=x64-windows
)

if "%PYTHON_EXECUTABLE%"=="" (
	IF EXIST "C:\Python37\python.exe" (
		set PYTHON_EXECUTABLE=C:\Python37\python.exe
	) ELSE (
		IF EXIST "C:\Python36\python.exe" ( 
			set PYTHON_EXECUTABLE=C:\Python36\python.exe
		)	
	)
)

if "%QT5_DIR%"=="" (
	IF EXIST "C:\Qt\5.11.2\msvc2015_64\lib\cmake\Qt5" (
		set Qt5_DIR=C:\Qt\5.11.2\msvc2015_64\lib\cmake\Qt5
	) ELSE (
		IF EXIST "C:\Qt\5.11.1\msvc2015_64\lib\cmake\Qt5" (
			set Qt5_DIR=C:\Qt\5.11.1\msvc2015_64\lib\cmake\Qt5
		)
	)
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
"%CMAKE_EXECUTABLE%" --build . --target ALL_BUILD --config %CMAKE_BUILD_TYPE%

REM install step
"%CMAKE_EXECUTABLE%" --build . --target INSTALL   --config %CMAKE_BUILD_TYPE%

REM run test step
"%CMAKE_EXECUTABLE%" --build . --target RUN_TESTS --config %CMAKE_BUILD_TYPE%
 
REM dist step
if NOT "%APPVEYOR%"=="" (
	"%CMAKE_EXECUTABLE%" --build . --target DIST   --config %CMAKE_BUILD_TYPE%
)

REM test OpenVisus 
set PYTHONPATH=%THIS_DIR%\build\%CMAKE_BUILD_TYPE%
cd %THIS_DIR%\build\%CMAKE_BUILD_TYPE%\OpenVisus

%PYTHON_EXECUTABLE% Samples/python/Array.py
%PYTHON_EXECUTABLE% Samples/python/Dataflow.py
%PYTHON_EXECUTABLE% Samples/python/Idx.py

.\visus.bat

echo "OpenVisus build finished"
