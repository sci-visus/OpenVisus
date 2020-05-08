SETLOCAL EnableDelayedExpansion
  
set PYTHONUNBUFFERED=1

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

python setup.py -q bdist_wheel --python-tag=cp%PYTHON_VERSION% --plat-name=win_amd64
echo "wheel produced"

if "%GIT_TAG%" == "true" ( 
	python -m twine upload --username %PYPI_USERNAME% --password %PYPI_PASSWORD% --skip-existing  "dist/*.whl" 
)


  

