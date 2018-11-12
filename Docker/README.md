# Build/Run the Docker container

Compile and run docker:

```
# change the name as you need
DOCKER_TAG=openvisus-trusty
cp ../../CMake/build/* . . && \
sudo docker build --tag $DOCKER_TAG ./
sudo docker run $DOCKER_OPTS $DOCKER_TAG /bin/bash
```

Note that to debug you can:

	- comment the last RUN docker line
	- compile again
	- `sudo docker run --rm -it $DOCKER_TAG /bin/bash`
	- type `alias ARG=` in the terminal
	- copy&paste all the Docker part with ARG (they are not passed as ENV)
	- execute the `./<build-filename>.sh` command in the terminal

To copy files from a container:

``
sudo docker run --name temp-$DOCKER_TAG $DOCKER_TAG /bin/true
sudo docker cp         temp-$DOCKER_TAG:./build/install/dist ./
sudo docker rm         temp-$DOCKER_TAG
```


# Build/Run the mod_visus container

Compile docker:

```
DOCKER_TAG=mod_visus-trusty
cp ../../CMake/build/* . && \
  sudo docker build  \
    --tag $DOCKER_TAG  \
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
