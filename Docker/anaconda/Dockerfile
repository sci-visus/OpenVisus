FROM ubuntu:16.04

# install all packages
RUN set -x \
	&& apt-get update \
	&& apt-get install -y \
	cmake git swig bzip2 ca-certificates curl \
	build-essential \
	apache2 apache2-dev

# install miniconda
RUN set -x \
	&& curl -o /tmp/miniconda.sh  "https://repo.continuum.io/miniconda/Miniconda2-4.4.10-Linux-x86_64.sh" \
	&& /bin/bash /tmp/miniconda.sh -b -p /usr/local/anaconda2 \
	&& rm /tmp/miniconda.sh \
	&& /usr/local/anaconda2/bin/conda update  -y -n base conda \
	&& /usr/local/anaconda2/bin/conda install -y -c conda-forge numpy

# compile OpenVisus
ENV VISUS_HOME /home/visus
RUN echo "force clone 5" > /dev/null
RUN git clone https://github.com/sci-visus/OpenVisus $VISUS_HOME
RUN mkdir -p $VISUS_HOME/build
WORKDIR $VISUS_HOME/build 
RUN cmake ../ -DVISUS_GUI=0 \
	-DVISUS_HOME=$VISUS_HOME \
	-DVISUS_PYTHON_SYS_PATH=$(pwd) \
	-DVISUS_INTERNAL_ZLIB=1 \
	-DVISUS_INTERNAL_LZ4=1 \
	-DVISUS_INTERNAL_TINYXML=1 \
	-DVISUS_INTERNAL_FREEIMAGE=1 \
	-DVISUS_INTERNAL_OPENSSL=1 \
	-DVISUS_INTERNAL_CURL=1 \
	-DVISUS_INTERNAL_SWIG=1 \
	-DPYTHON_VERSION=2 \
	-DPYTHON_EXECUTABLE=/usr/local/anaconda2/bin/python2.7 \
	-DPYTHON_LIBRARIES=/usr/local/anaconda2/lib/libpython2.7.so \
	-DPYTHON_INCLUDE_DIRS=/usr/local/anaconda2/include/python2.7 
RUN make

# setup visuspy
ENV PATH /usr/local/anaconda2/bin:$PATH
ENV PYTHONPATH $VISUS_HOME/build/:$PYTHONPATH
ENV LD_LIBRARY_PATH /usr/local/anaconda2/lib/:$LD_LIBRARY_PATH
RUN python setup.py install

# configure apache
ADD ./files/000-default.conf     /etc/apache2/sites-enabled/000-default.conf
ADD ./files/httpd-foreground.sh  /usr/local/bin/httpd-foreground.sh
RUN echo "LoadModule visus_module /home/visus/build/libmod_visus.so" > /etc/apache2/mods-available/visus.load
RUN chmod a+x /usr/local/bin/httpd-foreground.sh 
RUN a2enmod headers 
RUN a2enmod visus

# install webviewer
RUN git clone https://github.com/sci-visus/visus_javascript $VISUS_HOME/webviewer
ADD ./files/webviewer.conf  /etc/apache2/sites-enabled/webviewer.conf
ADD ./files/.htpasswd       $VISUS_HOME/.htpasswd
RUN  rm                     /etc/apache2/sites-enabled/000-default.conf

# download certificate
RUN mkdir -p /var/www/visus && \
    cd /var/www/visus && \
    curl --remote-name --time-cond cacert.pem https://curl.haxx.se/ca/cacert.pem && \
    chown -R www-data .

EXPOSE 80
CMD ["/usr/local/bin/httpd-foreground.sh"]

# add a sample dataset
RUN mkdir $VISUS_HOME/datasets
RUN cp -R /home/visus/Misc/dataset/cat $VISUS_HOME/datasets/cat
ADD ./files/visus.config    $VISUS_HOME/visus.config
RUN chown -R www-data $VISUS_HOME
RUN chmod -R a+rX  $VISUS_HOME 


