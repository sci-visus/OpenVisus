
# TAG=visus/portable-linux-binaries
# sudo docker build --tag $TAG -f scripts/PortableLinuxBinaries.Dockerfile .
# sudo docker login 
# sudo docker push $TAG
#
# to start interactively
# sudo docker run -it visus/portable-linux-binaries /bin/bash
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

# python inside manylinux don't contain python shared library, so I need to compile by myself
COPY scripts/install/cpython.sh .
RUN bash cpython.sh 3.8.2
RUN bash cpython.sh 3.7.7
RUN bash cpython.sh 3.6.10

# COPY scripts/install/miniconda.sh .
# COPY scripts/install/python.conda.sh .
# RUN bash miniconda.sh 
# RUN bash python.conda.sh 3.6
# RUN bash python.conda.sh 3.7
# RUN bash python.conda.sh 3.8

COPY scripts/install/httpd24.sh .
RUN bash httpd24.sh

RUN yum -y install xz \
	libX11-devel libx11-xcb-devel libXrender-devel libXau-devel libXext-devel \
	mesa-libGL-devel mesa-libGLU-devel \
	fontconfig fontconfig-devel freetype freetype-devel

COPY scripts/install/qt59.sh .
RUN bash qt59.sh 

COPY scripts/install/qt512.sh .
RUN bash qt512.sh 




