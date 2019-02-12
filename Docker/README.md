# //////////////////////////////////////////////////////////////////////
# How to build OpenVisus Docker container
#
# Note: for development with anaconda enabled, please see Docker/anaconda/README.md.
#

Compile and run the docker container. 
For example:

```
sudo docker build  -t openvisus-ubuntu Docker/ubuntu
sudo docker run   -it openvisus-ubuntu /bin/bash 
```

# //////////////////////////////////////////////////////////////////////
# For mod_visus 

Compile docker. For windows:

```
set VISUS_DATASETS=C:\projects\OpenVisus\datasets
set TAG=mod_visus-ubuntu
docker build  -t %TAG% Docker/%TAG%
docker run -it -v --name mydocker %VISUS_DATASETS%:/mnt/visus_datasets --expose=80 -p 8080:80 %TAG% "/usr/local/bin/httpd-foreground.sh"

```

For osx/linux:

```
VISUS_DATASETS=/path/to/datasets/dir
TAG=mod_visus-ubuntu
docker build  -t ${TAG} Docker/mod_visus-ubuntu
docker run -v ${VISUS_DATASETS}:/mnt/visus_datasets -p 80:80 -d ${TAG}
```

If you want to run interactively, use this version, which adds **-it** and removes **-d**:

```
docker run -it -v ${VISUS_DATASETS}:/mnt/visus_datasets -p 8080:80 ${TAG} /bin/bash
/usr/local/bin/httpd-foreground.sh
```

To test docker container, in another terminal:

```
curl  "http://0.0.0.0:8080/mod_visus?action=list"
```

Deploy to the repository:

```
sudo docker login -u scrgiorgio
# TYPE the secret <password>

docker tag $DOCKER_TAG visus/$DOCKER_TAG
docker push visus/$DOCKER_TAG
```

# //////////////////////////////////////////////////////////////////////
# Debug step-by-step build process

sudo docker run -it -v $(pwd):/home/OpenVisus --expose=80 -p 8080:80  --name manylinux quay.io/pypa/manylinux1_x86_64 /bin/bash^
cd /home/OpenVisus
export BUILD_DIR=/home/OpenVisus/build/manylinux
CMake/build_manylinux.sh

docker start manylinux
docker exec -it  manylinux /bin/bash
cd /home/OpenVisus
export BUILD_DIR=/home/OpenVisus/build/manylinux
CMake/build_manylinux.sh
