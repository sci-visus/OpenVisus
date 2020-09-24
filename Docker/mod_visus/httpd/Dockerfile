FROM httpd:2.4

ENV PYTHONUNBUFFERED=1
ENV VISUS_HOME=/home/OpenVisus

RUN apt-get update
RUN apt-get install -y git python3 python3-pip
RUN python3 -c "import sys;print(sys.version_info)"
RUN python3 -m pip install --upgrade pip
RUN python3 -m pip install --upgrade numpy 
RUN python3 -m pip install --upgrade OpenVisus
RUN python3 -c "import os,OpenVisus;os.system('rm -Rf /home/OpenVisus');os.system('ln -s {} /home/OpenVisus'.format(os.path.dirname(OpenVisus.__file__)))"

ADD https://api.github.com/repos/sci-visus/OpenVisusJS/git/refs/heads/master version.json
RUN git clone -bmaster https://github.com/sci-visus/OpenVisusJS.git /home/OpenVisus/webviewer

RUN echo "LoadModule status_module modules/mod_status.so"              >> /usr/local/apache2/conf/httpd.conf
RUN echo "LoadModule visus_module /home/OpenVisus/bin/libmod_visus.so" >> /usr/local/apache2/conf/httpd.conf
RUN echo "Include /usr/local/apache2/conf/000-default.conf"            >> /usr/local/apache2/conf/httpd.conf

COPY 000-default.conf                       /usr/local/apache2/conf/
COPY index.html                             /usr/local/apache2/htdocs/
COPY visus.config                           /home/OpenVisus/
COPY httpd-foreground                       /usr/local/bin/

# example about adding a OpenVisus datasets, if you want to add your own simply
# mount a new /datasets directory with a datasets.config file in it
# you can add a `.htpasswd` file in it
COPY datasets.config     /datasets/





