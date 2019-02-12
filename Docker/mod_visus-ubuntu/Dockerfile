# Using multi-image technique in order to save space in final image.
#
# First image needs pip and git in order to install openvisus and clone webviewer
# Second image doesn't need to include pip or git, saving hundreds of mb.
#

#
# first image
#
FROM ubuntu:16.04 as pip_install
RUN apt-get update 
RUN apt-get install -y python3-pip
RUN python3 -m pip install --upgrade pip 
RUN python3 -m pip install numpy
RUN python3 -m pip install OpenVisus

# clone webviewer
RUN apt-get install -y git
RUN git clone https://github.com/sci-visus/visus_javascript.git /webviewer

#
# second image
#
FROM ubuntu:16.04
ENV VISUS_HOME=/home/OpenVisus
RUN apt-get update \
  && apt-get install -y apache2 python3.5 libpython3.5 \
  && ln -s /usr/bin/python3.5 /usr/bin/python3 \
  && ln -s /usr/bin/python3.5 /usr/bin/python

# copy openvisus pip distro and webviewer clone from first image
COPY --from=pip_install /usr/local/lib/python3.5/dist-packages/OpenVisus  /usr/local/lib/python3.5/dist-packages/OpenVisus
COPY --from=pip_install /usr/local/lib/python3.5/dist-packages/numpy      /usr/local/lib/python3.5/dist-packages/numpy
RUN ln -s $(python3 -c "import os, OpenVisus; print(os.path.dirname(OpenVisus.__file__))") ${VISUS_HOME}
COPY --from=pip_install /webviewer ${VISUS_HOME}/webviewer

# configure mod_visus and webviewer
COPY 000-default.conf /etc/apache2/sites-enabled
COPY .htpasswd    ${VISUS_HOME}
COPY visus.config ${VISUS_HOME}
COPY httpd-foreground.sh /usr/local/bin
RUN echo "LoadModule visus_module ${VISUS_HOME}/bin/libmod_visus.so" > /etc/apache2/mods-available/visus.load \
  && a2enmod headers \
  && a2enmod visus \
  && chmod a+x /usr/local/bin/httpd-foreground.sh

# run
EXPOSE 80
CMD ["/usr/local/bin/httpd-foreground.sh"]
