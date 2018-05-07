FROM ubuntu:16.04

# install all packages
RUN apt-get update
RUN apt-get install -y cmake git swig 
RUN apt-get install -y build-essential 
RUN apt-get install -y liblz4-dev libtinyxml-dev libfreeimage-dev libcurl4-openssl-dev libssl-dev uuid-dev 
RUN apt-get install -y python3 python3-pip 
RUN apt-get install -y apache2 apache2-dev 
RUN pip3 install --upgrade pip
RUN pip3 install numpy

ENV VISUS_HOME /home/visus

# compile OpenVisus
RUN git clone https://github.com/sci-visus/OpenVisus $VISUS_HOME
RUN mkdir -p $VISUS_HOME/build
WORKDIR $VISUS_HOME/build 
RUN cmake ../ -DVISUS_GUI=0 -DVISUS_HOME=$VISUS_HOME -DVISUS_PYTHON_SYS_PATH=$(pwd)
RUN make

# add a sample dataset
ADD ./files/visus.config    $VISUS_HOME/visus.config
ADD ../../Misc/dataset/cat  $VISUS_HOME/datasets/cat

# configure and run apache
ADD ./files/000-default.conf     /etc/apache2/sites-enabled/000-default.conf
ADD ./files/httpd-foreground.sh  /usr/local/bin/httpd-foreground.sh
ADD ./files/visus.load           /etc/apache2/mods-available/visus.load
RUN chmod a+x /usr/local/bin/httpd-foreground.sh 
RUN chown -R www-data $VISUS_HOME
RUN chmod -R a+rX  $VISUS_HOME  
RUN a2enmod headers
RUN a2enmod visus 
EXPOSE 80
CMD ["/usr/local/bin/httpd-foreground.sh"]
  
