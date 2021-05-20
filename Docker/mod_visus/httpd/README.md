# Build Docker mod_visus image 

Choose the tag you want to expose (note: better to use PyPi tags)

```
cd Docker/mod_visus/httpd
TAG=YOUR_TAG_HERE
```

Build the new image:

```
sudo docker build --tag visus/mod_visus:$TAG  --build-arg TAG=$TAG .
```

If you want push to Docker hub:

```
sudo docker push visus/mod_visus:$TAG
```


# Use  mod_visus docker image

Pull the image from the repository, if needed:

```
TAG=YOUR_TAG_HERE
sudo docker pull visus/mod_visus:$TAG
```

Specify where all your datasets are:

```
DATASETS=/path/to/your/dataset
```

Create a `datasets.config` file inside with path relative to the container path `/datasets/`.
For example:

```
cat <<EOF > $DATASETS/datasets.config
<visus>
  <datasets>
    <dataset name="cat_gray" url="/datasets/cat/gray.idx" />
  </datasets>
</visus>
EOF
```

It's important that the container have at least read access to the directory.
You can both make the directory readable to anyone:

```
chmod -R a+rx $DATASETS
```

or add the Apache user (`daemon`) to be in the group of the owners. To do so run inside the container:

```
GID=$(stat -c "%g" $DATASETS)
addgroup --gid $GID my_new_group 
usermod -a -G $GID daemon
```


If you want to protect your datasets with a password you can do (replace `username` and `password`):

```
# sudo apt install apache2-utils
htpasswd -nb  username password > $DATASETS/.htpasswd
chmod 644 $DATASETS/.htpasswd
```

To run the container:

``` 
docker run --rm --publish 8080:80 --publish 443:443  -v $DATASETS:/datasets visus/mod_visus:$TAG
```

If it crashes you can debug from the inside doing

```
docker run --rm -it --publish 8080:80 --publish 443:443 -v $DATASETS:/datasets visus/mod_visus:$TAG /bin/bash
/usr/local/bin/httpd-foreground
```

Check if it works using HTTP (add `-u username:password` if needed):

```
curl http://localhost:8080
curl http://localhost:8080/server-status
curl http://localhost:8080/mod_visus?action=list
curl http://localhost:8080/mod_visus?dataset=cat_gray
```


Check if it works using HTTPS (add `-u username:password` if needed):

```
curl --insecure https://localhost:443
curl --insecure https://localhost:443/server-status
curl --insecure https://localhost:443/mod_visus?action=list
curl --insecure https://localhost:443/mod_visus?dataset=cat_gray

```

Some useful options for productions:

```
OPTIONS=--restart=always -d -name my-modvisus 
sudo systemctl enable docker

# to debug the logs
sudo docker ps  | grep mod_visus
sudo docker logs my-modvisus

````


# If you want to do load balancing using Docker Swarm

If you want to run multiple mod_visus with a ngix load balancer.

Create a `docker-compose.yml` file (change as needed):

```
cat <<EOF > docker-compose.yml 
worker1:
  image: visus/mod_visus:$TAG 
  restart: always
  volumes:
  - $DATASETS:/datasets
  ports:
  - "80"

worker2:
  image: visus/mod_visus:$TAG
  restart: always
  volumes:
  - $DATASETS:/datasets
  ports:
  - "80"

worker3:
  image: visus/mod_visus:$TAG
  restart: always
  volumes:
  - $DATASETS:/datasets
  ports:
  - "80"
      
nginx:
  image: nginx
  restart: always
  volumes:
  - ./nginx.conf:/etc/nginx/nginx.conf
  links:
  - worker1:worker1
  - worker2:worker2
  - worker3:worker3
  ports:
  - "8080:8080"
EOF
```

Create a `nginx.conf` file:

```
cat <<EOF > nginx.conf
events {
  worker_connections  4096;
}

http {
  upstream compiler {
    least_conn;
    server worker1:80;
    server worker2:80;
    server worker3:80;
  }

  server {
    listen 8080;
    location / {
	    proxy_pass http://compiler;
	    proxy_http_version 1.1;
	    proxy_set_header Upgrade $http_upgrade;
	    proxy_set_header Connection 'upgrade';
	    proxy_set_header Host $host;
	    proxy_cache_bypass $http_upgrade;
   }
  }
}
EOF
```

Then run the following command (add `-d` if you want to detach):

```
sudo docker-compose up -d
```








