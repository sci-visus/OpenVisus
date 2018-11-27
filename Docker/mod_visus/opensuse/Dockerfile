FROM opensuse:42.3

RUN zypper --non-interactive update
RUN zypper --non-interactive install python3 python3-pip apache2 
RUN python3 -m pip install --upgrade pip 
RUN python3 -m pip install numpy

RUN python3 -m pip install OpenVisus
ENV VISUS_HOME=/home/OpenVisus
RUN ln -s $(python3 -c "import os, OpenVisus; print(os.path.dirname(OpenVisus.__file__))") ${VISUS_HOME}

# configure apache 
ADD 000-default.conf /etc/apache2/conf.d/000-default.conf
RUN echo "LoadModule visus_module ${VISUS_HOME}/bin/libmod_visus.so" >> /etc/apache2/loadmodule.conf
RUN a2enmod visus 

ADD httpd-foreground.sh  /usr/local/bin/httpd-foreground.sh
RUN chmod a+x /usr/local/bin/httpd-foreground.sh 

# setup visus.config to redirect to your visus.config
RUN echo "<include url='/mnt/visus_datasets/visus.config' />" > ${VISUS_HOME}/visus.config
RUN chmod -R 755 ${VISUS_HOME}

EXPOSE 80
CMD ["/usr/local/bin/httpd-foreground.sh"]





