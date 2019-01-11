#
# ViSUS Docker *builder* for miniconda3 [Debian] environment
#
# Must build this Docker image from the OpenVisus directory (..):
#  `docker build -t OpenVisus:anaconda_builder -f docker/anaconda/builder/Dockerfile .`
#
# NOTES:
# - includes git repos for OpenVisus, XIDX, and visus_javascript
# - we build and test with this image, but ultimately transfer
#   installation to a fresh image to keep deployment small
#

FROM continuumio/miniconda3

# install all necessary packages for building, update conda and pip
RUN set -x \
  && conda update -y -n base conda \
  && conda install -y \
  cmake swig ca-certificates \
  libxml2 numpy zlib \
	&& apt-get update \
	&& apt-get install -y \
	build-essential \
	apache2 apache2-dev \
  libgomp1
# conda maybe also...
  # genutil lxml
# apt-get maybe also...
  # libapache2-mod-php

# clone OpenVisus, XIDX, visus_javascript
ENV CODE /root/code
ENV VISUS_HOME $CODE/OpenVisus
ENV XIDX_HOME $CODE/XIDX 
ENV WEBVIEWER_HOME $CODE/webviewer
RUN mkdir $CODE
WORKDIR $CODE
RUN set -x \
  && git clone https://github.com/sci-visus/OpenVisus.git \
  && git clone https://github.com/sci-visus/XIDX.git \
  && git clone https://github.com/sci-visus/visus_javascript.git

# configure OpenVisus
RUN set -x \
  && cd OpenVisus && mkdir build && cd build \
  && cmake .. \
  -DVISUS_GUI=0

# build and install XIDX (into OpenVisus install)
RUN set -x \
  && cd XIDX && mkdir build && cd build \
  && cmake -DCMAKE_INSTALL_PREFIX=$VISUS_HOME/build/install ..

#######
# setup environment for debugging: paths, apache setup, certs
# note: these should be nearly identical to installer Dockerfile

# setup paths
ENV PATH $VISUS_HOME/build/install/bin:$PATH
ENV PYTHONPATH $VISUS_HOME/build/install:$VISUS_HOME/build/install/lib:$PYTHONPATH
ENV LD_LIBRARY_PATH $VISUS_HOME/build/install/bin:$VISUS_HOME/build/install/lib:$LD_LIBRARY_PATH

# configure mod_visus (should this be a script instead?)
#ADD Docker/anaconda/000-default.conf /etc/apache2/sites-enabled/000-default.conf   # no need since we're adding the webviewer
ADD Docker/anaconda/httpd-foreground.sh  /usr/local/bin/httpd-foreground.sh
RUN echo "LoadModule visus_module ${VISUS_HOME}/bin/libmod_visus.so" > /etc/apache2/mods-available/visus.load \
  && a2enmod headers \
  && a2enmod visus \
  && chmod a+x /usr/local/bin/httpd-foreground.sh \
  && chmod -R 755 ${VISUS_HOME}

# install webviewer (should this script just live in webviewer git?)
ADD Docker/anaconda/webviewer.conf  /etc/apache2/sites-enabled/webviewer.conf
ADD Docker/anaconda/.htpasswd       $VISUS_HOME/.htpasswd
RUN rm /etc/apache2/sites-enabled/000-default.conf

# download certificate
RUN set -x \
  && mkdir -p /var/www/visus \
  && cd /var/www/visus \
  && curl --remote-name --time-cond cacert.pem https://curl.haxx.se/ca/cacert.pem \
  && chown -R www-data .

# ready to debug
################

EXPOSE 80
# build and install OpenVisus on startup since any failure will prevent creating this image
ADD Docker/anaconda/builder/package_all.sh /root/code/package_all.sh
ADD Docker/anaconda/builder/build_all.sh /root/code/build_all.sh
RUN chmod a+x /root/code/build_all.sh && chmod a+x /root/code/package_all.sh
CMD ["/root/code/build_all.sh", "/root/code/package_all.sh", "/bin/bash"]
