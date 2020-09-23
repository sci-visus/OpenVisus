

# Upload a new Docker mod_visus image (OPTIONAL Developers only)

Choose the tag you want to expose

```
# TAG=$(python3 Libs/swig/setup.py print-tag) && echo ${TAG}
TAG=2.1.93
```

Replace that tag in the Dockerfile and build the new Docker image:

```
cd Docker/mod_visus/httpd
sudo docker build --tag visus/mod_visus:$TAG  .
```

Eventually debug it:

```
DATASETS=/mnt/c/projects/OpenVisus/datasets
sudo docker run -it --entrypoint /bin/bash -w=/home/OpenVisus -v $DATASETS:/datasets visus/mod_visus:$TAG
httpd-foreground
exit
```

Eventually push it to docker hub (change username as needed):

```
sudo docker login --username=scrgiorgio 
sudo docker push visus/mod_visus:$TAG
```

