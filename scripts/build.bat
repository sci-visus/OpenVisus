@echo on

REM to test
REM set PYTHON_VERSION=38
REM build.bat

set PYTHON_EXECUTABLE=C:/Python%PYTHON_VERSION%-x64/python.exe 

"%PYTHON_EXECUTABLE%" -m pip install numpy setuptools wheel twine --upgrade

mkdir build 
cd build

cmake.exe -G "Visual Studio 15 2017 Win64" -DQt5_DIR="C:\Qt\5.11\msvc2017_64\lib\cmake\Qt5" -DPYTHON_EXECUTABLE=%PYTHON_EXECUTABLE% ../
cmake.exe --build . --target ALL_BUILD --config Release
cmake.exe --build . --target INSTALL   --config Release

cd build\Release\OpenVisus
 
set PYTHONPATH=..\
%PYTHON_EXECUTABLE% Samples/python/Array.py 
%PYTHON_EXECUTABLE% Samples/python/Dataflow.py 
%PYTHON_EXECUTABLE% Samples/python/Dataflow2.py 
%PYTHON_EXECUTABLE% Samples/python/Idx.py 
%PYTHON_EXECUTABLE% Samples/python/XIdx.py 
%PYTHON_EXECUTABLE% Samples/python/DataConversion1.py 
%PYTHON_EXECUTABLE% Samples/python/DataConversion2.py
set PYTHONPATH=

.\visus.bat


if "%APPVEYOR_REPO_TAG%" == "true" (

	rmdir "dist"       /s /q
	rmdir "build"      /s /q
	rmdir "__pycache_" /s /q
	"%PYTHON_EXECUTABLE%" setup.py -q bdist_wheel --python-tag=cp%PYTHON_VERSION% --plat-name=win_amd64

	set HOME=%USERPROFILE% 
	"%PYTHON_EXECUTABLE%" -m twine upload --username %PYPI_USERNAME% --password %PYPI_PASSWORD% --skip-existing  "dist/*.whl"
)


if "BUILD_CONDA" != "0" (

	C:\Miniconda%PYTHON_VERSION%-x64

	export PATH=~/miniconda3/bin:$PATH
	source ~/miniconda3/etc/profile.d/conda.sh
	eval "$(conda shell.bash hook)" # see https://github.com/conda/conda/issues/8072
	
	conda install conda-build -y
	PYTHONPATH=$(pwd)/.. python -m OpenVisus use-pyqt
	rmdir "dist"       /s /q
	rmdir "build"      /s /q
	rmdir "__pycache_" /s /q
	rm -Rf $(find ~/miniconda3/conda-bld -iname "openvisus*.tar.bz2")
	python setup.py -q bdist_conda 
	CONDA_FILENAME=$(find C:\Miniconda%PYTHON_VERSION%-x64/conda-bld -iname "openvisus*.tar.bz2")

	conda install -y --force-reinstall %CONDA_FILENAME%
	
	python Samples/python/Array.py 
	python Samples/python/Dataflow.py 
	python Samples/python/Dataflow2.py 
	python Samples/python/Idx.py 
	python Samples/python/XIdx.py 
	python Samples/python/DataConversion1.py 
	python Samples/python/DataConversion2.py
	
	if "%APPVEYOR_REPO_TAG%" == "true" (
		conda install anaconda-client -y
		anaconda -t %ANACONDA_TOKEN% upload "%CONDA_BUILD_FILENAME%"
	)

)
