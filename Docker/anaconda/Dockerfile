FROM ubuntu:16.04

# install all packages
RUN set -x \
	&& apt-get update \
	&& apt-get install -y \
		cmake git swig bzip2 ca-certificates \
		build-essential \
		liblz4-dev libtinyxml-dev libfreeimage-dev libcurl4-openssl-dev libssl-dev uuid-dev \
		apache2 apache2-dev 

# install miniconda
ENV PATH            /usr/local/anaconda2/bin:$PATH
ENV LD_LIBRARY_PATH /usr/local/anaconda2/lib/:$LD_LIBRARY_PATH
RUN set -x \
	&& curl -o /tmp/miniconda.sh  "https://repo.continuum.io/miniconda/Miniconda2-4.4.10-Linux-x86_64.sh" \
    	&& /bin/bash /tmp/miniconda.sh -b -p /usr/local/anaconda2 \
    	&& rm /tmp/miniconda.sh \
	&& conda update -n base conda \
	&& conda install -c conda-forge numpy

# compile OpenVisus
ENV VISUS_HOME /home/visus
ENV PYTHONPATH /home/visus/build:$PYTHONPATH # not sure this is necessary
RUN echo "force clone 3" > /dev/null
RUN git clone https://github.com/sci-visus/OpenVisus $VISUS_HOME
RUN mkdir -p $VISUS_HOME/build
WORKDIR $VISUS_HOME/build 
RUN cmake ../ -DVISUS_GUI=0 -DVISUS_HOME=$VISUS_HOME -DVISUS_PYTHON_SYS_PATH=$(pwd) -DPYTHON_VERSION=2
RUN make

# configure apache
ADD ./files/000-default.conf     /etc/apache2/sites-enabled/000-default.conf
ADD ./files/httpd-foreground.sh  /usr/local/bin/httpd-foreground.sh
RUN echo "LoadModule visus_module /home/visus/build/libmod_visus.so" > /etc/apache2/mods-available/visus.load
RUN chmod a+x /usr/local/bin/httpd-foreground.sh 
RUN a2enmod headers 
RUN a2enmod visus
# todo (?) add visus javascript viewer? 
EXPOSE 80
CMD ["/usr/local/bin/httpd-foreground.sh"]

# add a sample dataset
RUN mkdir $VISUS_HOME/datasets
RUN cp -R /home/visus/Misc/dataset/cat $VISUS_HOME/datasets/cat
ADD ./files/visus.config    $VISUS_HOME/visus.config
RUN chown -R www-data $VISUS_HOME
RUN chmod -R a+rX  $VISUS_HOME 


