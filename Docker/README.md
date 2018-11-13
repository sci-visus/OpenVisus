# How to build OpenVisus Docker container

Compile and run the docker container. For example, for the `trusty` container:

```
sudo docker build -t openvisus-trusty --build-arg BRANCH=master Docker/trusty
sudo docker run  -it openvisus-trusty /bin/bash 
```

# How to debug the building process

Run the script interactively:

```
sudo docker run -it -v c:\projects\OpenVisus:/home/OpenVisus ubuntu:trusty 
cd /home/OpenVisus
CMake/build_<your_distrib_name>.sh
exit
```

To re-enter in an exited image:

```
docker ps -a
CONTAINER_ID=f3399626bda1 
docker start $CONTAINER_ID 
sudo docker exec -ti $CONTAINER_ID /bin/bash
```


# Build/Run the mod_visus container

Compile docker:

```
DOCKER_TAG=mod_visus-trusty
BRANCH=master

sudo docker build  \
  --tag $DOCKER_TAG  \
  --build-arg BRANCH=${BRANCH} \
  --build-arg DISABLE_OPENMP=0 \
  --build-arg VISUS_GUI=0 \
  --build-arg VISUS_MODVISUS=1 \
  ./
```

Configure datasets and run docker:

```
# change this to point to where your visus datasets are stored
VISUS_DATASETS=$(pwd)/../../datasets

DOCKER_OPTS=""
DOCKER_OPTS+=" -it"  # allocate a tty for the container process.
DOCKER_OPTS+=" --rm" #automatically clean up the container and remove the file system when the container exits
DOCKER_OPTS+=" -v $VISUS_DATASETS:/mnt/visus_datasets" # mount the volume
DOCKER_OPTS+=" --expose=80 -p 8080:80" # expose the port and remap
docker run $DOCKER_OPTS $DOCKER_TAG "/usr/local/bin/httpd-foreground.sh"

# docker run $DOCKER_OPTS --entrypoint=/bin/bash $DOCKER_TAG
# /usr/local/bin/httpd-foreground.sh
```

To test docker container, in another terminal:

```
curl -v "http://0.0.0.0:8080/mod_visus?action=list"
```

Deploy to the repository:

```
sudo docker login -u scrgiorgio
# TYPE the secret <password>

docker tag $DOCKER_TAG visus/$DOCKER_TAG
docker push visus/$DOCKER_TAG
```
