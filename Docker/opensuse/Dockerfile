FROM opensuse:42.3

# install all packages
RUN zypper --non-interactive update
RUN zypper --non-interactive install --type pattern devel_basis
RUN zypper --non-interactive install  gcc-c++ cmake git swig  python3 python3-pip python3-devel libuuid-devel libopenssl-devel apache2 apache2-devel
RUN pip3 install --upgrade pip
RUN pip3 install numpy

# compile OpenVisus (forcing clone) 
ENV GIT_BRANCH master
ADD https://api.github.com/repos/sci-visus/OpenVisus/git/refs/heads/$GIT_BRANCH version.json 
RUN git clone -b$GIT_BRANCH https://github.com/sci-visus/OpenVisus.git /home/visus
RUN mkdir -p /home/visus/build
WORKDIR /home/visus/build 
RUN cmake ../ -DVISUS_GUI=0 -DVISUS_HOME=/home/visus -DVISUS_PYTHON_SYS_PATH=$(pwd)
RUN cmake --build . --target all -- -j 4

# configure apache 
ADD 000-default.conf /etc/apache2/conf.d/000-default.conf
RUN echo "LoadModule visus_module /home/visus/build/libmod_visus.so" >> /etc/apache2/loadmodule.conf
RUN a2enmod visus 
ADD httpd-foreground.sh  /usr/local/bin/httpd-foreground.sh
RUN chmod a+x /usr/local/bin/httpd-foreground.sh 

# fix LD_LIBRARY_PATH problem
RUN echo "/home/visus/build" >> /etc/ld.so.conf 
RUN ldconfig

# setup visus.config to redirect to your visus.config
RUN echo "<include url='/visus_datasets/visus.config' />" > /home/visus/visus.config
RUN chown -R wwwrun  /home/visus 
RUN chmod -R a+rX    /home/visus 

# run apache
EXPOSE 80
CMD ["/usr/local/bin/httpd-foreground.sh"]





