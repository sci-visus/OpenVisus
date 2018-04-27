Before starting
-----------------------------------

Open a docker shell and make sure you are logged:

```
docker login
Username: visus
Password: XXXX
```

Example of using precompiled mod_visus
--------------------------------------

Create 'server.config' file (replace 2kbit1 with your dataset name):
```
cat <<EOF > server.config
<?xml version="1.0" ?>
<visus>
  <dataset name='2kbit1' url='file:///home/visus/dataset/2kbit1/visus.idx' permissions='public'/>
</visus>
EOF
```

Create Dockerfile:
```
cat <<EOF > Dockerfile
FROM visus/mod_visus
COPY server.config /home/visus/server.config
EOF
```

Build mod_visus docker image:
```
docker build -t visus/my-server .
```

Run the image mounting a host volume:
```
docker run -it --rm -p 8080:80 --volume="c:/visus_dataset/2kbit1:/home/visus/dataset/2kbit1" visus/my-server 
```

Test if it works:
```
wget "http://localhost:8080/mod_visus?action=readdataset&dataset=2kbit1"
```


Example of creating a test david_subsampled server
--------------------------------------

Create 'server.config' file:
```
cat <<EOF > server.config
<?xml version="1.0" ?>
<visus>
  <dataset name='david_subsampled' url='file:///home/visus/dataset/david_subsampled/visus.idx' permissions='public'/>
</visus>
EOF
```

Create Dockerfile:
```
cat <<EOF > Dockerfile
FROM visus/mod_visus
COPY server.config /home/visus/server.config
RUN set -x \
  && apt update \
  && apt install -y curl \
  && cd /home/visus/dataset \
  && curl http://atlantis.sci.utah.edu/download/david_subsampled.tar.gz -o temp.tar.gz \
  && tar xvzf temp.tar.gz \
  && rm -f temp.tar.gz \
  && chown -R www-data /home/visus/dataset \
  && chmod -R a+rX  /home/visus/dataset  
EOF
```

Build mod_visus docker image:
```
docker build -t visus/david_subsampled .
```

Run the image mounting a host volume:
```
docker run -it --rm -p 8080:80 visus/david_subsampled 
```

Test if it works:
```
wget "http://localhost:8080/mod_visus?action=readdataset&dataset=david_subsampled"
```



Some tricks
------------------------------------

To debug mod_visus (example: to check Apache log files):
```
docker run -ti -p 8080:80 --entrypoint=/bin/bash visus/mod_visus -s
/usr/local/bin/httpd-foreground.sh
```

To run an interactive 'clean' ubuntu (or whatever):
```
docker run -ti -p 8080:80 --entrypoint=/bin/bash ubuntu:16.04 -s
```

If you want to remove all old docker images and containers (and you know what you are doing!)
```
docker rm  -f $(docker ps  -a -q)
docker rmi -f $(docker images -q)
```

