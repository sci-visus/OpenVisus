# Prerequisites

Edit the visus.config in this directory to point to your datasets. For example:

```
# change this to point to where your visus datasets are stored
VISUS_DATASETS=~/visus_datasets

cat <<EOF > $VISUS_DATASETS/visus.config
<visus>
  <dataset name='2kbit1' url='file:///visus_datasets/2kbit1/visus.idx' permissions='public'/>
</visus>
EOF
```

Setup docker run options:

```
# allocate a tty for the container process.
DOCKER_OPTS="-it"

#automatically clean up the container and remove the file system when the container exits
DOCKER_OPTS+=" --rm"

# mount the volume
DOCKER_OPTS+=" -v $VISUS_DATASETS:/visus_datasets"

# map network ports
DOCKER_OPTS+=" -p 8080:80"
```


# Use mod_visus-opensuse container

Run Docker

```
docker run $DOCKER_OPTS visus/mod_visus-opensuse 
```


# Build mod_visus-opensuse and deploy


Build Docker:

```
docker build -t mod_visus-opensuse .
```

Run Docker:

```

docker run $DOCKER_OPTS mod_visus-opensuse 
```

If you want to debug the docker container:

```
docker run $DOCKER_OPTS --entrypoint=/bin/bash mod_visus-opensuse
/usr/local/bin/httpd-foreground.sh
```

To test docker container, in another terminal:

```
curl -v "http://0.0.0.0:8080/mod_visus?action=list"
```

Deploy to the repository:

```
sudo docker login -u scrgiorgio
# TYPE the usual password

docker tag mod_visus-opensuse visus/mod_visus-opensuse
docker push visus/mod_visus-opensuse
```
