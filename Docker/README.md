# ////////////////////////////////////////////////////////////////  OpenVisus Docker Images

We provide examples of several different Docker images, both with and without Apache mod_visus.

## For an interactive container, just compile and run
For example:

```
sudo docker build  -t openvisus-ubuntu -f Dockerfile.ubuntu .
sudo docker run   -it openvisus-ubuntu /bin/bash 
```

#### In the running container, test the OpenVisus Python library:

```
python3 /home/OpenVisus/Samples/python/Idx.py
python3 /home/OpenVisus/Samples/python/Dataflow.py
python3 /home/OpenVisus/Samples/python/Array.py
```

## Host your own datasets using a mod_visus image

For Windows:
```
set VISUS_DATASETS=C:\path\to\datasets\dir
set TAG=mod_visus-ubuntu
docker build -t %TAG% -f Dockerfile.%TAG% .
docker run -d --name mydocker -v %VISUS_DATASETS%:/mnt/visus_datasets -p 8080:80 %TAG%
```

For osx/linux:
```
VISUS_DATASETS=/path/to/datasets/dir
TAG=mod_visus-ubuntu
docker build -t ${TAG} -f Dockerfile.${TAG} .
docker run -d --name mydocker -v ${VISUS_DATASETS}:/mnt/visus_datasets -p 8080:80 ${TAG}
```

If you want to run interactively, add **-it**, remove **-d**, and explicitly run /bin/bash:
```
docker run -it -v ${VISUS_DATASETS}:/mnt/visus_datasets -p 8080:80 ${TAG} /bin/bash
/usr/local/bin/httpd-foreground.sh
```

Notes:
* `-v <local_path>:<remote_path>` mounts local datasets directory and may be omitted to simply see it running
* `-p 8080:80` directs the server to run on host port 8080, but in the container it will appear to be port 80

#### To test the running container

- in another terminal, list datasets hosted by the container:

        curl  "http://0.0.0.0:8080/mod_visus?action=list"

- in a browser, open the webviewer: `http://0.0.0.0:8080/viewer.html`
    - change server name in viewer to: `http://0.0.0.0:8080/mod_visus?`

- in an interactive session you can also test the OpenVisus Python library (see above)

## Deploy to the repository

```
sudo docker login -u <username>
# TYPE the secret <password>

docker tag $TAG visus/$TAG
docker push visus/$TAG
```

## Debug step-by-step build process

From the OpenVisus code directory, create a container and try to build:
```
docker run -it -v $(pwd):/home/OpenVisus --expose=80 -p 8080:80 --name manylinux visus/travis-image /bin/bash -ic "cd /home/OpenVisus && export BUILD_DIR=/home/OpenVisus/build/manylinux && ./scripts/build.sh"
```

If it fails, start and connect to the container to debug:
```
docker start manylinux
docker exec -it manylinux /bin/bash
cd /home/OpenVisus
export BUILD_DIR=/home/OpenVisus/build/manylinux
./scripts/build.sh
```
