# OpenViSUS Visualization project  
     
![GitHub Actions](https://github.com/sci-visus/OpenVisus/workflows/BuildOpenVisus/badge.svg)
[![Binder](https://mybinder.org/badge_logo.svg)](https://mybinder.org/v2/gh/sci-visus/OpenVisus/master?filepath=Samples%2Fjupyter)
 
 
The mission of ViSUS.org is to provide support for the scientific community with Big Data, management, analysis and visualization tools.
In this website we provide access to open source software tools and libraries such as the ViSUS framework and the PIDX library.
These softwares are distributed under the permissive BSD license.

Table of content:

- [Binary Distribution](#binary-distribution)

- [Windows compilation Visual Studio](#windows-compilation-visual-studio)

- [Windows compilation mingw](#windows-compilation-mingw)

- [MacOSX compilation clang](#macosx-compilation-clang)

- [MacOSX compilation gcc](#macosx-compilation-gcc)

- [Linux compilation gcc](#linux-compilation-gcc)

- [Minimal compilation](#minimal-compilation)


<!--//////////////////////////////////////////////////////////////////////// -->
## Binary distribution

If you are using `pip`

```
# For Linux sometimes you have to install some python libraries 
# sudo apt-get install python3.6 libpython3/6

# sometimes pip is not installed or is too old
# wget https://bootstrap.pypa.io/get-pip.py
# python3 get-pip.py
# python3 -m pip install --user --no-cache-dir  --upgrade pip

python -m pip install --user --upgrade pip
python -m pip install --user virtualenv

cd /path/to/your/project

# create the environment
# replace 'myenv' here and below with your name
python -m venv myenv

# on Windows: .\env\Scripts\activate
source myenv/bin/activate

python -m pip install --upgrade OpenVisus
python -m OpenVisus configure 
python -m OpenVisus viewer

# (OPTIONAL) deactivate the environment
deactivate

# (OPTIONAL) remove the environment
rm -r /path/to/your/project/myenv
```

If you are using `conda`:

```

# replace 'myenv' with whatever you want for the new environment 
# replace '3.6' with the wanted python version
conda create -n myenv python=3.6 

# activate it
conda activate myenv

# install openvisus inside conda
conda install --name myenv  -y --channel visus openvisus

# IMPORTANT trick to avoid problems with other pip packages installed in ~/.local (see https://github.com/conda/conda/issues/7173)
export PYTHONNOUSERSITE=True 

conda install --name myenv  -y conda

python -m OpenVisus configure
python -m OpenVisus viewer

# (OPTIONAL) deactivate the environment
conda deactivate

# (OPTIONAL) remove the environment
conda remove --name myenv --all

```

Give a look to directory `Samples/python` and Jupyter examples:

[Samples/jupyter/quick_tour.ipynb](https://github.com/sci-visus/OpenVisus/blob/master/Samples/jupyter/quick_tour.ipynb)

[Samples/jupyter/Agricolture.ipynb](https://github.com/sci-visus/OpenVisus/blob/master/Samples/jupyter/Agricolture.ipynb)

[Samples/jupyter/Climate.ipynb](https://github.com/sci-visus/OpenVisus/blob/master/Samples/jupyter/Climate.ipynb)

[Samples/jupyter/ReadAndView.ipynb](https://github.com/sci-visus/OpenVisus/blob/master/Samples/jupyter/ReadAndView.ipynb)



# Binary Windows IDX distribution

Download the zip file containing the header files, static library, and dynamic library.

If you want to use the static library, link with.




<!--//////////////////////////////////////////////////////////////////////// -->
## Windows compilation Visual Studio

Install git, cmake and swig.  
The fastest way is to use `chocolatey`:

```
choco install -y git cmake swig
```

Install Python3.x.

Install Qt5 (http://download.qt.io/official_releases/online_installers/qt-unified-windows-x86-online.exe)

To compile OpenVisus (change the paths as needed):

```
set Python_EXECUTABLE=C:\Python37\python.exe
set Qt5_DIR=D:\Qt\5.12.8\5.12.8\msvc2017_64\lib\cmake\Qt5

python -m pip install numpy

git clone https://github.com/sci-visus/OpenVisus
cd OpenVisus
mkdir build
cd build
cmake -G "Visual Studio 16 2019" -A "x64" -DQt5_DIR=%Qt5_DIR% -DPython_EXECUTABLE=%Python_EXECUTABLE% ../ 
cmake --build . --target ALL_BUILD --config Release
cmake --build . --target INSTALL   --config Release

set PYTHON_PATH=.\Release
python -m OpenVisus configure --user
python -m OpenVisus viewer
```


<!--//////////////////////////////////////////////////////////////////////// -->
## Windows compilation mingw

NOTE: only VISUS_MINIMAL is supported.

Install prerequisites. The fastest way is to use `chocolatey`:

```
choco install -y git cmake mingw
```

To compile OpenVisus (change the paths as needed):

```
git clone https://github.com/sci-visus/OpenVisus
cd OpenVisus

mkdir build_gcc
cd build_gcc

set PATH=%PATH%;C:\ProgramData\chocolatey\lib\mingw\tools\install\mingw64\bin


cmake -G "MinGW Makefiles" -DVISUS_MINIMAL=1 ../ 
cmake --build . --target all       --config Release
cmake --build . --target install   --config Release

set PYTHON_PATH=.\Release
python -m OpenVisus configure --user
python -m OpenVisus viewer
```


<!--//////////////////////////////////////////////////////////////////////// -->
## MacOSX compilation clang

Make sure you have command line tools:

```
sudo xcode-select --install || sudo xcode-select --reset
```

Build the repository (change as needed):

```
git clone https://github.com/sci-visus/OpenVisus
cd OpenVisus

# change as needed if you have python in another place
Python_EXECUTABLE=/Library/Frameworks/Python.framework/Versions/3.6/bin/python3

# install prerequisites
brew install swig cmake

# install qt5 (change as needed)
brew install qt5
Qt5_DIR=$(brew --prefix qt5)/lib/cmake/Qt5

mkdir build 
cd build

cmake -GXcode -DPython_EXECUTABLE=${Python_EXECUTABLE} -DQt5_DIR=${Qt5_DIR} ../
cmake --build ./ --target ALL_BUILD --config Release --parallel 4
cmake --build ./ --target install   --config Release

export PYTHONPATH=$(pwd)/Release

# this command will install PyQt5 and link OpenVisus to PyQt5 in user space (given that you don't want to run as root)
python3 -m OpenVisus configure --user
python3 -m OpenVisus test
python3 -m OpenVisus viewer

# OPTIONAL
python3 -m pip install --upgrade opencv-python opencv-contrib-python 
python3 -m OpenVisus test-viewer1
python3 -m OpenVisus test-viewer2

# OPTIONAL
python3 -m pip install --upgrade jupyter
python3 -m jupyter notebook ../Samples/jupyter/Agricolture.ipynb
```


<!--//////////////////////////////////////////////////////////////////////// -->
## MacOSX compilation gcc

Maybe you need to install gcc:

```
brew install gcc@9
```

Build the repository (change as needed):

```

# change the path for your gcc
export CC=$(brew --prefix gcc@9)/bin/gcc-9
export CXX=$(brew --prefix gcc@9)/bin/g++-9


git clone https://github.com/sci-visus/OpenVisus
cd OpenVisus

# change as needed if you have python in another place
Python_EXECUTABLE=/Library/Frameworks/Python.framework/Versions/3.6/bin/python3

# install prerequisites
brew install swig cmake

# install qt5 (change as needed)
brew install qt5
Qt5_DIR=$(brew --prefix qt5)/lib/cmake/Qt5

mkdir build_gcc
cd build_gcc
cmake -G"Unix Makefiles" -DPython_EXECUTABLE=${Python_EXECUTABLE} -DQt5_DIR=${Qt5_DIR} ../
cmake --build ./ --target all       --config Release --parallel 8
cmake --build ./ --target install   --config Release

export PYTHONPATH=$(pwd)/Release

# this command will install PyQt5 and link OpenVisus to PyQt5 in user space (given that you don't want to run as root)
python3 -m OpenVisus configure --user
python3 -m OpenVisus test
python3 -m OpenVisus viewer

# OPTIONAL
python3 -m pip install --upgrade opencv-python opencv-contrib-python 
python3 -m OpenVisus viewer1
python3 -m OpenVisus viewer2

# OPTIONAL
python3 -m pip install --upgrade jupyter
python3 -m jupyter notebook ../Samples/jupyter/Agricolture.ipynb
```

<!--//////////////////////////////////////////////////////////////////////// -->
## Linux compilation gcc

We are showing as an example how to build OpenVisus on Ubuntu 16.

Install prerequisites:

```
sudo apt install -y patchelf swig cmake
```

Install python3 (>=3.6):

```
sudo apt install -y python3 python3-dev python3-pip
python3 -m pip install numpy
```

(OPTIONAL) Install apache developer files if you want to enable Apache mod_visus:

```
sudo apt-get install -y libapr1 libapr1-dev libaprutil1 libaprutil1-dev apache2-dev
```

(OPTIONAL) Install qt5 if you want to compile the visus viewer (change repository and version as needed):

```
sudo add-apt-repository -y ppa:beineri/opt-qt-5.12.8-xenial
sudo apt update
sudo apt-get install -y qt512base qt512imageformats
```

Compile OpenVisus (change options -D as needed):

```
git clone https://github.com/sci-visus/OpenVisus
cd OpenVisus
mkdir build && cd build

cmake -DPython_EXECUTABLE=$(which python3) -DQt5_DIR=/opt/qt512/lib/cmake/Qt5 -DVISUS_GUI=0 _DVISUS_MODVISUS=0 ../
cmake --build ./ --target all     --config Release --parallel 4 
cmake --build ./ --target install --config Release
```

Test from command line:

```
# test embedding python
Release/OpenVisus/bin/visus

# test extending python
PYTHONPATH=$(pwd)/Release python3 -c "from OpenVisus import *"
```
If you want to test the viewer:

```
PYTHONPATH=$(pwd)/Release python3 -m OpenVisus configure --user # configure is needed to install PyQt5
PYTHONPATH=$(pwd)/Release python3 -m OpenVisus viewer
```


<!--//////////////////////////////d////////////////////////////////////////// -->
## Minimal compilation

Minimal compilation disable 

- Image support
- Network support
- Python supports

it enables only minimal IDX read/write operations.


For Windows/Visual Studio:

```
mkdir build && cd build
cmake -G "Visual Studio 16 2019" -A "x64" -DVISUS_MINIMAL=1 ../ 
cmake --build . --target ALL_BUILD --config Release
cmake --build . --target INSTALL   --config Release
```

For Windows/mingw

```
choco install -y mingw
set PATH=%PATH%;C:\ProgramData\chocolatey\lib\mingw\tools\install\mingw64\bin
mkdir build && cd build
cmake -G "MinGW Makefiles" -DVISUS_MINIMAL=1 ../ 
cmake --build . --target all       --config Release
cmake --build . --target install   --config Release
```

For Apple/Xcode

```
mkdir build  && cd build
cmake -GXcode -DVISUS_MINIMAL=1 ../
cmake --build ./ --target ALL_BUILD --config Release --parallel 4
cmake --build ./ --target install   --config Release
```

For Apple/gcc:

```

brew install gcc
export CC=cc-9
export CXX=g++-9
mkdir build && cd build
cmake -G"Unix Makefiles" -DVISUS_MINIMAL=1 ../
make -j 
make install
```


For Linux/gcc:

```
mkdir build && cd build
cmake -DVISUS_MINIMAL ../
make -j
make install
```


To use the VisusMinimal you can create a Makefile (change as needed):

```
CXX=g++-9 -std=c++11

OpenVisus_DIR=build/Release/OpenVisus

CXX_FLAGS=\
	-I$(OpenVisus_DIR)/include/Db \
	-I$(OpenVisus_DIR)/include/Kernel \
	-DVISUS_STATIC_=1 

main: main.o
	$(CXX) -o $@ $< -L${OpenVisus_DIR}/lib -lVisusMinimal
 
main.o: main.cpp 
	$(CXX) $(CXX_FLAGS) -c -o $@ $< 

clean:
	rm -f  main main.o

.PHONY: clean
```

If you don't want to use C++11 because you have an old compiler (like C++98) see Executable/use_minimal directory
which use a `Visus/Minimal.h` header.

<!--//////////////////////////////d////////////////////////////////////////// -->
## Static Win32 library compilation

Enable `VISUS_WIN32_STATIC_LIB` and `VISUS_NET` to build the core libraries (`VisusKernel.lib` and `VisusDb.lib`) and their dependencies statically (`/MTd` and `/MT` runtime library settings).
Make sure all the other options are disabled. Only Debug and Release builds are supported. After running the `INSTALL` target, both the headers
and binaries are in `build/Debug/dist` or `build/Release/dist`. Make sure to link with `VisusKernel.lib VisusDb.lib curl.lib crypto.lib ssl.lib` and also system libraries `Shell32.lib Ws2_32.lib advapi32.lib`.

To use the idx reader, include the `IdxDataset.h` as follows.

```
#undef min
#undef max
#define VISUS_STATIC_LIB 1
#include <Visus/IdxDataset.h>
```

<!--//////////////////////////////////////////////////////////////////////// -->
## Commit CI

For OpenVisus developers only:

```
TAG=$(python3 Libs/swig/setup.py new-tag) && echo ${TAG}
# replace manually the version in enviroment.yml if needed for binder
git commit -a -m "New tag" && git tag -a $TAG -m "$TAG" && git push origin $TAG && git push origin
```


