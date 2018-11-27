FROM opensuse:42.3

RUN zypper --non-interactive update
RUN zypper --non-interactive install python3 python3-pip apache2 
RUN pip3 install --upgrade pip
RUN pip3 install numpy OpenVisus

RUN mkdir -p /home/OpenVisus 
RUN ln -s $(python3 -c "import os,OpenVisus; os.path.dirname(OpenVisus.__file__)) /home/OpenVisus

# configure apache 
ADD 000-default.conf /etc/apache2/conf.d/000-default.conf
RUN echo "LoadModule OpenVisus_module /home/OpenVisus/bin/libmod_visus.so" >> /etc/apache2/loadmodule.conf
RUN a2enmod OpenVisus 
ADD httpd-foreground.sh  /usr/local/bin/httpd-foreground.sh
RUN chmod a+x /usr/local/bin/httpd-foreground.sh 

# fix LD_LIBRARY_PATH problem
RUN echo "/home/OpenVisus/bin" >> /etc/ld.so.conf 
RUN ldconfig

# setup visus.config to redirect to your visus.config
RUN echo "<include url='/mnt/visus_datasets/visus.config' />" > /home/OpenVisus/visus.config
RUN chown -R wwwrun  /home/OpenVisus
RUN chmod -R a+rX    /home/OpenVisus

# run apache
EXPOSE 80
CMD ["/usr/local/bin/httpd-foreground.sh"]





