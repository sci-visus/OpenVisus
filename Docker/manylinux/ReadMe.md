# How to create Docker images for Linux portable binaries

Run (under Windows CLI, to adapt for other OS):

```
export DOCKER_CLI_EXPERIMENTAL=enabled

docker login -u scrgiorgio
# type the password

cd Docker/manylinux
sudo docker buildx create --name mybuilder --use 2>/dev/null || true
```

For x86_64:

```
sudo docker buildx build --platform linux/amd64 -t visus/manylinux:1.0 -t visus/manylinux:latest --progress=plain -f Dockerfile.x86_64  ./ --push
```

For aarcgh4:

```
sudo docker buildx build --platform linux/arm64 -t visus/manylinux:1.0 -t visus/manylinux:latest --progress=plain -f Dockerfile.aarch64 ./ --push
```

To debug:

```
docker run --platform linux/arm64 -it --rm quay.io/pypa/manylinux2014_aarch64 bash
```

# AARCH 64

```
# CentOS 7 based (includes glibc 2.17)
# ldd --version ldd 
# docker run -it --rm  -v ${PWD}:${PWD} -w ${PWD} quay.io/pypa/manylinux2014_aarch64 bash 
# version `GLIBC_2.25' not found


export LC_ALL=en_US.UTF-8
export LANG=en_US.UTF-8
export LANGUAGE=en_US.UTF-8
export ARCHITECTURE=aarch64

# problem 
# curl -o https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-$ARCHITECTURE.sh
# bash ./Miniconda3-latest-Linux-aarch64.sh -b
# version `GLIBC_2.25' not found

curl –o https://repo.anaconda.com/archive/Anaconda3-2021.11-Linux-$ARCHITECTURE.sh

# ????
yum install -y anaconda


~/miniconda3/bin/conda init bash 
source ~/.bashrc 
conda update --all -y  
conda config  --set changeps1 no --set anaconda_upload no


export PYTHON_VERSION=3.8


conda create --name my-python -y python=${PYTHON_VERSION}
source $(conda info --base)/etc/profile.d/conda.sh
conda activate my-python
conda install -y numpy anaconda-client conda conda-build wheel make cmake patchelf swig

# not compatible (!)
# conda install -y -c conda-forge cxx-compiler

export VISUS_GUI=0
export VISUS_SLAM=0
export VISUS_MODVISUS=0
export VISUS_CLI=0

if [[ "${VISUS_GUI}"=="1"]]; then
	conda install -c conda-forge -y qt==5.12.9
fi

export Python_EXECUTABLE=$(which python)
export BUILD_DIR=build_docker
./scripts/build_linux.sh

apt-get update
apt-get install -y vim
disable visus executable

```

