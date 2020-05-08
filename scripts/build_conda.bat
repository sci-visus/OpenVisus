SETLOCAL EnableDelayedExpansion

set PYTHONUNBUFFERED=1

set "PATH=%MINICONDA_DIR%\Scripts;%MINICONDA_DIR%\Library\bin;%PATH%"

call %MINICONDA_DIR%\Scripts\activate.bat
conda config --set always_yes yes --set changeps1 no --set anaconda_upload no
conda update -q conda
conda env remove -n tmp
conda create -q -n tmp python=%PYTHON_VERSION:~0,1%.%PYTHON_VERSION:~1,1% numpy
conda activate tmp
conda install conda-build anaconda-client 
  
set PYTHONPATH=..\
python -m OpenVisus configure
python -m OpenVisus test
python -m OpenVisus convert
set PYTHONPATH=
  
python -c "import os,glob;[os.remove(it) for it in glob.glob(r'%MINICONDA_DIR%\**\openvisus*.tar.bz2',recursive=True)]"
python setup.py -q bdist_conda 
python -c "import glob;print(glob.glob(r'%MINICONDA_DIR%\**\openvisus*.tar.bz2', recursive=True)[0])" >tmp.txt 
set /P CONDA_FILENAME=<tmp.txt
echo "CONDA_FILENAME=%CONDA_FILENAME%"

if "%GIT_TAG%" == "true" ( 
	anaconda -t "%ANACONDA_TOKEN%" upload "%CONDA_FILENAME%" 
)  