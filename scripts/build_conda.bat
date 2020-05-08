@echo on
SETLOCAL EnableDelayedExpansion

set PYTHONUNBUFFERED=1
  
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

git describe --tags --exact-match 2>NUL
if ERRORLEVEL == 0 ( 
	anaconda -t "%ANACONDA_TOKEN%" upload "%CONDA_FILENAME%" 
)  