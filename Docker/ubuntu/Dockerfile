FROM ubuntu:16.04

# install all packages
RUN set -x \
	&& apt-get update \
	&& apt-get install -y \
		cmake git swig bzip2 ca-certificates \
		build-essential libssl-dev uuid-dev \
		python3 python3-pip \
		apache2 apache2-dev \
	&& pip3 install --upgrade pip \
	&& pip3 install numpy

# compile OpenVisus
ENV VISUS_HOME /home/visus
ENV GIT_BRANCH master
ADD https://api.github.com/repos/sci-visus/OpenVisus/git/refs/heads/$GIT_BRANCH version.json 
RUN git clone -b$GIT_BRANCH https://github.com/sci-visus/OpenVisus.git $VISUS_HOME
RUN mkdir -p $VISUS_HOME/build
WORKDIR $VISUS_HOME/build 
RUN cmake ../ -DVISUS_GUI=0 -DVISUS_HOME=$VISUS_HOME -DVISUS_PYTHON_SYS_PATH=$(pwd)
RUN cmake --build . --target all -- -j 4

# configure apache
ADD .000-default.conf     /etc/apache2/sites-enabled/000-default.conf
ADD .httpd-foreground.sh  /usr/local/bin/httpd-foreground.sh
RUN echo "LoadModule visus_module $VISUS_HOME/build/libmod_visus.so" > /etc/apache2/mods-available/visus.load
RUN chmod a+x /usr/local/bin/httpd-foreground.sh 
RUN a2enmod headers 
RUN a2enmod visus 
EXPOSE 80
CMD ["/usr/local/bin/httpd-foreground.sh"]

# add a sample dataset
ADD .visus.config    $VISUS_HOME/visus.config
RUN chown -R www-data $VISUS_HOME
RUN chmod -R a+rX  $VISUS_HOME 


  
