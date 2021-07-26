FROM httpd:2.4

RUN apt-get -qq update
RUN apt-get install -y git python3 python3-pip
RUN python3 -m pip install --upgrade pip
RUN python3 -m pip install --upgrade numpy

# install OpenVisus (change version as needed) and webviewer
ARG TAG
RUN python3 -m pip install --upgrade OpenVisusNoGui==$TAG
RUN python3 -c "import os,OpenVisus;os.system('rm -Rf /home/OpenVisus');os.system('ln -s {} /home/OpenVisus'.format(os.path.dirname(OpenVisus.__file__)))"
RUN git clone -bmaster https://github.com/sci-visus/OpenVisusJS.git /home/OpenVisus/webviewer

# How do I create a self-signed SSL Certificate for testing purposes:
#   see https://httpd.apache.org/docs/2.4/ssl/ssl_faq.html 
#   simpliest command to generate it: `openssl req -new -x509 -nodes -out server.crt -keyout server.key`
COPY server.crt /usr/local/apache2/conf/server.crt
COPY server.key /usr/local/apache2/conf/server.key

COPY openvisus.conf /usr/local/apache2/conf/openvisus.conf
RUN echo "Include /usr/local/apache2/conf/openvisus.conf" >> /usr/local/apache2/conf/httpd.conf

RUN echo "<visus><include url='/datasets/datasets.config' /></visus>" > /home/OpenVisus/visus.config
COPY datasets /datasets

ENV PYTHONUNBUFFERED=1
ENV VISUS_HOME=/home/OpenVisus






