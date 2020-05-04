@echo on

set PATH=C:/Python%PYTHON_VERSION%-x64;%PATH%

choco install -y --allow-empty-checksums swig
python -m pip install numpy setuptools wheel twine --upgrade

mkdir build_appveyor
cd build_appveyor

cmake.exe -G "Visual Studio 16 2019" -A "x64" -DQt5_DIR="C:\Qt\5.9\msvc2017_64\lib\cmake\Qt5" -DPython_EXECUTABLE=C:/Python%PYTHON_VERSION%-x64/python.exe  ../
cmake.exe --build . --target ALL_BUILD            --config Release
cmake.exe --build . --target INSTALL              --config Release

cd Release\OpenVisus

set PYTHONPATH=..\
python -m OpenVisus test
python -m OpenVisus convert
set PYTHONPATH=

if "%APPVEYOR_REPO_TAG%" == "true" (
	python setup.py -q bdist_wheel --python-tag=cp%PYTHON_VERSION% --plat-name=win_amd64
	python -m twine upload --username %PYPI_USERNAME% --password %PYPI_PASSWORD% --skip-existing  "dist/*.whl"
)

set BUILD_CONDA=0
if "%PYTHON_VERSION%" == "36" ( set BUILD_CONDA=1 )
if "%PYTHON_VERSION%" == "37" ( set BUILD_CONDA=1 )
if "BUILD_CONDA" == "1" (

	set CONDA_DIR=C:\Miniconda%PYTHON_VERSION%-x64
	%CONDA_DIR%\Scripts\activate.bat

	conda install conda-build numpy -y
	
	set PYTHONPATH=..\
	python -m OpenVisus configure
	set PYTHONPATH=
	
	del %CONDA_DIR%\conda-bld\win-64\openvisus*.tar.bz2
	python setup.py -q bdist_conda 
	python -c "import glob;open('~conda_filename.txt', 'wt').write(str(glob.glob('%CONDA_DIR%/conda-bld/win-64/openvisus*.tar.bz2')[0]))"
	set /P CONDA_FILENAME=<~conda_filename.txt
	conda install -y --force-reinstall %CONDA_FILENAME%
	cd /d %CONDA_DIR%\lib\site-packages\OpenVisus

	python -m OpenVisus test
	python -m OpenVisus convert
	
	if "%APPVEYOR_REPO_TAG%" == "true" (
		conda install anaconda-client -y
		anaconda -t %ANACONDA_TOKEN% upload %CONDA_FILENAME%
	)

)

