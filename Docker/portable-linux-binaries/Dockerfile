# IMAGE_NAME=visus/portable-linux-binaries_x86_64:2.0
# sudo docker build --tag $IMAGE_NAME --progress=plain .
# sudo docker login -u scrgiorgio
# sudo docker push $IMAGE_NAME

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




