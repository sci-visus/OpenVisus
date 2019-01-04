# //////////////////////////////////////////////////////////////////////
# OpenVisus Docker containers based on miniconda3 for building/testing and install
# Includes OpenVisus, XIDX, and webviewer (visus_javascript)
#

First, build and run the docker `builder` container (from code/OpenVisus):
```
docker build -t miniconda_builder Docker/miniconda/builder
docker start -it miniconda_builder
```

Once inside the running container, update, rebuild, and install the projects:
```
cd /root/code/OpenVisus/build
git pull
cmake ..
cmake --build . --target all -- -j16
cmake --build . --target test
cmake --build . --target install
```
```
cd ../../XIDX/build
git pull
cmake -DCMAKE_INSTALL_PREFIX=/root/code/OpenVisus/build/install ..
cmake --build . --target all -- -j16
cmake --build . --target test
cmake --build . --target install
```
```
cd ../../webviewer
git pull
```

Create an archive of the installation that can be copied into a fresh, lightweight install container:
```
tar zcf visus_miniconda_install.tgz -C /root/code/OpenVisus/build/install .
tar zcf visus_webviewer.tgz -C /root/code/webviewer webviewer
# XIDX is copied implicitly since it's installed to same folder as OpenVisus.
scp visus_miniconda_install.tgz visus_webviewer.tgz <username>@<system>:/tmp
exit
```

Next, build the docker installer container, which copies the new build, and start the container:
```
# from code/OpenVisus:
mv /tmp/visus_*.tgz .
docker build -t visus_miniconda_install -f Docker/miniconda/installer .
docker start -it visus_miniconda_install
```

If you have issues or need to customize the installation, log into the container using:
```
docker run -it visus_miniconda_install /bin/bash
```









#-------------------------------------------------------------------------------
#-------------------------------------------------------------------------------
#-------------------------------------------------------------------------------
#-------------------------------------------------------------------------------
#-------------------------------------------------------------------------------
#-------------------------------------------------------------------------------
#-------------------------------------------------------------------------------
#-------------------------------------------------------------------------------
# Old docs, may be necessary so leaving them here for now

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
docker run -it -v ${VISUS_DATASETS}:/mnt/visus_datasets --expose=80 -p 8080:80 ${TAG} "/usr/local/bin/httpd-foreground.sh"
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

docker run -it -v C:\projects\OpenVisus:/home/OpenVisus --expose=80 -p 8080:80  --name manylinux quay.io/pypa/manylinux1_x86_64 /bin/bash
cd /home/OpenVisus
export BUILD_DIR=/home/OpenVisus/build/manylinux
CMake/build_manylinux.sh

docker start manylinux
docker exec -it  manylinux /bin/bash
cd /home/OpenVisus
export BUILD_DIR=/home/OpenVisus/build/manylinux
CMake/build_manylinux.sh
