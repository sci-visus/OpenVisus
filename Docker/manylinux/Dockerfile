
# compile docker:
#   sudo docker build -t openvisus-manylinux .
# 
# to run interactively step by step
#   sudo docker run -i -t quay.io/pypa/manylinux1_x86_64 /bin/bash
# 
# to debug errors
#   sudo docker run --rm -it openvisus-manylinux /bin/bash -il 
# 
# to create an instance
#   sudo docker run --name openvisus-manylinux-instance openvisus-manylinux /bin/true
# 
# to copy the wheel
#   sudo docker cp openvisus-manylinux-instance:/home/visus/build/install/dist ./
# 
# remove the instance
#   sudo docker rm openvisus-manylinux-instance

FROM quay.io/pypa/manylinux1_x86_64

ARG PYTHON_VERSION=3.6.6
ARG GIT_BRANCH=Centos5
ARG CMAKE_BUILD_TYPE=Release
ARG VISUS_INTERNAL_DEFAULT=1
ARG DISABLE_OPENMP=1
ARG VISUS_GUI=0
ARG PYPI_USERNAME=""
ARG PYPI_PASSWORD=""

ADD https://api.github.com/repos/sci-visus/OpenVisus/git/refs/heads/$GIT_BRANCH version.json
RUN git clone -b${GIT_BRANCH} https://github.com/sci-visus/OpenVisus.git /home/visus

WORKDIR /home/visus 
RUN chmod u+x ./CMake/build/manylinux.sh && ./CMake/build/manylinux.sh






  
