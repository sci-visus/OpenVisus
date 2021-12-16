# Load balancing with Docker Swarm

In this tutorial we show how to run multiple mod_visus with a ngix load balancer.

![Diagram](https://github.com/sci-visus/images/blob/main/Load-balancing-docker-swarm.png?raw=true)

Create a `docker-compose.yml` file with 3 mod_visus_workers. Replace $DATASETS with the directory where you have 

- (1) OpenVisus datasets and 
- (2) `datasets.config` file as exmplained in `docker_modvisus.md` tutorial. Also replace `$OPENVISUS_TAG` with the proper value.

```
worker1:
  image: visus/mod_visus:$OPENVISUS_TAG 
  restart: unless-stopped
  volumes:
  - $DATASETS:/datasets
  ports:
  - "8080:80"

worker2:
  image: visus/mod_visus:$OPENVISUS_TAG
  restart: unless-stopped
  volumes:
  - $DATASETS:/datasets
  ports:
  - "8080:80"

worker3:
  image: visus/mod_visus:$OPENVISUS_TAG
  restart: unless-stopped
  volumes:
  - $DATASETS:/datasets
  ports:
  - "8080:80"
      
nginx:
  image: nginx
  restart: unless-stopped
  volumes:
  - ./nginx.conf:/etc/nginx/nginx.conf
  links:
  - worker1:worker1
  - worker2:worker2
  - worker3:worker3
  ports:
  - "80:8080"
```

Create a `nginx.conf` file (I think this configuration correponds to a Layer 7 reverse proxy; it's probably better to use a Layer4 at tcp level?):

```
events {
  worker_connections  4096;
}

http {
  upstream compiler {
    least_conn;
    server worker1:8080;
    server worker2:8080;
    server worker3:8080;
  }

  server {
    listen 80;
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
```

Then run the following command (add `-d` if you want to detach):

```
sudo docker-compose up -d
```


