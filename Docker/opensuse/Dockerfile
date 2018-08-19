FROM opensuse:42.3

# install all packages
RUN zypper --non-interactive update

RUN zypper --non-interactive install --type pattern devel_basis
RUN zypper --non-interactive install  gcc-c++ cmake git swig  python3 python3-pip python3-devel libuuid-devel libopenssl-devel apache2 apache2-devel
    
RUN pip3 install --upgrade pip
RUN pip3 install numpy

# compile OpenVisus (forcing clone)
ENV VISUS_HOME /home/visus
ENV GIT_BRANCH master
ADD https://api.github.com/repos/sci-visus/OpenVisus/git/refs/heads/$GIT_BRANCH version.json 
RUN git clone -b$GIT_BRANCH https://github.com/sci-visus/OpenVisus.git $VISUS_HOME
RUN mkdir -p $VISUS_HOME/build
WORKDIR $VISUS_HOME/build 
RUN cmake ../ -DVISUS_GUI=0 -DVISUS_HOME=$VISUS_HOME -DVISUS_PYTHON_SYS_PATH=$(pwd)
RUN cmake --build . --target all -- -j 4

# configure apache ()
ADD 000-default.conf /etc/apache2/conf.d/000-default.conf
RUN echo "LoadModule visus_module $VISUS_HOME/build/libmod_visus.so" >> /etc/apache2/loadmodule.conf
RUN a2enmod visus 
ADD httpd-foreground.sh  /usr/local/bin/httpd-foreground.sh
RUN chmod a+x /usr/local/bin/httpd-foreground.sh 
#  httpd -M

# how to run apache
EXPOSE 80
CMD ["/usr/local/bin/httpd-foreground.sh"]

# add a sample dataset
ADD visus.config    $VISUS_HOME/visus.config
RUN chown -R wwwrun  $VISUS_HOME
RUN chmod -R a+rX  $VISUS_HOME 



  
