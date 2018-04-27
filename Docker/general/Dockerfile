FROM ubuntu:16.04

MAINTAINER Giorgio Scorzelli <scrgiorgio@gmail.com>

RUN set -x \
  && apt-get -y update \
  && apt-get -y install \
    apache2 \
    libfreeimage3 \
    libcurl3 \
    curl \
    python3-dev \
    python3-numpy \    
    unzip

RUN  mkdir -p /var/www/visus && \
     cd /var/www/visus && \
     curl --remote-name --time-cond cacert.pem https://curl.haxx.se/ca/cacert.pem && \
     chown -R www-data .

ADD  envvars              /etc/apache2/
ADD  visus.load           /etc/apache2/mods-available/
RUN  rm                   /etc/apache2/sites-enabled/000-default.conf
ADD  visus_config         /visus/config
ADD  visus_datasets       /visus/datasets
ADD  apache2.tgz          /visus/
RUN  chown -R www-data    /visus
ADD  usr_local_visus      /usr/local/visus
ADD  start_server.sh      /visus
RUN  cd /etc/apache2 \
     && sed -i "s#Include ports.conf#Include /tmp/apache2/ports.conf#" apache2.conf \
     && echo "IncludeOptional /tmp/apache2/sites-enabled/*.conf" >> apache2.conf \
     && cd /visus \
     && chmod u+x start_server.sh \
     && curl https://codeload.github.com/sci-visus/visus_javascript/zip/master -o master.zip \
     && unzip master.zip \
     && rm master.zip \
     && mv visus_javascript-master webviewer

ENV PATH="/usr/local/visus/:${PATH}"

RUN chmod a+x /usr/local/visus/*.so \
  && chown www-data /visus \
  && chmod -R a+rX  /visus \
  && rm -r /var/lib/apt/lists/*
RUN a2enmod headers
RUN a2enmod visus

WORKDIR /usr/local/visus
RUN python3 setup.py install

WORKDIR /visus

EXPOSE 80
CMD ["/visus/start_server.sh"]
