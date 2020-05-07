@echo on

set PYTHONUNBUFFERED=1

choco install -y --allow-empty-checksums swig
set PATH=%Python_EXECUTABLE%\..;%PATH%
python -m pip install numpy setuptools wheel twine --upgrade
  
mkdir build_win
cd build_win
cmake.exe -G "Visual Studio 16 2019" -A "x64" -DQt5_DIR=%Qt5_DIR% -DPython_EXECUTABLE=%Python_EXECUTABLE%  ../
cmake.exe --build . --target ALL_BUILD --config Release
cmake.exe --build . --target INSTALL   --config Release

cd Release\OpenVisus
set PYTHONPATH=..\
python -m OpenVisus test
python -m OpenVisus convert
set PYTHONPATH=
  
python setup.py -q bdist_wheel --python-tag=%PYTHON_TAG% --plat-name=win_amd64
if "%APPVEYOR_REPO_TAG%" == "true" ( python -m twine upload --username %PYPI_USERNAME% --password %PYPI_PASSWORD% --skip-existing  "dist/*.whl" )
  
if "%PYTHON_VERSION%" == "3.8" appveyor exit
  
set "PATH=%MINICONDA%\\Scripts;%MINICONDA%\\Library\\bin;%PATH%"
call %MINICONDA%\Scripts\activate.bat
conda config --set always_yes yes --set changeps1 no --set anaconda_upload no
conda update -q conda
conda env remove -n tmp
conda create -q -n tmp python=%PYTHON_VERSION% numpy
conda activate tmp
conda install conda-build anaconda-client 
  
set PYTHONPATH=..\
python -m OpenVisus configure
python -m OpenVisus test
python -m OpenVisus convert
set PYTHONPATH=
  
python -c "import os,glob;[os.remove(it) for it in glob.glob('%MINICONDA%\\**\\openvisus*.tar.bz2',recursive=True)]"
python setup.py -q bdist_conda 
python -c "import os,glob;open('~conda_filename.txt', 'wt').write(str(glob.glob('%MINICONDA%\\**\\openvisus*.tar.bz2', recursive=True)[0]))"
set /P CONDA_FILENAME=<~conda_filename.txt
echo "CONDA_FILENAME=%CONDA_FILENAME%"
if "%APPVEYOR_REPO_TAG%" == "true" ( anaconda -t "%ANACONDA_TOKEN%" upload "%CONDA_FILENAME%" )