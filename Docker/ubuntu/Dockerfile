# Ubuntu 18.04.1 LTS (Bionic Beaver)
# Ubuntu 16.04.5 LTS (Xenial Xerus)
# Ubuntu 14.04.5 LTS (Trusty Tahr)
FROM ubuntu:trusty 

ARG PYTHON_VERSION=3.6.6
ARG GIT_BRANCH=master

RUN apt-get update && apt-get -qq install git
ADD https://api.github.com/repos/sci-visus/OpenVisus/git/refs/heads/$GIT_BRANCH version.json
RUN git clone -b${GIT_BRANCH} https://github.com/sci-visus/OpenVisus.git /home/OpenVisus      

WORKDIR /home/OpenVisus
RUN chmod +x ./CMake/build_ubuntu.sh && ./CMake/build_ubuntu.sh
