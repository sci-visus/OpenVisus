# Instructions

## Use precompiled openvisus conda packages 

From a shell:

```
conda install -c visus openvisus
```


## Build conda openvisus recipe

### Build conda openvisus on Windows

[BROKEN right now]

### Build conda openvisus using Docker

Type:

```
cd conda/openvisus
sudo docker build .
```

### Build OpenVisus on Linux/OSX

In a shell:

```
# uncomment if needed
# rm -Rf $(find ${HOME}/miniconda${PYTHON_VERSION:0:1} -iname "*openvisus*"

# change as needed
export PYTHON_VERSION=3.6 
export USE_CONDA=1
./build.sh
```

In case you want to upload to conda rep:

```
anaconda login
CONDA_BUILD_FILENAME=$(find ${HOME}/miniconda${PYTHON_VERSION:0:1}/conda-bld -iname "openvisus*.tar.bz2")
anaconda upload ${CONDA_BUILD_FILENAME}
```
