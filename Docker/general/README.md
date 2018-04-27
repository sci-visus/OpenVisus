Apache mod_visus server with lightweight webviewer
----------------------------------------------------

# How to build and install the image
Build the mod_visus image (see README in ubuntu folder), and copy usr_local_visus folder here or if mod_visus image already exists copy usr_local_visus directly from the docker container like below
```
sudo docker cp $DOCKER_ID:/usr/local/visus ./usr_local_visus
```

On windows:
```
dos2unix envvars # fix git problem 
dos2unix start_server.sh # fix git problem 
```

Build the Docker image, optionally providing a tag to identify it for a specific purpose
```
TAG=sometag
docker build -t $DOCKER_USER/visus:$TAG .
```

Push the image to Docker hub (OPTIONAL) 
```
docker push $DOCKER_USER/visus:$TAG
```

Get the image from Docker hub (OPTIONAL) 
```
docker pull $DOCKER_USER/visus:$TAG
```

# Run
Bare minumum run:
```
docker run -d -p 8080:80 $DOCKER_USER/visus:$TAG
```

Or run the container with one or more local volumes mapped into it (for logging, cache, configuration, etc).
Here's an example which maps local paths for these items:
```
docker run -d -p 8080:80 -v ./datasets:/visus/datasets -v ./config:/visus/config -v /scratch/visus_cache:/tmp/visus_cache $DOCKER_USER/visus:$TAG
```

Take note of the IP of the Docker VM when it starts:
```
DOCKER_IP=$(docker-machine ip) 
```

If you want to debug a running container, get the CONTAINER_ID and use
```
docker exec -i -t <CONTAINER_ID> /bin/bash
```
This will run the server in the background, but include an interactive session
Once attached, the log files can be examined, or the server can be restarted with
```
service apache2 restart
```
After you are finished inspecting, it is okay to simply 'exit' from this terminal.

These are the paths to map that the server will utilize:
- /visus/datasets  - 
- /visus/config    - contains .htpasswd, server.config, and visus.config
  - .htpasswd is used by apache for secure operations such as add_dataset, created like this:
```
htpasswd -c ./visus_config/.htpasswd your_username
htpasswd /var/lib/wwwrun/visus/.htpasswd another_username
```
  - server.config  - used to define the datasets available on the apache mod_visus server
  - visus.config   - used to specify configuration paramaters for the local `visus` utility
- /tmp/visus_cache - the location to use for $(VisusCacheDirectory)
  - NOTE: the default loopback device used by Docker is not very fast, so it's recommended to map this volume
- additional paths to be used for conversion or for datasets referenced by server.config

# Test

- Test mod_visus server access using curl
  - curl -v "http://$DOCKER_IP:8080/mod_visus?action=list"
  - curl -o output.txt -v "http://<machine_ip_here>:<port>/mod_visus?action=readdataset&dataset=cat"
- Test authenticated requests (Learn how to set username/password at http://wiki.visus.org/index.php?title=ViSUS_Server)
  - curl --user visus:<password> "http://your_server/mod_visus?action=add_dataset&name=dataset_name&url=file:///path/to/dataset.idx"
- Test webviewer access using your browser, by navigating to <hostname>:<port> and ensuring all your datasets are visible.

===============================================================================

Compilation of mod_visus.so library
-----------------------------------

TODO: This section still needs some cleanup. Fundamentally, the only thing that's really needed here is usr_local_visus (as copied into the above recipe), 
      so you only need to build and install visus, then just forget about testing it and proceed with the instructions above.

NOTE: These build steps could be combined with the dockerfile using multi-stage builds (https://docs.docker.com/engine/userguide/eng-image/multistage-build/)
      In summary, we go ahead and create the docker develop image as descibed below, but then we simply copy out the pieces into our destination image directly, no manual steps necessary.

Create and enter in the Linux virtual machine
    
```
#if you change the linux version, remember to change the DockerFile too
docker run -t -i ubuntu:16.04 /bin/bash
```
    
Change the prompt if you want to

```
export PS1="\[\033[49;1;31m\][\t] [\u@visus_develop] [\w]  \[\033[49;1;0m\] \n"
```
    
make sure you have an updated OS with all the components needed for compilation
(note: there are likely missing libraries in this list; just go ahead and install them if required)

```
apt-get -y update
apt-get -y upgrade
apt-get -y install \
  git gcc build-essential curl \
  vim apache2 apache2-dev wget unzip cmake \
  swig python python-dev libpng-dev nano \
  openssl libssl-dev libcurl4-openssl-dev \
  zlib1g-dev libfreeimage-dev \
  emacs24-nox cmake-curses-gui libcurl4-openssl-dev \
  libssl-dev iputils-ping \
  graphviz patchelf
```

Generate an ssh key for use with github (optional)

```
ssh-keygen -t rsa
# accept all defaults
cat ~/.ssh/id_rsa.pub
# navigate to http://github.com and go to your account settings -> SSH keys -> Add key
# copy and paste key into github dialog
```

Clone ViSUS code and compile it


```
mkdir -p /home/code/visus
cd /home/code/visus
# if you registered an ssh key above...
git clone git@github.com:sci-visus/visus.git ./
# otherwise...
git clone https://github.com/sci-visus/visus ./
# optionally build visuspy (you'll need to add some more libs above, and probably this should be in a different docker with which this one will be composed...)
BUILD_SWIG=1
mkdir -p /home/build/visus && cd /home/build/visus
cmake /home/code/visus \
  -DCMAKE_BUILD_TYPE=Release \
  -DVISUS_GUI=0 \
  -DVISUS_SERVER_CONFIG_FILE=/tmp/visus/server.config \
  -DVISUS_CACHE_PATH=/tmp/visus_cache \
  -DVISUS_DEFAULT_CONFIG_FILE=/tmp/visus/visus.config \
  -DVISUS_LOG_FILE=/tmp/visus/visus.log 

# or "make -j 8" if you want to run in parallel, but you may get a segmentation fault
make install
```

Configure Apache and mod_visus module.

```
# type 'sudo -i' if you are not root
VISUS_SRC=/home/code/visus
VISUS_LIB=/usr/local/visus/lib
mkdir -p /visus
mkdir -p /tmp/visus

cp -r $VISUS_SRC/Misc/dataset     /visus/dataset
cp $VISUS_SRC/Docker/server.config     /tmp/visus/server.config
chmod -R u+rwX,go+rX,go-w /visus /tmp/visus

cat <<EOF >/etc/apache2/mods-available/visus.load
LoadModule visus_module $VISUS_LIB/libmod_visus.so
EOF
    
rm /etc/apache2/sites-enabled/webviewer.conf
cat <<\EOF >/etc/apache2/sites-enabled/webviewer.conf
<VirtualHost *:80>
  ServerAdmin yourname@host.com
  DocumentRoot /visus/viewer
  
  <Directory /visus/viewer>
    Options Indexes FollowSymLinks MultiViews
    AllowOverride All
    Require all granted
  </Directory> 

  <LocationMatch "/mod_visus">
    <If "%{QUERY_STRING} =~ /.*action=AddDataset.*/ || %{QUERY_STRING} =~ /.*action=configure_datasets.*/ || %{QUERY_STRING} =~ /.*action=add_dataset.*/" >
      AuthType Basic
      AuthName "Authentication Required"
      AuthUserFile "/visus/config/.htpasswd"
      Require valid-user
    </If>
    <Else>
      Require all granted
    </Else>

    SetHandler visus
    Header set "Access-Control-Allow-Origin" "*"
   </LocationMatch>
      
   ErrorLog ${APACHE_LOG_DIR}/error.log
   CustomLog ${APACHE_LOG_DIR}/access.log combined  
      
 </VirtualHost>
```

Create VisusUserDirectory and set permissions for www-data (user that runs apache)

```
mkdir /var/www/visus
chown www-data /var/www/visus
chmod g+w /var/www/visus
```

Get the trusted root certificates needed to access some sites

```
cd /var/www/visus
curl --remote-name --time-cond cacert.pem https://curl.haxx.se/ca/cacert.pem
```

Enable mod_visus and run Apache server

```
a2enmod headers 
a2enmod visus
source /etc/apache2/envvars 
mkdir -p $APACHE_RUN_DIR $APACHE_LOCK_DIR $APACHE_LOG_DIR
/usr/sbin/apache2 -DFOREGROUND
```

Check the message from Visus server, the dataset list should show at least one
dataset. Press CTRL+C to kill the server

Exit Docker VM, check the Docker ID

```
exit
docker ps -a
DOCKER_ID=<here_your_number>
```

Commit the Virtual machine locally

```
docker commit $DOCKER_ID $DOCKER_USER/my_mod_visus
docker images
```

Test the visus server (OPTIONAL) 

```
docker run -i -t -p 8080:80 $DOCKER_USER/my_mod_visus /bin/bash
export PS1="\[\033[49;1;31m\][\t] [\u@\h] [\w]  \[\033[49;1;0m\] \n"
source /etc/apache2/envvars 
mkdir -p $APACHE_RUN_DIR $APACHE_LOCK_DIR $APACHE_LOG_DIR
/usr/sbin/apache2 -DFOREGROUND
```

Use CTRL-p CTRL-q to detach without stopping the container.
Later, you can attach to the Docker container to examine its current state. Get the container id from

```
docker ps
```

and attach using 

```
docker attach <CONTAINER_ID>
```

In another docker you can get the IP address of the machine:

```
docker-machine ls
```

and test it:

```
alias more=less
curl -o output.txt -v "http://<machine_ip_here>/mod_visus?action=readdataset&dataset=cat"
more output.txt
```

# Creation of a Apache+mod_visus Docker image

Once we have a working server, we can now create an image that only contains the compiled server instead of all the code and libraries necessary for compilation. This results in a Docker image that is only around 1/10 the size and is suitable for deployment. The properties of the new image, such as number of processors or availble memory, can easily be modified.

Returning to your local source tree

```
# CMAKE_SOURCE_DIR is where you have pulled the visus repository
cd ${CMAKE_SOURCE_DIR}/docker
```

Copy the pertinent files from the previous step

```
cd <visus>/Docker/general
VISUS_DIR=/visus
sudo docker cp $DOCKER_ID:$VISUS_DIR/config ./visus_config
sudo docker cp $DOCKER_ID:$VISUS_DIR/datasets ./visus_datasets
sudo docker cp $DOCKER_ID:/usr/local/visus ./usr_local_visus
sudo docker cp $DOCKER_ID:/etc/apache2/mods-available/visus.load       ./visus.load
sudo docker cp $DOCKER_ID:/etc/apache2/sites-enabled/webviewer.conf    ./webviewer.conf
sudo docker cp $DOCKER_ID:/etc/apache2/envvars                         ./envvars
sudo docker cp $DOCKER_ID:/visus/cacert.pem                            ./cacert.pem
```

Build a new Docker image with Apache server+precompiled mod_visus

```
docker build -t $DOCKER_USER/mod_visus .
``` 
   
Push the Docker Visus Image to Docker hub (OPTIONAL) 

```
docker push $DOCKER_USER/mod_visus
```

Get the Docker Visus Image from Docker hub (OPTIONAL) 

```
docker pull $DOCKER_USER/mod_visus
```

# Run and test

Run the virtual machine in interactive mode (including a virtual terminal)

```
# Typically, you will run the server with one or more local volumes mapped into the Docker instance (for data or cache):
docker run -t -i -d -p 8080:80 -v /scratch/datasets/cached:/tmp/visus_cache $DOCKER_USER/mod_visus

# Otherwise, run with the following command:
docker run -t -i -d -p 8080:80 $DOCKER_USER/mod_visus

sudo docker run -d -p 8080:80 -v /scratch/Docker/visus/datasets:/visus/datasets -v /scratch/Docker/visus/config:/tmp/visus/config -v /scratch/datasets/cached:/tmp/visus_cache scicameron/visus_develop:updated

# bare minumum run: 
docker run -d -p 8080:80 visus/mod_visus
```

This will run the server in the background, but include an interactive session
Take note of the IP of the Docker VM when it starts:

```
DOCKER_IP=$(docker-machine ip) 
```

From another host check the Visus Web server is working

```
curl -v "http://$DOCKER_IP:8080/mod_visus?action=list"
```

If you want to debug a running container, get the CONTAINER_ID and use

```
docker exec -i -t <CONTAINER_ID> /bin/bash
```

Once attached, the log files can be examined, or the server can be restarted with

```
service apache2 restart
```

After you are finished inspecting, it is okay to simply 'exit' from this terminal.