# //////////////////////////////////////////////////////////////////////
# OpenVisus Docker containers based on continuumio/miniconda3 for
# building/testing and installing OpenVisus, XIDX, and webviewer.
#

=======
BUILDER
=======

First, build and run the docker `builder` container (from code/OpenVisus):
```
docker build -t visus_anaconda_builder -f Docker/anaconda/builder/Dockerfile .
docker start -it visus_anaconda_builder
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

Create an archive of the installation that can be copied into a fresh, lightweight install container (XIDX is with OpenVisus):
```
tar zcf visus_anaconda_install.tgz -C /root/code/OpenVisus/build/install .
tar zcf visus_webviewer.tgz -C /root/code/webviewer webviewer
scp visus_anaconda_install.tgz visus_webviewer.tgz <username>@<system>:/tmp
exit
```

=======
INSTALL
=======

Next, build the docker installer container, which copies the new build:
```
# from code/OpenVisus:
mv /tmp/visus_*.tgz .
docker build -t visus_anaconda_install -f Docker/anaconda/installer/Dockerfile .
```

Finally, start the container (maps port 80 from the container to 8080 on the host):
```
docker run -p 8080:80 visus_anaconda_install
```

By default, the apache server is started. If you have issues or want to customize anything, start using:
```
docker run -it visus_anaconda_install /bin/bash
```









#-------------------------------------------------------------------------------
#-------------------------------------------------------------------------------
#-------------------------------------------------------------------------------
#-------------------------------------------------------------------------------
#-------------------------------------------------------------------------------
#-------------------------------------------------------------------------------
#-------------------------------------------------------------------------------
#-------------------------------------------------------------------------------
# Old docs, some integration is necessary so leaving them here for now

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
