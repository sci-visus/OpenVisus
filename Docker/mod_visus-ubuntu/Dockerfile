FROM ubuntu:16.04
RUN apt-get update 
RUN apt-get install -y python3-pip
RUN python3 -m pip install --upgrade pip 
RUN python3 -m pip install numpy
RUN python3 -m pip install OpenVisus

FROM ubuntu:16.04
RUN apt-get update \
  && apt-get install -y apache2 python3.5 libpython3.5 \
  && ln -s /usr/bin/python3.5 /usr/bin/python3 \
  && ln -s /usr/bin/python3.5 /usr/bin/python
COPY --from=0 /usr/local/lib/python3.5/dist-packages/OpenVisus  /usr/local/lib/python3.5/dist-packages/OpenVisus
COPY --from=0 /usr/local/lib/python3.5/dist-packages/numpy      /usr/local/lib/python3.5/dist-packages/numpy

ENV VISUS_HOME=/home/OpenVisus
RUN ln -s $(python3 -c "import os, OpenVisus; print(os.path.dirname(OpenVisus.__file__))") ${VISUS_HOME}

# configure mod_visus
COPY 000-default.conf /etc/apache2/sites-enabled
COPY visus.config ${VISUS_HOME}
COPY httpd-foreground.sh  /usr/local/bin
RUN echo "LoadModule visus_module ${VISUS_HOME}/bin/libmod_visus.so" > /etc/apache2/mods-available/visus.load \
  && a2enmod headers \
  && a2enmod visus \
  && chmod a+x /usr/local/bin/httpd-foreground.sh

# run
EXPOSE 80
CMD ["/usr/local/bin/httpd-foreground.sh"]
