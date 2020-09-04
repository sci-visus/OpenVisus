# to build it:
#    sudo docker build --tag modvisus_opensuse -f Dockerfile.opensuse .

# to run it:
#    sudo docker run   --publish 8080:80 modvisus_opensuse
#    wget -q -O -  "http://localhost:8080/mod_visus?action=list"

# if you need to debug:
#    sudo docker run -it -w=/home/OpenVisus --entrypoint  /bin/bash modvisus_opensuse
#    /usr/local/bin/httpd-foreground.sh
#    more /var/log/apache2/error_log

FROM opensuse/leap

ENV PYTHONUNBUFFERED=1
ENV VISUS_HOME=/home/OpenVisus

RUN zypper --non-interactive update 
RUN zypper --non-interactive install python3 python3-pip python3-devel apache2 git  
RUN python3 -m pip install --upgrade pip OpenVisus
RUN python3 -c "import os,OpenVisus;os.system('rm -Rf /home/OpenVisus');os.system('ln -s {} /home/OpenVisus'.format(os.path.dirname(OpenVisus.__file__)))"

ADD https://api.github.com/repos/sci-visus/OpenVisusJS/git/refs/heads/master version.json
RUN git clone -bmaster https://github.com/sci-visus/OpenVisusJS.git /home/OpenVisus/webviewer

COPY 000-default.conf     /etc/apache2/conf.d/
COPY .htpasswd            /home/OpenVisus/
COPY visus.config         /home/OpenVisus/
COPY httpd-foreground.sh  /usr/local/bin/
RUN chmod a+x /usr/local/bin/httpd-foreground.sh

RUN echo "LoadModule headers_module   /usr/lib64/apache2-prefork/mod_headers.so"   >>   /etc/apache2/loadmodule.conf 
RUN echo "LoadModule visus_module     /home/OpenVisus/bin/libmod_visus.so"         >>   /etc/apache2/loadmodule.conf 

EXPOSE 80
CMD ["/usr/local/bin/httpd-foreground.sh"]





