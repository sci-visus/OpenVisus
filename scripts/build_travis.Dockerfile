# example
# TAG=visus/travis-image
# sudo docker build --tag $TAG -f scripts/build_travis.Dockerfile .
# sudo docker login 
# sudo docker push $TAG
#
# if you see some errors in the build you can do:
# sudo docker run --rm -it $TAG /bin/bash 

FROM quay.io/pypa/manylinux2010_x86_64

ENV LC_ALL en_US.UTF-8
ENV LANG en_US.UTF-8
ENV LANGUAGE en_US.UTF-8

WORKDIR /install

RUN yum -y install patchelf xz zlib zlib-devel

COPY scripts/install/openssl.sh .
RUN bash  openssl.sh

COPY scripts/install/cmake.sh .
RUN bash  cmake.sh

COPY scripts/install/swig.sh .
RUN bash  swig.sh

COPY scripts/install/cpython.sh .
RUN bash cpython.sh 3.8.2
RUN bash cpython.sh 3.7.7
RUN bash cpython.sh 3.6.10

COPY scripts/install/miniconda.sh .
RUN bash miniconda.sh 

COPY scripts/install/python.conda.sh .
RUN bash python.conda.sh 3.6
RUN bash python.conda.sh 3.7
RUN bash python.conda.sh 3.8

COPY scripts/install/httpd24.sh .
RUN bash httpd24.sh

# be careful if you change version, it could have problems with conda/pyqt in case you need it
COPY scripts/install/qt59.sh .
RUN bash qt59.sh 
