# How to add a dataset

Copy the data to atlantis:

```
export ATLANTIS=scrgiorgio@atlantis.sci.utah.edu
scp -r arecibo1 $ATLANTIS:/usr/sci/cedmav/data
```

Login into atlantis, fix permissions,add the new dataset and run the new server:

```
ssh $ATLANTIS

cd /home/sci/scrgiorgio/atlantis-docker
chmod -R a+rX /usr/sci/cedmav/data/arecibo1

# add this line to `datasets.config
#     <dataset name="arecibo1" url="$(visus_datasets)/data/arecibo1/visus.idx" />
vi datasets.config

# EVENTUALLY edit `run.background.sh` file and change the tag to the latest one
./run.background.sh
```

Test it:

```
curl https://atlantis.sci.utah.edu/mod_visus?action=list
visusviewer https://atlantis.sci.utah.edu/mod_visus?dataset=arecibo1
```

# Preamble:

Run in the console:

```
cd /home/sci/scrgiorgio/atlantis-docker

CONTAINER_NAME=atlantis-docker-1

# !!! change as needed !!!!
# see https://hub.docker.com/r/visus/mod_visus/tags
TAG=2.1.166
```

# Run mod_visus


Run the Docker OpenVisus server. 

```
sudo docker stop $CONTAINER_NAME || true
sudo docker rm   $CONTAINER_NAME || true
sudo docker run \
   --publish 80:80  \
   --publish 443:443 \
   --mount type=bind,source=$PWD/datasets.config,target=/datasets/datasets.config \
   --mount type=bind,source=$PWD/config.js,target=/home/OpenVisus/webviewer/config.js \
   --mount type=bind,source=$PWD/certbot/etc/letsencrypt/live/atlantis.sci.utah.edu/fullchain.pem,target=/usr/local/apache2/conf/server.crt \
   --mount type=bind,source=$PWD/certbot/etc/letsencrypt/live/atlantis.sci.utah.edu/privkey.pem,target=/usr/local/apache2/conf/server.key \
    -v /usr/sci/cedmav:/usr/sci/cedmav:shared \
    -v /usr/sci/brain:/usr/sci/brain:shared   \
   --restart unless-stopped --name $CONTAINER_NAME -d visus/mod_visus:$TAG
```

Please note:
- `:shared`   for the NFS mounts otherwise you will get Docker "Too many levels of symbolic links" error message
- `-d`        is for `detach`
- `--restart` should cover crashes, not sure about Atlantis reboot


# Test mod_visus

Using HTTP:

```
curl http://atlantis.sci.utah.edu/mod_visus?action=list
```

Using HTTPS:

```
curl https://atlantis.sci.utah.edu/mod_visus?action=list
```
 
If you need to copy some files from a container:

```
sudo docker run --name temp visus/mod_visus:$TAG /bin/true
sudo docker cp temp:/usr/local/apache2/conf/extra/httpd-ssl.conf ./httpd-ssl.conf
sudo docker rm temp
sudo docker run --rm --mount type=bind,source=$PWD/httpd-ssl.conf,target=/tmp/httpd-ssl.conf ubuntu chown $UID /tmp/httpd-ssl.conf
```


# Debug mod_visus

if you want to run httpd-foreground manually:

```
sudo docker run \
   --publish 80:80  \
   --publish 443:443 \
   --mount type=bind,source=$PWD/datasets.config,target=/datasets/datasets.config \
   --mount type=bind,source=$PWD/config.js,target=/home/OpenVisus/webviewer/config.js \
   --mount type=bind,source=$PWD/certbot/etc/letsencrypt/live/atlantis.sci.utah.edu/fullchain.pem,target=/usr/local/apache2/conf/server.crt \
   --mount type=bind,source=$PWD/certbot/etc/letsencrypt/live/atlantis.sci.utah.edu/privkey.pem,target=/usr/local/apache2/conf/server.key \
    -v /usr/sci/cedmav:/usr/sci/cedmav:shared \
    -v /usr/sci/brain:/usr/sci/brain:shared   \
   --rm -it visus/mod_visus:$TAG /bin/bash
```
   
If you want to enter in the container:

```
sudo docker exec -it $CONTAINER_NAME /bin/bash
```

To inspect the logs:

```
sudo docker logs --follow $CONTAINER_NAME
```


# Generate/Renew CA certificates

See https://letsencrypt.org/docs/integration-guide/


```
# copy old certificates just to be sure
mkdir -p certbot.backup
cp -r certbot certbot.backup/$(date +"%m_%d_%Y")

# I need port 80 and 443 to stop
sudo docker stop $CONTAINER_NAME || true
sudo docker rm   $CONTAINER_NAME || true

# see https://certbot.eff.org/docs/install.html#running-with-docker
sudo docker run -it --rm --name certbot \
   -v "$PWD/certbot/etc/letsencrypt:/etc/letsencrypt" \
   -v "$PWD/certbot/var/lib/letsencrypt:/var/lib/letsencrypt" \
    -p 80:80 \
    -p 443:443 \
   certbot/certbot certonly --standalone -m scrgiorgio@gmail.com -n --agree-tos -d atlantis.sci.utah.edu 

# fix file permissions
sudo docker run\
   -v "$PWD/certbot/etc/letsencrypt:/etc/letsencrypt" \
   -v "$PWD/certbot/var/lib/letsencrypt:/var/lib/letsencrypt" \
   ubuntu:latest chmod a+rX -R /etc/letsencrypt

```

Note than during the `Apache` run (previous section) the `$PWD/certbot` directory is mounted to the right Apache location.


Now run OpenVisus again (see above).
