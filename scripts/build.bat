@echo on

REM to test
REM call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
REM set Python_EXECUTABLE=C:/Python37/python.exe 
REM set Qt5_DIR=D:\Qt\5.9.9\msvc2017_64\lib\cmake\Qt5
REM set GENERATOR=Visual Studio 16 2019
REM set CONDA_DIR=C:\Miniconda37-x64
REM set PYTHON_TAG=cp37
REM build.bat

"%Python_EXECUTABLE%" -m pip install numpy setuptools wheel twine --upgrade

mkdir build_appveyor
cd build_appveyor

cmake.exe -G "%GENERATOR%" -A "x64" -DQt5_DIR="%Qt5_DIR%" -DPython_EXECUTABLE=%Python_EXECUTABLE% ../
cmake.exe --build . --target ALL_BUILD            --config Release
cmake.exe --build . --target INSTALL              --config Release

cd Release\OpenVisus
 
set PYTHONPATH=..\
"%Python_EXECUTABLE%" Samples/python/Array.py 
"%Python_EXECUTABLE%" Samples/python/Dataflow.py 
"%Python_EXECUTABLE%" Samples/python/Dataflow2.py 
"%Python_EXECUTABLE%" Samples/python/Idx.py 
"%Python_EXECUTABLE%" Samples/python/XIdx.py 
"%Python_EXECUTABLE%" Samples/python/DataConversion1.py 
"%Python_EXECUTABLE%" Samples/python/DataConversion2.py
"%Python_EXECUTABLE%" -m OpenVisus GENERATE-SCRIPTS qt5
set PYTHONPATH=

.\visus.bat

if "%APPVEYOR_REPO_TAG%" == "true" (
	"%Python_EXECUTABLE%" setup.py -q bdist_wheel --python-tag=%PYTHON_TAG% --plat-name=win_amd64
	"%Python_EXECUTABLE%" -m twine upload --username %PYPI_USERNAME% --password %PYPI_PASSWORD% --skip-existing  "dist/*.whl"
)

if "BUILD_CONDA" != "0" (

	%CONDA_DIR%\Scripts\activate.bat

	conda install conda-build numpy -y
	
	REM remove qt5 and use conda pyqt
	set PYTHONPATH=..\
	python -m OpenVisus CONFIGURE
	set PYTHONPATH=
	
	del %CONDA_DIR%\conda-bld\win-64\openvisus*.tar.bz2
	python setup.py -q bdist_conda 
	python -c "import glob;open('~conda_filename.txt', 'wt').write(str(glob.glob('%CONDA_DIR%/conda-bld/win-64/openvisus*.tar.bz2')[0]))"
	set /P CONDA_FILENAME=<~conda_filename.txt
	conda install -y --force-reinstall %CONDA_FILENAME%
	cd /d %CONDA_DIR%\lib\site-packages\OpenVisus
	
	python Samples/python/Array.py 
	python Samples/python/Dataflow.py 
	python Samples/python/Dataflow2.py 
	python Samples/python/Idx.py 
	python Samples/python/XIdx.py 
	python Samples/python/DataConversion1.py 
	python Samples/python/DataConversion2.py
	.\visus.bat
	
	if "%APPVEYOR_REPO_TAG%" == "true" (
		conda install anaconda-client -y
		anaconda -t %ANACONDA_TOKEN% upload %CONDA_FILENAME%
	)

)
