@echo on

REM set PYTHON_VERSION=3.7
REM set Python_EXECUTABLE=C:\Python37\python.exe
REM set BUILD_CONDA=1
REM set PATH=%PATH%;C:\Program Files\CMake\bin;C:\Program Files\swig
REM set Qt5_DIR=D:\Qt\5.9.9\msvc2017_64\lib\cmake\Qt5
REM .\scripts\build_win.bat
REM set CONDA_DIR=C:\Miniconda37-x64
REM if NOT "%CONDA_DIR%" == "" ( .\scripts\build_conda.bat )

set

set PATH=%Python_EXECUTABLE%\..;%PATH%

choco install -y --allow-empty-checksums swig
python -m pip install numpy setuptools wheel twine --upgrade

mkdir build_win
cd build_win

cmake.exe -G "Visual Studio 16 2019" -A "x64" -DQt5_DIR=%Qt5_DIR% -DPython_EXECUTABLE=%Python_EXECUTABLE%  ../
cmake.exe --build . --target ALL_BUILD            --config Release
cmake.exe --build . --target INSTALL              --config Release

cd Release\OpenVisus

set PYTHONPATH=..\
python -m OpenVisus test
python -m OpenVisus convert
set PYTHONPATH=

if "%APPVEYOR_REPO_TAG%" == "true" (
	python setup.py -q bdist_wheel --python-tag=cp%PYTHON_VERSION:~0,1%%PYTHON_VERSION:~2,1% --plat-name=win_amd64
	python -m twine upload --username %PYPI_USERNAME% --password %PYPI_PASSWORD% --skip-existing  "dist/*.whl"
)

