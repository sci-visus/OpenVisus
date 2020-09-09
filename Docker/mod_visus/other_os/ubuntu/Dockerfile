# to build it:
#    sudo docker build --tag mod_visus_ubuntu  .
#
# to run it:
#    sudo docker run   --publish 8080:80 mod_visus_ubuntu
#    wget -q -O -  "http://localhost:8080/mod_visus?action=list"
#
# if you need to debug:
#    sudo docker run -it --entrypoint /bin/bash -w=/home/OpenVisus mod_visus_ubuntu
#    httpd-foreground.sh
#    more /var/log/apache2/error.log

FROM ubuntu:18.04

ENV PYTHONUNBUFFERED=1
ENV VISUS_HOME=/home/OpenVisus

RUN apt-get update
RUN apt-get install -y git apache2 
RUN apt-get install -y python3 python3-pip
RUN python3 -m pip install --upgrade pip OpenVisus
RUN python3 -c "import os,OpenVisus;os.system('rm -Rf /home/OpenVisus');os.system('ln -s {} /home/OpenVisus'.format(os.path.dirname(OpenVisus.__file__)))"

ADD https://api.github.com/repos/sci-visus/OpenVisusJS/git/refs/heads/master version.json
RUN git clone -bmaster https://github.com/sci-visus/OpenVisusJS.git /home/OpenVisus/webviewer

COPY 000-default.conf /etc/apache2/sites-enabled/
COPY .htpasswd    /home/OpenVisus
COPY visus.config /home/OpenVisus
COPY httpd-foreground.sh /usr/local/bin

RUN echo "LoadModule visus_module /home/OpenVisus/bin/libmod_visus.so" > /etc/apache2/mods-available/visus.load
RUN a2enmod headers
RUN a2enmod visus 
RUN chmod a+x /usr/local/bin/httpd-foreground.sh

EXPOSE 80
CMD ["/usr/local/bin/httpd-foreground.sh"]
