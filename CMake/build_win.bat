@echo on

REM setup compiler 
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat"

REM need vcpkg libraries
cd c:\tools\vcpkg
vcpkg.exe install zlib:x64-windows lz4:x64-windows tinyxml:x64-windows freeimage:x64-windows openssl:x64-windows curl:x64-windows

REM install numpy 
"%PYTHON_EXECUTABLE%" -m pip install --user numpy

REM build OpenVisus
cd %APPVEYOR_BUILD_FOLDER% 
mkdir build 
cd build 
cmake ^
	-G "%CMAKE_GENERATOR%" ^
	-DCMAKE_TOOLCHAIN_FILE="c:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake" ^
	-DVCPKG_TARGET_TRIPLET="x64-windows" ^
	-DQt5_DIR="%QT5_DIR%/lib/cmake/Qt5" ^
	-DPYPI_USERNAME=%PYPI_USERNAME% ^
	-DPYPI_PASSWORD=%PYPI_PASSWORD% ^
	-DPYTHON_EXECUTABLE=%PYTHON_EXECUTABLE% ^
	../

cmake --build . --target ALL_BUILD --config RelWithDebInfo
cmake --build . --target RUN_TESTS --config RelWithDebInfo
cmake --build . --target INSTALL   --config RelWithDebInfo
cmake --build . --target deploy    --config RelWithDebInfo

if %DEPLOY_GITHUB%==1 (
   echo "Deploying to github enabled"
   cmake --build . --target sdist --config RelWithDebInfo
   "%PYTHON_EXECUTABLE%" -c "import os;import glob;filename=glob.glob('install/dist/*.zip')[0];os.rename(filename,filename.replace('.zip','-%PYTHON_VERSION%-win_amd64.zip'))" 
)
 
if %DEPLOY_PYPI%==1 (
   echo "Deploying to pypi enabled"
   cmake --build . --target bdist_wheel   --config RelWithDebInfo
   cmake --build . --target pypi          --config RelWithDebInfo
)