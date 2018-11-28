FROM ubuntu:16.04

RUN apt-get update 
RUN apt-get install -y apache2
RUN apt-get install -y python3 python3-pip
RUN python3 -m pip install --upgrade pip 
RUN python3 -m pip install numpy

RUN python3 -m pip install OpenVisus
ENV VISUS_HOME=/home/OpenVisus
RUN ln -s $(python3 -c "import os, OpenVisus; print(os.path.dirname(OpenVisus.__file__))") ${VISUS_HOME}

ADD 000-default.conf /etc/apache2/sites-enabled/000-default.conf
RUN echo "LoadModule visus_module ${VISUS_HOME}/bin/libmod_visus.so" > /etc/apache2/mods-available/visus.load
RUN a2enmod headers 
RUN a2enmod visus 

ADD httpd-foreground.sh  /usr/local/bin/httpd-foreground.sh
RUN chmod a+x /usr/local/bin/httpd-foreground.sh 

RUN echo "<include url='/mnt/visus_datasets/visus.config' />" > ${VISUS_HOME}/visus.config
RUN chmod -R 755 ${VISUS_HOME}

EXPOSE 80
CMD ["/usr/local/bin/httpd-foreground.sh"]

  