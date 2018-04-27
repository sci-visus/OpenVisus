Visus Python swig Docker instance
----------------------------------------------------

# How to build and install the image

Build the visuspy module in your docker develop image (see ../general/README.md), and copy the visuspy directory (/usr/local/visus/visuspy) to this directory. Keep track of whether you built with python2 or python3.

```
docker cp $DOCKER_ID:/usr/local/visus/visuspy ./visuspy
```

Build the Docker image, specifying python3 if necessary:

```
PYTHON3="--build-arg python3=true"
TAG=sometag
docker build -t $DOCKER_USER/visus_python:$TAG ${PYTHON3} .
```

Push the Docker Visus Image to Docker hub (OPTIONAL) 
Note the command below appends '3' to the repo name if PYTHON3 was set, so both visus_python and visus_python3 can exist.

```
docker push $DOCKER_USER/visus_python${PYTHON3:+3}
```

Get the Docker Visus Image from Docker hub (OPTIONAL) 

```
docker pull $DOCKER_USER/visus_python
```

# Run and test

Run the virtual machine in interactive mode (including a virtual terminal). You will be presented with a bash prompt with the visuspy module available to python.

```
docker run -t -i $DOCKER_USER/visus_python
```

