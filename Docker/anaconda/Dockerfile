# TODO: need to test this file

FROM ubuntu:16.04

# install all packages
RUN apt-get update 
RUN apt-get install -y cmake git swig bzip2 ca-certificates curl build-essential 
RUN apt-get install -y apache2 apache2-dev

# install miniconda
RUN set -x \
	&& curl -o /tmp/miniconda.sh  "https://repo.continuum.io/miniconda/Miniconda2-4.4.10-Linux-x86_64.sh" \
	&& /bin/bash /tmp/miniconda.sh -b -p /usr/local/anaconda2 \
	&& rm /tmp/miniconda.sh \
	&& /usr/local/anaconda2/bin/conda update  -y -n base conda \
	&& /usr/local/anaconda2/bin/conda install -y -c conda-forge numpy

# compile OpenVisus
ENV VISUS_HOME /home/visus
ADD https://api.github.com/repos/sci-visus/OpenVisus/git/refs/heads/$GIT_BRANCH version.json
RUN git clone https://github.com/sci-visus/OpenVisus $VISUS_HOME
RUN mkdir -p $VISUS_HOME/build

WORKDIR $VISUS_HOME/build 

RUN cmake \
	-DVISUS_GUI=0 \
	-DVISUS_INTERNAL_DEFAULT=1 \
	-DPYTHON_VERSION=2 \
	-DPYTHON_EXECUTABLE=/usr/local/anaconda2/bin/python2.7 \
	-DPYTHON_LIBRARY=/usr/local/anaconda2/lib/libpython2.7.so \
	-DPYTHON_INCLUDE_DIRS=/usr/local/anaconda2/include/python2.7 \
	../ 
	
RUN make

# setup OpenVisus
ENV PATH /usr/local/anaconda2/bin:$PATH
ENV PYTHONPATH $VISUS_HOME/build/:$PYTHONPATH
ENV LD_LIBRARY_PATH /usr/local/anaconda2/lib/:$LD_LIBRARY_PATH
RUN python setup.py install

# configure apache
ADD 000-default.conf     /etc/apache2/sites-enabled/000-default.conf
ADD httpd-foreground.sh  /usr/local/bin/httpd-foreground.sh
RUN echo "LoadModule visus_module $VISUS_HOME/build/libmod_visus.so" > /etc/apache2/mods-available/visus.load
RUN chmod a+x /usr/local/bin/httpd-foreground.sh 
RUN a2enmod headers 
RUN a2enmod visus

# install webviewer
RUN git clone https://github.com/sci-visus/visus_javascript $VISUS_HOME/webviewer
ADD webviewer.conf  /etc/apache2/sites-enabled/webviewer.conf
ADD .htpasswd       $VISUS_HOME/.htpasswd
RUN  rm             /etc/apache2/sites-enabled/000-default.conf

# download certificate
RUN mkdir -p /var/www/visus && \
    cd /var/www/visus && \
    curl --remote-name --time-cond cacert.pem https://curl.haxx.se/ca/cacert.pem && \
    chown -R www-data .
    
# add a sample dataset
RUN echo "<include url='/mnt/visus_datasets/visus.config' />" > ${VISUS_HOME}/visus.config
RUN chown -R www-data $VISUS_HOME
RUN chmod -R a+rX     $VISUS_HOME 

EXPOSE 80
CMD ["/usr/local/bin/httpd-foreground.sh"]




