---
layout: default
title: Atlantis Sci Utah Edu
parent: Old Docs
nav_order: 2
---

# Table of contents
{: .no_toc .text-delta }

1. TOC
{:toc}

---

# Add a dataset

Copy the data to atlantis:

```bash
export ATLANTIS=scrgiorgio@atlantis.sci.utah.edu
scp -r arecibo1 $ATLANTIS:/usr/sci/cedmav/data
```

Login into atlantis, fix permissions,add the new dataset and run the new server:

```bash
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

```bash
curl https://atlantis.sci.utah.edu/mod_visus?action=list
visusviewer https://atlantis.sci.utah.edu/mod_visus?dataset=arecibo1
```


# run.background.sh

```bash

CONTAINER_NAME=atlantis-docker-1

# https://hub.docker.com/r/visus/mod_visus_x86_64/tags
IMAGE_NAME=visus/mod_visus_x86_64:2.1.223

# make a copy of the certificates
__date__=$(date +"%m_%d_%Y")
mkdir -p ./backup/certbot/${__date__}
cp -r certbot backup/certbot/${__date__}

# stop mod_visus
sudo docker stop $CONTAINER_NAME || true
sudo docker rm   $CONTAINER_NAME || true

# if you have problems..
RENEW_CERTIFICATE=${RENEW_CERTIFICATE:-0}
if [[ "$RENEW_CERTIFICATE" == "1" ]] ; then

  echo "Renew certificates"
  sudo docker run -it --rm --name certbot \
     -v "$PWD/certbot/etc/letsencrypt:/etc/letsencrypt" \
     -v "$PWD/certbot/var/lib/letsencrypt:/var/lib/letsencrypt" \
      -p 80:80 \
      -p 443:443 \
     certbot/certbot certonly --standalone -m scrgiorgio@gmail.com -n --agree-tos -d atlantis.sci.utah.edu 

  echo "Fix certificate permissions"
  sudo docker run \
     -v "$PWD/certbot/etc/letsencrypt:/etc/letsencrypt" \
     -v "$PWD/certbot/var/lib/letsencrypt:/var/lib/letsencrypt" \
     ubuntu:latest chmod a+rX -R /etc/letsencrypt
fi

# restart mod_visus
echo "Restarting mod_visus"

# Please NOTE
# - `:shared`   for the NFS mounts otherwise you will get Docker "Too many levels of symbolic links" error message
# - `-d`        is for `detach`
# `--restart` should cover crashes, not sure about Atlantis reboot
# to run in foreground mode replace last 3 lines with `--rm -it $IMAGE_NAME /bin/bash`
sudo docker run \
   --publish 80:80  \
   --publish 443:443 \
   --mount type=bind,source=$PWD/datasets.config,target=/datasets/datasets.config \
   --mount type=bind,source=$PWD/config.js,target=/home/OpenVisus/webviewer/config.js \
   --mount type=bind,source=$PWD/certbot/etc/letsencrypt/live/atlantis.sci.utah.edu/fullchain.pem,target=/usr/local/apache2/conf/server.crt \
   --mount type=bind,source=$PWD/certbot/etc/letsencrypt/live/atlantis.sci.utah.edu/privkey.pem,target=/usr/local/apache2/conf/server.key \
    -v /usr/sci/cedmav:/usr/sci/cedmav:shared \
    -v /usr/sci/brain:/usr/sci/brain:shared   \
   --restart unless-stopped \
   --name $CONTAINER_NAME \
   -d $IMAGE_NAME

echo "Restart done"
```

To test test:

```bash
# HTTP test
curl http://atlantis.sci.utah.edu/mod_visus?action=list

# HTTPS test
curl https://atlantis.sci.utah.edu/mod_visus?action=list

# If you want to enter in the container:
sudo docker exec -it $CONTAINER_NAME /bin/bash
```

# To inspect the logs:
sudo docker logs --follow $CONTAINER_NAME

# If you need to copy some files from a container:
sudo docker run --name temp visus/mod_visus:$TAG /bin/true
sudo docker cp temp:/usr/local/apache2/conf/extra/httpd-ssl.conf ./httpd-ssl.conf
sudo docker rm temp
sudo docker run --rm --mount type=bind,source=$PWD/httpd-ssl.conf,target=/tmp/httpd-ssl.conf ubuntu chown $UID /tmp/httpd-ssl.conf
```
