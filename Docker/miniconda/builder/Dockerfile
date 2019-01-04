#
# ViSUS Docker *builder* for miniconda3 [Debian] environment
#
# Must build this Docker image from the OpenVisus directory (..):
#  `docker build -t OpenVisus:miniconda_builder -f docker/miniconda/builder/Dockerfile .`
#
# NOTES:
# - includes git repos for OpenVisus, XIDX, and visus_javascript
#  > update these repos when you run the Docker image (TODO: add an update_repos.sh script to accomplish this)
#

FROM continuumio/miniconda3

# install packages necessary for building
RUN set -x \
	&& apt-get update \
	&& apt-get install -y \
	cmake git swig bzip2 ca-certificates curl \
	build-essential \
	apache2 apache2-dev \
  libgomp1 libxml2-dev
#maybe also...
#python-dev libapache2-mod-php

# install conda packages
RUN conda install -y numpy
# maybe also...
#cdms2 genutil lxml

# clone OpenVisus, XIDX, visus_javascript
RUN mkdir /root/code
ENV CODE /root/code
ENV VISUS_HOME $CODE/OpenVisus
ENV XIDX_HOME $CODE/XIDX 
ENV WEBVIEWER_HOME $CODE/webviewer
WORKDIR $CODE
RUN git clone https://github.com/sci-visus/OpenVisus.git
RUN git clone https://github.com/sci-visus/XIDX.git
RUN git clone https://github.com/sci-visus/visus_javascript.git

# build and install OpenVisus
# - see below for detailed commands to accomplish this
# - is this really the correct action to take for the Docker builder image?

# build and install XIDX
# - is this really the correct action to take for the Docker builder image?

# install webviewer
# - is this really the correct action to take for the Docker builder image?

# NOTE: We want to build and test with this image, but ultimately transfer the installs to the install image,
#       thereby keeping the deployment reasonably small for customers to use.



CMD ["/bin/bash"]





#---------------
#---------------
# old stuff may or may not be needed (this is a draft of the builder Dockerfile, so keeping it here for reference)

# compile OpenVisus
# ENV VISUS_HOME /home/visus
# RUN echo "force clone 5" > /dev/null
# RUN git clone https://github.com/sci-visus/OpenVisus $VISUS_HOME
# RUN mkdir -p $VISUS_HOME/build
# WORKDIR $VISUS_HOME/build 
# RUN cmake ../ -DVISUS_GUI=0 \
# 	-DVISUS_HOME=$VISUS_HOME \
# 	-DVISUS_PYTHON_SYS_PATH=$(pwd) \
# 	-DVISUS_INTERNAL_ZLIB=1 \
# 	-DVISUS_INTERNAL_LZ4=1 \
# 	-DVISUS_INTERNAL_TINYXML=1 \
# 	-DVISUS_INTERNAL_FREEIMAGE=1 \
# 	-DVISUS_INTERNAL_OPENSSL=1 \
# 	-DVISUS_INTERNAL_CURL=1 \
# 	-DVISUS_INTERNAL_SWIG=1 \
# 	-DPYTHON_VERSION=2 \
# 	-DPYTHON_EXECUTABLE=/usr/local/anaconda2/bin/python2.7 \
# 	-DPYTHON_LIBRARY=/usr/local/anaconda2/lib/libpython2.7.so \
# 	-DPYTHON_INCLUDE_DIRS=/usr/local/anaconda2/include/python2.7 
# RUN make

# # build and install XIDX
# WORKDIR $XIDX_HOME
# RUN mkdir -p build
# WORKDIR $XIDX_HOME/build
#note: installing xidx alongside visus
# RUN cmake -DCMAKE_INSTALL_PREFIX=$VISUS_HOME/build/install .. && make 
# ENV PYTHONPATH $XIDX_HOME/build:$PYTHONPATH

# # setup OpenVisus
# ENV PATH /usr/local/anaconda2/bin:$PATH
# ENV PYTHONPATH $VISUS_HOME/build/:$PYTHONPATH
# ENV LD_LIBRARY_PATH /usr/local/anaconda2/lib/:$LD_LIBRARY_PATH
# RUN python setup.py install

# # configure apache
# ADD ./files/000-default.conf     /etc/apache2/sites-enabled/000-default.conf
# ADD ./files/httpd-foreground.sh  /usr/local/bin/httpd-foreground.sh
# RUN echo "LoadModule visus_module $VISUS_HOME/build/libmod_visus.so" > /etc/apache2/mods-available/visus.load
# RUN chmod a+x /usr/local/bin/httpd-foreground.sh 
# RUN a2enmod headers 
# RUN a2enmod visus

# # install webviewer
# RUN git clone https://github.com/sci-visus/visus_javascript $VISUS_HOME/webviewer
# ADD ./files/webviewer.conf  /etc/apache2/sites-enabled/webviewer.conf
# ADD ./files/.htpasswd       $VISUS_HOME/.htpasswd
# RUN  rm                     /etc/apache2/sites-enabled/000-default.conf

# # download certificate
# RUN mkdir -p /var/www/visus && \
#     cd /var/www/visus && \
#     curl --remote-name --time-cond cacert.pem https://curl.haxx.se/ca/cacert.pem && \
#     chown -R www-data .

# EXPOSE 80
# CMD ["/usr/local/bin/httpd-foreground.sh"]

# # add a sample dataset
# ADD ./files/visus.config    $VISUS_HOME/visus.config
# RUN chown -R www-data $VISUS_HOME
# RUN chmod -R a+rX  $VISUS_HOME 


