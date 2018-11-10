FROM quay.io/pypa/manylinux1_x86_64

ARG PYTHON_VERSION=3.6.6
ARG GIT_BRANCH=master
ARG CMAKE_BUILD_TYPE=Release
ARG VISUS_INTERNAL_DEFAULT=1
ARG DISABLE_OPENMP=1
ARG VISUS_GUI=0

ADD https://api.github.com/repos/sci-visus/OpenVisus/git/refs/heads/$GIT_BRANCH version.json
RUN mkdir -p /home/visus
RUN git clone -b${GIT_BRANCH} https://github.com/sci-visus/OpenVisus.git /home/visus

COPY  build.sh /home/visus
WORKDIR /home/visus 
RUN chmod u+x ./build.sh && ./build.sh






  
