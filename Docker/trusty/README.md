# Simple container

Compile docker:

```
DOCKER_TAG=openvisus-trusty
sudo docker build -t $DOCKER_TAG Docker/trusty
```

To run interactively step by step

```
sudo docker run -i -t ubuntu:trusty /bin/bash
```
 
To debug errors:

```
sudo docker run --rm -it $DOCKER_TAG /bin/bash -il 
```


# Mod visus

Compile docker:

```
DOCKER_TAG=mod_visus-trusty
docker build -t $DOCKER_TAG .
```

Run docker:

```
# change this to point to where your visus datasets are stored
VISUS_DATASETS=~/visus_datasets
cat <<EOF > $VISUS_DATASETS/visus.config
<visus>
  <dataset name='2kbit1' url='file:///visus_datasets/2kbit1/visus.idx' permissions='public'/>
</visus>
EOF

DOCKER_OPTS="-it" # allocate a tty for the container process.
DOCKER_OPTS+=" --rm" #automatically clean up the container and remove the file system when the container exits
DOCKER_OPTS+=" -v $VISUS_DATASETS:/mnt/visus_datasets" # mount the volume
DOCKER_OPTS+=" -p 8080:80" # map network ports
DOCKER_OPTS+=" --expose=80" # expose the port

docker run $DOCKER_OPTS visus/$DOCKER_TAG "/usr/local/bin/httpd-foreground.sh"

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
