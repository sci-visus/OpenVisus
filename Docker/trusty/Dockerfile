# /////////////////////////////////////////////////////////
# compile docker:
#   DOCKER_TAG=openvisus-trusty
#   sudo docker build -t $DOCKER_TAG Docker/trusty
# 
# to run interactively step by step
#   sudo docker run -i -t ubuntu:trusty /bin/bash
# 
# to debug errors
#   sudo docker run --rm -it $DOCKER_TAG /bin/bash -il 
# /////////////////////////////////////////////////////////

FROM ubuntu:trusty 

ARG PYTHON_VERSION=3.6.6
ARG GIT_BRANCH=Centos5
ARG CMAKE_BUILD_TYPE=Release
ARG VISUS_INTERNAL_DEFAULT=0
ARG DISABLE_OPENMP=0
ARG VISUS_GUI=1

RUN sudo apt-get update
RUN sudo apt-get -qy install git

ADD https://api.github.com/repos/sci-visus/OpenVisus/git/refs/heads/$GIT_BRANCH version.json
RUN git clone -b${GIT_BRANCH} https://github.com/sci-visus/OpenVisus.git /home/OpenVisus
WORKDIR /home/OpenVisus 
RUN chmod u+x ./Docker/trusty/build.sh && ./Docker/trusty/build.sh



  
