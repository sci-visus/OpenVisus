# sudo docker login -u scrgiorgio
# TAG=4.2
# sudo docker build --tag visus/portable-linux-binaries_x86_64:$TAG --tag visus/portable-linux-binaries_x86_64:latest --progress=plain .
# sudo docker push visus/portable-linux-binaries_x86_64:$TAG
# sudo docker push visus/portable-linux-binaries_x86_64:latest

FROM nsdf/manylinux2010_x86_64:latest

WORKDIR /tmp

# openssl
RUN \
  curl -L --insecure --retry 3 https://www.openssl.org/source/openssl-1.0.2n.tar.gz  -O && \
  tar xzf openssl-*.tar.gz && \
  pushd openssl-* && \
  ./config -fpic shared --prefix=/usr --openssldir=/usr && \
  make && \
  make install && \
  popd && \
  rm -Rf openssl-* 

COPY httpd24.sh .
RUN bash httpd24.sh

RUN yum -y install xz \
	libX11-devel libx11-xcb-devel libXrender-devel libXau-devel libXext-devel \
	mesa-libGL-devel mesa-libGLU-devel \
	fontconfig fontconfig-devel freetype freetype-devel

COPY qt512.sh .
RUN bash qt512.sh 

# install conda
RUN \
  curl -L -O https://github.com/conda-forge/miniforge/releases/latest/download/Miniforge3-Linux-x86_64.sh && \
  bash Miniforge3-Linux-x86_64.sh -b && \
  rm -f Miniforge3-Linux-x86_64.sh && \
  ~/miniforge3/bin/conda config --set always_yes yes --set anaconda_upload no && \
  ~/miniforge3/bin/conda install -c conda-forge conda cmake swig anaconda-client conda-build wheel
  
RUN yum install -y mesa-libGLU yum install -y mesa-libGLU-devel

# added qt 5.15  
COPY qt515.sh .
RUN bash qt515.sh 


