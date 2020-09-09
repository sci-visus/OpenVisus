# Use mod_visus Docker image:

```

# change as needed, a directory with OpenVisus datasets and a datasets.conf file
DATASETS=/mnt/d/GoogleSci/visus_dataset

# add -d to detach
sudo docker run  -v $DATASETS:/datasets --publish 8080:80 visus/mod_visus

wget -q -O -  "http://localhost:8080/index.html"
wget -q -O -  "http://localhost:8080/server-status"
wget -q -O -  "http://localhost:8080/viewer/index.html"
wget -q -O -  "http://localhost:8080/mod_visus?action=list"
```

Eventually inspect logs:

```
sudo docker logs <container_id>
```


# (developer only) Build it:

```
cd Docker/mod_visus/httpd
sudo docker build --tag visus/mod_visus  .
```

Eventually debug it:

```
sudo docker run -it --entrypoint /bin/bash -w=/home/OpenVisus -v $DATASETS:/datasets visus/mod_visus
httpd-foreground
```

Eventually push it to docker hub (change username as needed):

```
sudo docker login --username=scrgiorgio 
sudo docker push visus/mod_visusx
```

