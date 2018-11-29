FROM alpine:3.7

RUN apk add gcc gfortran build-base  python3  python3-dev
RUN python3 -m pip install --upgrade pip 
RUN python3 -m pip install numpy

# broken here! alpine does not support pip binary manylinux wheel
# TODO: recompilation from scratch?

RUN python3 -m pip install OpenVisus
ENV VISUS_HOME=/home/OpenVisus
RUN ln -s $(python3 -c "import os, OpenVisus; print(os.path.dirname(OpenVisus.__file__))") ${VISUS_HOME}

RUN mkdir -p /etc/apache2/vhosts.d 
ADD 000-default.conf /etc/apache2/vhosts.d/000-default.conf
RUN echo "LoadModule visus_module ${VISUS_HOME}/bin/libmod_visus.so" >> /etc/apache2/httpd.conf 
RUN echo "Include /etc/apache2/vhosts.d/*.conf" >> /etc/apache2/httpd.conf 

ADD .httpd-foreground.sh  /usr/local/bin/httpd-foreground.sh
RUN chmod a+x /usr/local/bin/httpd-foreground.sh 

RUN echo "<include url='/mnt/visus_datasets/visus.config' />" > ${VISUS_HOME}/visus.config
RUN chown -R apache:apache ${VISUS_HOME}
RUN chmod -R 755 ${VISUS_HOME}

EXPOSE 80
CMD ["/usr/local/bin/httpd-foreground.sh"]
