FROM ubuntu:trusty 

ARG PYTHON_VERSION=3.6.6
ARG GIT_BRANCH=Centos5
ARG CMAKE_BUILD_TYPE=Release
ARG VISUS_INTERNAL_DEFAULT=0
ARG DISABLE_OPENMP=0
ARG VISUS_GUI=1
ARG VISUS_MODVISUS=0
                      
# clone OpenVisus
RUN sudo apt-get update
RUN sudo apt-get -qy install git                                 
ADD https://api.github.com/repos/sci-visus/OpenVisus/git/refs/heads/$GIT_BRANCH version.json
RUN git clone -b${GIT_BRANCH} https://github.com/sci-visus/OpenVisus.git /home/OpenVisus      

WORKDIR /home/OpenVisus 
COPY build.sh  ./
RUN chmod u+x build.sh

# if you want to debug: 
#   (*) comment the "RUN build.sh"
#   (*) sudo docker run --rm -it $DOCKER_TAG /bin/bash
#   (*) alias ARG=
#   (*) cut&paste all the Docker part with ARG
#   (*) ./build.sh

# RUN build.sh 




  
