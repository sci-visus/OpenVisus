FROM httpd:2.4

# make sure the architecture is right
RUN uname -m

ARG PYTHON_VERSION=3.9

RUN apt-get update --no-install-recommends
RUN apt-get install --no-install-recommends -y wget unzip python${PYTHON_VERSION} python${PYTHON_VERSION}-dev libpython${PYTHON_VERSION} python3-pip python${PYTHON_VERSION}-numpy && \
	python${PYTHON_VERSION} -m pip install --no-cache-dir --upgrade pip && \
	python${PYTHON_VERSION} -m pip install --no-cache-dir --upgrade numpy && \
	apt-get clean && \
	rm -rf /var/lib/apt/lists/*

# install OpenVisus (change version as needed)
ARG TAG
RUN echo "install OpenVisus PYTHON_VERSION=${PYTHON_VERSION} TAG={$TAG}"
RUN python${PYTHON_VERSION} -m pip install --no-cache-dir  --upgrade OpenVisusNoGui==$TAG
RUN python${PYTHON_VERSION} -c "import os,OpenVisus;os.system('rm -Rf /home/OpenVisus');os.system('ln -s {} /home/OpenVisus'.format(os.path.dirname(OpenVisus.__file__)))"

# install webviewer
RUN wget https://github.com/sci-visus/OpenVisusJS/archive/refs/heads/master.zip && \
	unzip master.zip && \
	mv OpenVisusJS-master /home/OpenVisus/webviewer && \
	rm master.zip 

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






