
Edit the visus.config in this directory to point to your datasets. For example:

```
DATASET_HOST_DIRECTORY=~/visus_datasets
DATASET_CONTAINER_DIRECTORY=/visus_datasets
cat <<EOF > ./visus.config
<?xml version="1.0" ?>
<visus>
  <dataset name='2kbit1' url='file://$DATASET_CONTAINER_DIRECTORY/2kbit1/visus.idx' permissions='public'/>
</visus>
EOF
```

Build Docker:

```
docker build -t mod_visus-opensuse .
```

Run Docker:

```

# allocate a tty for the container process.
DOCKER_OPTS="-it"

#automatically clean up the container and remove the file system when the container exits
DOCKER_OPTS+=" --rm"

# mount the volume
DOCKER_OPTS+=" -v $DATASET_HOST_DIRECTORY:$DATASET_CONTAINER_DIRECTORY"

# map network ports
DOCKER_OPTS+=" -p 8080:80"

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

