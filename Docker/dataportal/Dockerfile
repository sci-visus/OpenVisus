FROM ubuntu:16.04

RUN apt-get update 
RUN apt-get install -y apache2
RUN apt-get install -y python3 python3-pip
RUN python3 -m pip install --upgrade pip 
RUN python3 -m pip install numpy

RUN python3 -m pip install OpenVisus
ENV VISUS_HOME=/home/visus
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

# install dataportal
RUN apt-get install -y git php php-xml php-curl php-json php-sqlite3 libapache2-mod-php
RUN git clone https://github.com/sci-visus/visus-dataportal ${VISUS_HOME}/dataportal
WORKDIR ${VISUS_HOME}/dataportal
RUN git submodule update --init -- ext
ADD dataportal.conf  /etc/apache2/sites-enabled/dataportal.conf
ADD local.php	      ${VISUS_HOME}/dataportal/local.php
ADD .htpasswd       ${VISUS_HOME}/.htpasswd
ADD .htaccess       ${VISUS_HOME}/dataportal/.htaccess
ADD server-tuning.conf /etc/apache2/mods-enabled/server-tuning.conf
RUN chgrp -R www-data ${VISUS_HOME}/dataportal
# visus.config needs write permissions
ADD visus.config    ${VISUS_HOME}/visus.config
RUN chgrp www-data ${VISUS_HOME}/visus.config
RUN chmod g+w       ${VISUS_HOME}/visus.config
# db needs write permissions
RUN chmod -R g+w       ${VISUS_HOME}/dataportal/db
# data folder needs write permissions
RUN mkdir /data
RUN chgrp -R www-data /data
RUN chmod -R g+w      /data
RUN mkdir /data/.trash/
RUN chgrp www-data /data/.trash/
RUN chmod g+w /data/.trash
# webviewer config
ADD config.js       ${VISUS_HOME}/dataportal/viewer/config.js
RUN  rm             /etc/apache2/sites-enabled/000-default.conf  

WORKDIR ${VISUS_HOME}