@echo on

set CONDA_DIR=C:\Miniconda%PYTHON_VERSION%-x64
%CONDA_DIR%\Scripts\activate.bat

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

del %CONDA_DIR%\conda-bld\win-64\openvisus*.tar.bz2
python setup.py -q bdist_conda 

if "%APPVEYOR_REPO_TAG%" == "true" (
	python -c "import glob;open('~conda_filename.txt', 'wt').write(str(glob.glob('%CONDA_DIR%/conda-bld/win-64/openvisus*.tar.bz2')[0]))"
	set /P CONDA_FILENAME=<~conda_filename.txt
	anaconda -t "%ANACONDA_TOKEN%" upload "%CONDA_FILENAME%"
)

