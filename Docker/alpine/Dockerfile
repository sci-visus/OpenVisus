FROM alpine:3.7

RUN apk add --update alpine-sdk 
RUN apk add cmake python3 python3-dev swig apache2 apache2-dev curl curl-dev zlib zlib-dev
RUN pip3 install --upgrade pip
RUN pip3 install numpy

# compile OpenVisus (forcing clone) 
ENV GIT_BRANCH master
ADD https://api.github.com/repos/sci-visus/OpenVisus/git/refs/heads/$GIT_BRANCH version.json 
RUN curl -L -o OpenVisus.zip http://github.com/sci-visus/OpenVisus/zipball/$GIT_BRANCH
RUN unzip OpenVisus.zip 
RUN mv sci-visus-OpenVisus*/ /home/visus

RUN mkdir -p /home/visus/build && \
	cd /home/visus/build && \
	cmake ../ \
		-DVISUS_INTERNAL_ZLIB=0 \
		-DVISUS_INTERNAL_LZ4=1 \
		-DVISUS_INTERNAL_TINYXML=1 \
		-DVISUS_INTERNAL_FREEIMAGE=1 \
		-DVISUS_INTERNAL_OPENSSL=0 \
		-DVISUS_INTERNAL_CURL=0 \
		-DVISUS_INTERNAL_PYTHON=0 \
		-DVISUS_GUI=0 \
		-DVISUS_HOME=/home/visus \
		-DVISUS_PYTHON_SYS_PATH=$(pwd) && \
	cmake --build . --target all -- -j 4

# configure apache 
RUN mkdir /etc/apache2/vhost.d 
RUN echo "LoadModule visus_module /home/visus/build/libmod_visus.so" >> /etc/apache2/httpd.conf 
RUN echo "IncludeOptional /etc/apache2/vhost.d/*.conf"               >> /etc/apache2/httpd.conf 
ADD 000-default.conf /etc/apache2/vhost.d/000-default.conf

# setup visus.config to redirect to your visus.config
RUN echo "<include url='/visus_datasets/visus.config' />" > /home/visus/visus.config
RUN chown -R apache:apache /home/visus 
RUN chmod -R a+rX /home/visus 

ADD httpd-foreground.sh /usr/local/bin/httpd-foreground.sh
RUN chmod a+x /usr/local/bin/httpd-foreground.sh 

# run apache
EXPOSE 80
CMD ["/usr/local/bin/httpd-foreground.sh"]





