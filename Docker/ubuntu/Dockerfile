FROM ubuntu:16.04

# install all packages
RUN set -x \
	&& apt-get update \
	&& apt-get install -y \
		cmake git swig bzip2 ca-certificates \
		build-essential \
		liblz4-dev libtinyxml-dev libfreeimage-dev libcurl4-openssl-dev libssl-dev uuid-dev \
		python3 python3-pip \
		apache2 apache2-dev \
	&& pip3 install --upgrade pip \
	&& pip3 install numpy

# compile OpenVisus
ENV VISUS_HOME /home/visus
RUN echo "force clone 1" > /dev/null
RUN git clone https://github.com/sci-visus/OpenVisus $VISUS_HOME
RUN mkdir -p $VISUS_HOME/build
WORKDIR $VISUS_HOME/build 
RUN cmake ../ -DVISUS_GUI=0 -DVISUS_HOME=$VISUS_HOME -DVISUS_PYTHON_SYS_PATH=$(pwd)
RUN make

# configure apache
ADD ./files/000-default.conf     /etc/apache2/sites-enabled/000-default.conf
ADD ./files/httpd-foreground.sh  /usr/local/bin/httpd-foreground.sh
RUN echo "LoadModule visus_module /home/visus/build/libmod_visus.so" > /etc/apache2/mods-available/visus.load
RUN chmod a+x /usr/local/bin/httpd-foreground.sh 
RUN a2enmod headers 
RUN a2enmod visus 
EXPOSE 80
CMD ["/usr/local/bin/httpd-foreground.sh"]

# add a sample dataset
RUN mkdir $VISUS_HOME/datasets
RUN cp -R /home/visus/Misc/dataset/cat $VISUS_HOME/datasets/cat
ADD ./files/visus.config    $VISUS_HOME/visus.config
RUN chown -R www-data $VISUS_HOME
RUN chmod -R a+rX  $VISUS_HOME 


  
