FROM quay.io/pypa/manylinux1_x86_64

ARG PYTHON_VERSION=3.6.6
ARG GIT_BRANCH=Centos5
ARG CMAKE_BUILD_TYPE=Release
ARG VISUS_INTERNAL_DEFAULT=1
ARG DISABLE_OPENMP=1
ARG VISUS_GUI=0
ARG VISUS_MODVISUS=0

ARG PYPI_USERNAME=""
ARG PYPI_PASSWORD=""

ADD https://api.github.com/repos/sci-visus/OpenVisus/git/refs/heads/$GIT_BRANCH version.json
RUN git clone -b${GIT_BRANCH} https://github.com/sci-visus/OpenVisus.git /home/OpenVisus

WORKDIR /home/OpenVisus 
COPY *.sh  CMake/build/ 
RUN chmod +x CMake/build/*.sh
RUN ./CMake/build/manylinux.sh







  
