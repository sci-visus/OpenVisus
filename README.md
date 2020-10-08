# OpenViSUS Visualization project  
  
![GitHub Actions](https://github.com/sci-visus/OpenVisus/workflows/BuildOpenVisus/badge.svg)
[![Binder](https://mybinder.org/badge_logo.svg)](https://mybinder.org/v2/gh/sci-visus/OpenVisus/master?filepath=Samples%2Fjupyter)

 
The mission of ViSUS.org is to provide support for the scientific community with Big Data, management, analysis and visualization tools.
In this website we provide access to open source software tools and libraries such as the ViSUS framework and the PIDX library.
These softwares are distributed under the permissive BSD license.

Table of content:

- [Binary Distribution](#binary-distribution)

- [mod_visus Docker Image](#mod_visus-docker-image)

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


<!--//////////////////////////////////////////////////////////////////////// -->
# mod_visus Docker Image

```

# your dataset directory, it must contain a `datasets.conf` file (and maybe contains an `.htpasswd` file)
sudo docker pull visus/mod_visus

# restart policy
#   IMPPORTANT: you must enable docker service on boot: `sudo systemctl enable docker`
#   no              Do not automatically restart the container. (the default)
#   on-failure      Restart the container if it exits due to an error, which manifests as a non-zero exit code.
#   always          Always restart the container if it stops. If it is manually stopped, it is restarted only when Docker daemon restarts or the container itself is manually restarted. (See the second bullet listed in restart policy details)
#   unless-stopped  Similar to always, except that when the container is stopped (manually or otherwise), it is not restarted even after Docker daemon restarts.


NAME=your_name_here

# for short living containers
# OPTS=--rm --restart=no

# for long living auto-restarting containers (even after a reboot):
#   By design, containers started in detached mode (-d) exit when the root process used to run the container exits
#   unless you also specify the --rm option

OPTS=-d --restart=always

# the directory containning a datasets.conf file
DATASETS=/mnt/c/projects/OpenVisus/datasets

sudo docker run  --name $NAME  $OPTS -v $DATASETS:/datasets --publish 8080:80 \visus/mod_visus:latest 

# test it
wget -q -O -  "http://localhost:8080/index.html"
wget -q -O -  "http://localhost:8080/server-status"
wget -q -O -  "http://localhost:8080/viewer/index.html"
wget -q -O -  "http://localhost:8080/mod_visus?action=list"
```

Eventually inspect logs:

```
sudo docker ps  | grep mod_visus
sudo docker logs $NAME
```

If you want to run multiple mod_visus with a ngix load balancer:

```
cd Docker/mod_visus/load_balancing

# ... do all your customization (-d if you want to detach)....
sudo docker-compose up -d
```


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
python3 -m OpenVisus viewer1
python3 -m OpenVisus viewer2

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
sudo apt install -y patchelf swig
```

Install a recent cmake. For example:

```
wget https://github.com/Kitware/CMake/releases/download/v3.17.2/cmake-3.17.2-Linux-x86_64.sh
sudo mkdir /opt/cmake
sudo sh cmake-3.17.2-Linux-x86_64.sh --skip-license --prefix=/opt/cmake
sudo ln -s /opt/cmake/bin/cmake /usr/bin/cmake
```

Install python (choose the version you prefer, here we are assuming 3.7):

```
sudo apt update
sudo apt install -y software-properties-common
sudo add-apt-repository ppa:deadsnakes/ppa
sudo apt update
sudo apt install -y python3.7 python3.7-dev python3-pip
python3.7 -m pip install numpy
```

Install apache developer files (OPTIONAL for mod_visus):

```
sudo apt-get install -y libapr1 libapr1-dev libaprutil1 libaprutil1-dev apache2-dev
```

Install qt5 (5.12 or another version):

```
#sudo add-apt-repository -y ppa:beineri/opt-qt592-xenial
#sudo apt update
#sudo apt-get install -y qt59base qt59imageformats

sudo add-apt-repository -y ppa:beineri/opt-qt-5.12.8-xenial
sudo apt update
sudo apt-get install -y qt512base qt512imageformats
```


Compile OpenVisus (change as needed):

```
git clone https://github.com/sci-visus/OpenVisus
cd OpenVisus

export Python_EXECUTABLE=python3.7
export Qt5_DIR=/opt/qt512/lib/cmake/Qt5 
alias python=${Python_EXECUTABLE}

mkdir build 
cd build

cmake -DPython_EXECUTABLE=${Python_EXECUTABLE} -DQt5_DIR=${Qt5_DIR} ../
cmake --build ./ --target all     --config Release --parallel 4 
cmake --build ./ --target install --config Release

python -m pip install --upgrade pip

export PYTHONPATH=./Release
python -m OpenVisus configure --user
python -m OpenVisus viewer
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
	-DVISUS_STATIC_KERNEL_LIB=1 \
	-DVISUS_STATIC_DB_LIB=1

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

<!--//////////////////////////////////////////////////////////////////////// -->
## Commit CI

For OpenVisus developers only:

```
TAG=$(python3 Libs/swig/setup.py new-tag) && echo ${TAG}
git commit -a -m "New tag" && git tag -a $TAG -m "$TAG" && git push origin $TAG && git push origin
```


