FROM opensuse:42.3

# install all packages
RUN zypper --non-interactive update
RUN zypper --non-interactive in -t pattern devel_basis
RUN zypper --non-interactive in gcc-c++
RUN zypper --non-interactive in cmake git swig 
RUN zypper --non-interactive in python3 python3-pip python3-devel 
RUN zypper --non-interactive in zlib-devel liblz4-devel tinyxml-devel libuuid-devel libfreeimage-devel libcurl-devel libopenssl-devel  
RUN zypper --non-interactive in apache2 apache2-devel
RUN pip3 install --upgrade pip
RUN pip3 install numpy

ENV VISUS_HOME /home/visus

# compile OpenVisus
RUN echo "force clone 1" > /dev/null 
RUN git clone https://github.com/sci-visus/OpenVisus $VISUS_HOME
RUN mkdir -p $VISUS_HOME/build
WORKDIR $VISUS_HOME/build 
RUN cmake ../ -DVISUS_GUI=0 -DVISUS_HOME=$VISUS_HOME -DVISUS_PYTHON_SYS_PATH=$(pwd)
RUN make

RUN zypper --non-interactive in nano

# configure apache ()
ADD ./files/000-default.conf /etc/apache2/conf.d/000-default.conf
RUN echo "LoadModule visus_module /home/visus/build/libmod_visus.so" >> /etc/apache2/loadmodule.conf
RUN a2enmod visus 
ADD ./files/httpd-foreground.sh  /usr/local/bin/httpd-foreground.sh
RUN chmod a+x /usr/local/bin/httpd-foreground.sh 
#  httpd -M

# how to run apache
EXPOSE 80
CMD ["/usr/local/bin/httpd-foreground.sh"]

# add a sample dataset
RUN mkdir $VISUS_HOME/datasets
RUN cp -R /home/visus/Misc/dataset/cat $VISUS_HOME/datasets/cat
ADD ./files/visus.config    $VISUS_HOME/visus.config
RUN chown -R wwwrun  $VISUS_HOME
RUN chmod -R a+rX  $VISUS_HOME 



  
