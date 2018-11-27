# mod_visus container

Compile docker. For windows:

```
set VISUS_DATASETS=C:\projects\OpenVisus\datasets
set TAG=modvisus-ubuntu
docker build  -t %TAG% Docker/mod_visus/ubuntu
docker run -it -v %VISUS_DATASETS%:/mnt/visus_datasets --expose=80 -p 8080:80 %TAG% "/usr/local/bin/httpd-foreground.sh"

```

For osx/linux:

```
VISUS_DATASETS=/path/to/datasets/dir
TAG=modvisus-ubuntu
docker build  -t ${TAG} Docker/mod_visus/ubuntu
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