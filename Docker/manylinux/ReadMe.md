# How to create Docker images for Linux portable binaries

Run (under Windows CLI, to adapt for other OS):

```
export DOCKER_CLI_EXPERIMENTAL=enabled
docker login -u scrgiorgio
# type the password
sudo docker buildx create --name mybuilder --use || true
sudo docker buildx build --platform linux/amd64 -t visus/manylinux:1.0 -t visus/manylinux:latest --progress=plain -f Dockerfile.x86_64  ./ --push

docker buildx build --platform linux/arm64 -t visus/manylinux:1.0 -t visus/manylinux:latest --progress=plain -f Dockerfile.aarch64 ./ --push
```

docker run --platform linux/arm64 -it --rm quay.io/pypa/manylinux2014_aarch64 bash

# AARCH 64

```
docker run -it --rm  -v ${PWD}:${PWD} -w ${PWD} -e PYTHON_VERSION=3.8 -e VISUS_GUI=0 -e VISUS_SLAM=0 -e VISUS_MODVISUS=0 continuumio/miniconda3:latest bash -c '
conda create --name my-python -y python=${PYTHON_VERSION}
source $(conda info --base)/etc/profile.d/conda.sh
conda activate my-python
conda install -y numpy anaconda-client conda conda-build wheel   make cmake patchelf swig
conda install -y -c conda-forge cxx-compiler

if [[ "${VISUS_GUI}"=="1"]]; then
	conda install -c conda-forge -y qt==5.12.9
fi

Python_EXECUTABLE=`which python` BUILD_DIR=build_conda ./scripts/build_linux.sh

apt-get update
apt-get install -y vim
disable visus executable..
'


