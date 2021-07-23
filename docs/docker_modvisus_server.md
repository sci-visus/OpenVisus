# Introduction

In this example, we run an OpenVisus server inside a Docker container. 

The OpenVisus server is a C++ [Apache module](http://httpd.apache.org/docs/2.4/developer/modguide.html) called [mod_visus](https://github.com/sci-visus/OpenVisus/tree/master/Executable/mod_visus).  

In this tutorial we assume that the datasets are stored in a *shared* directory (i.e. an *host* directory mounted  inside the Docker instance); but there are other more advanced options, such as using Object Storage (e.g. Amazon S3, Wasabi, Google Cloud Storage, Azure Blob Storage etc.) and let the OpenVisus server to create a local cache. 

Apache server is the `httpd:2.4` version and runs in `event` mode under  `daemon` account. `HTTP` (port 80) and `HTTPS` (port 443) will be mapped to port `8080` and port `443`. If you need to change this setup, you may need to customize the [Dockerfile](https://github.com/sci-visus/OpenVisus/tree/master/Docker/mod_visus/httpd).

<p align = "center">
<img src="https://github.com/sci-visus/images/blob/main/diagram.png?raw=true">
<i>Figure 1. Diagram of a dockerized OpenVisus server</i>
</p>


For more complex case with a load balancer (e.g. nginx or HAProxy) in front of OpenVisus instances, please refer to `Kubernetes` or `Docker Swarm` tutorials.  


# Create a dataset directory

As an example of creating a minimal dataset directory, type:

```
export DATASETS=/mnt/data/datasets
mkdir -p $DATASETS

pushd $DATASETS
wget https://github.com/sci-visus/OpenVisus/releases/download/files/2kbit1.zip
unzip 2kbit1.zip -d ./2kbit1
rm -f 2kbit1.zip
popd
```

Create a `datasets.config` file i.e. the master configuration file which tells to OpenVisus where the data is located:
```
cat <<EOF > $DATASETS/datasets.config
<visus>
  <datasets>
    <dataset name="2kbit1" url="/datasets/2kbit1/visus.idx" />
  </datasets>
</visus>
EOF
```

**Important note**: Docker instance will see the `$DATASET` directory as `/datasets`, so the `<dataset ...>` items should be filled accordingly.

# Run the OpenVisus server

You can run the server from the external (change the image version as needed):

```
IMAGE=visus/mod_visus:2.1.142
sudo docker run --rm --publish 8080:80 --publish 443:443 -v $DATASETS:/datasets $IMAGE
```

From another shell:

```
# should return an HTML document (BODY it works)
curl http://localhost:8080

# internal messages
curl http://localhost:8080/server-status

# it should return an XML list with 2kbit1 inside
curl http://localhost:8080/mod_visus?action=list

# it should return a text file with (version) as first line
curl http://localhost:8080/mod_visus?dataset=2kbit1 
```


If you want to debug the server with an internal shell:

```
sudo docker run --rm -it --publish 8080:80 --publish 443:443 -v $DATASETS:/datasets $IMAGE /bin/bash
/usr/local/bin/httpd-foreground
```
# Optional sections


## Enable password security 

To enable password security:

```
export MODVISUS_USERNAME=visus
export MODVISUS_PASSWORD=`openssl rand -base64 48`

# save in a file for later use
cat <<EOF > .mod_visus.identity.sh
#!/bin/bash
export MODVISUS_USERNAME=$MODVISUS_USERNAME
export MODVISUS_PASSWORD=$MODVISUS_PASSWORD
EOF
chmod 600 .mod_visus.identity.sh
source .mod_visus.identity.sh

htpasswd -nb  $MODVISUS_USERNAME $MODVISUS_PASSWORD > $DATASETS/.htpasswd

# TOFIX: is this secure?
chmod 644 $DATASETS/.htpasswd
```

To test the password-protected server with curl:

```
curl -u$MODVISUS_USERNAME:$MODVISUS_PASSWORD ...
```


## Enable dynamic datasets 

If your need to change the `datasets.config` at runtime, for example because you want to dynamically add/remove datasets at runtime, add a `ModVisus/Dynamic` section to your `datasets.config`:

```
<visus>
  <ModVisus>
     <Dynamic enabled='true' filename='/datasets/datasets.config' msec='5000' />
  </ModVisus>
  ...
</visus>
```

This way mod_visus will check for file changes every 5000 milliseconds, and will fire a `reload` dataset event if needed.


## File permission problems 

The *quick-and-dirty* way of fixing file permissions is to set `read-write` permissions for all datasets:

```
chmod -R a+rwX $DATASETS
```

Another more polite way is to map the users and groups of the host to the docker instance.

Find all the groups

```
find $DATASETS -printf "%g %G\n" | sort | uniq 
```

this command will print a list with the group name (`%g`) and group ID (`%G`):

```
user1 1000
user2 1001
...
```

Then create Dockerfile adding the groups  and adding the `daemon` user to those groups:

```
FROM $IMAGE
RUN addgroup --gid <group-id-1> <group-name-1>
RUN addgroup --gid <group-id-2> <group-name-2>
RUN usermod -a -G <group-name-1>,<group-name-2> daemon 
```

Build the image:

```
sudo docker build --tag visus/my_mod_visus:$TAG .
```

Another way (**not tested**) is to directly add the groups to `daemon` user by command line:

```
docker run ... --group-add <group-id-1> --group-add <group-id-2> ...

```


