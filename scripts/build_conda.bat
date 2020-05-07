@echo on

CALL %CONDA_DIR%\Scripts\activate.bat
CALL conda config --set always_yes yes --set changeps1 no --set anaconda_upload no
CALL conda update -q conda
CALL conda env remove -n tmp
CALL conda create -q -n tmp python=%PYTHON_VERSION:~0,1%.%PYTHON_VERSION:~2,1% numpy
CALL conda activate tmp
CALL conda install conda-build anaconda-client 

set PYTHONPATH=..\
python -m OpenVisus configure
python -m OpenVisus test
python -m OpenVisus convert
set PYTHONPATH=

python -c "import os,glob;[os.remove(it) for it in glob.glob('%CONDA_DIR%\\**\\openvisus*.tar.bz2',recursive=True)]"
python setup.py -q bdist_conda 
python -c "import os,glob;open('~conda_filename.txt', 'wt').write(str(glob.glob('%CONDA_DIR%\\**\\openvisus*.tar.bz2', recursive=True)[0]))"
set /P CONDA_FILENAME=<~conda_filename.txt
echo "CONDA_FILENAME=%CONDA_FILENAME%"

if "%APPVEYOR_REPO_TAG%" == "true" (
	CALL anaconda -t "%ANACONDA_TOKEN%" upload "%CONDA_FILENAME%"
)

