# ATLANTIS Docker mod_visus

Login to atlantis.

Run the Docker OpenVisus server:

```
cd /home/sci/scrgiorgio/atlantis-docker

# important the `:shared` for the NFS mounts otherwise you will get Docker "Too many levels of symbolic links" error message
# NOTE: -d is for `detach`
# NOTE: --restart should cover crashes, not sure about Atlantis reboot (TOCHECK!)
# NOTE: I am exposing either HTTP and HTTPS ports
CONTAINER_NAME=atlantis-docker-1
TAG=2.1.158
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

# (OPTIONAL) if you want to run httpd-foreground manually
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
   
   
# (OPTIONAL) if you want to enter in the container
sudo docker exec -it $CONTAINER_NAME /bin/bash

# (OPTIONAL) to inspect the logs
sudo docker logs --follow $CONTAINER_NAME
```


Test it from another terminal:

```
# HTTP
curl http://atlantis.sci.utah.edu/mod_visus?action=list


# HTTPS
curl https://atlantis.sci.utah.edu/mod_visus?action=list
```
 

(OPTIONAL) Example for developers. If you need to copy some files from a container:

```
sudo docker run --name temp visus/mod_visus:$TAG /bin/true
sudo docker cp temp:/usr/local/apache2/conf/extra/httpd-ssl.conf ./httpd-ssl.conf
sudo docker rm temp
sudo docker run --rm --mount type=bind,source=$PWD/httpd-ssl.conf,target=/tmp/httpd-ssl.conf ubuntu chown $UID /tmp/httpd-ssl.conf

```

# Generate new CA certificates

This is what I did:

```
# generate certificates
# see https://certbot.eff.org/docs/install.html#running-with-docker
sudo docker run -it --rm --name certbot \
   -v "$HOME/.certbot/etc/letsencrypt:/etc/letsencrypt" \
   -v "$HOME/.certbot/var/lib/letsencrypt:/var/lib/letsencrypt" \
    -p 80:80 \
    -p 443:443 \
   certbot/certbot certonly --standalone -m scrgiorgio@gmail.com -n --agree-tos -d atlantis.sci.utah.edu 

# fix file permissions
sudo docker run\
   -v "$HOME/.certbot/etc/letsencrypt:/etc/letsencrypt" \
   -v "$HOME/.certbot/var/lib/letsencrypt:/var/lib/letsencrypt" \
   ubuntu:latest chmod a+rX -R /etc/letsencrypt

```
TODO:
- (DONE) does not start Apache on atlantis
- find a way to renew (automatically?) certificates (THIS ONE EXPIRES 2021-11-17). Maybe using a docker compose? Probably with an nginx reverse proxy in front...
- ask Ali for sudo docker-compose
- what happens on atlantis-reboot? will openvisus Docker come up?
- ask Ali about visus.sci.utah.edu
- replace "it works" with real data