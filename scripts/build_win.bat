@echo on

set PATH=C:/Python%PYTHON_VERSION%-x64;%PATH%

choco install -y --allow-empty-checksums swig
python -m pip install numpy setuptools wheel twine --upgrade

mkdir build_win
cd build_win

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

