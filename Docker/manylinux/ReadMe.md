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
# CentOS 7 based (includes glibc 2.17)  `ldd --version`
# docker run -it --rm  -v ${PWD}:${PWD} -w ${PWD} --platform linux/arm64  quay.io/pypa/manylinux2014_aarch64 bash 

export LC_ALL=en_US.UTF-8
export LANG=en_US.UTF-8
export LANGUAGE=en_US.UTF-8
export ARCHITECTURE=aarch64

# anaconda and miniconda seems to have problems with aarm64
curl -L https://github.com/conda-forge/miniforge/releases/latest/download/Miniforge3-Linux-aarch64.sh -o install.sh
bash install.sh -b
rm -f install.sh
source /root/miniforge3/etc/profile.d/conda.sh
# conda update --all -y
conda config --set anaconda_upload no

export PYTHON_VERSION=3.8
conda create --name python-$PYTHON_VERSION -y python=${PYTHON_VERSION}
conda activate python-$PYTHON_VERSION
# conda install -y numpy anaconda-client conda-build wheel conda conda-build wheel cxx-compiler
# conda install -y qt==5.12.9

export VISUS_GUI=0
export VISUS_SLAM=0
export VISUS_MODVISUS=0
export VISUS_CLI=0
export Python_EXECUTABLE=$(which python)
export BUILD_DIR=build_conda
./scripts/build_linux.sh

```

