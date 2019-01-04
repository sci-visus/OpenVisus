#
# ViSUS Docker *install* for miniconda3 [Debian] environment
#
# Must build this Docker image from the OpenVisus directory (..):
#  `docker build -t OpenVisus:miniconda_install -f docker/miniconda/install/Dockerfile .`
#
# NOTES:
# - follow instructions in ../README.md to deploy this install container
#  - use miniconda_builder to build and test code, then create tgz as described in README.md
#

FROM continuumio/miniconda3

# install necessary system libraries and conda packages
RUN set -x &&\
    apt-get update &&\
    apt-get install -y libgomp1 &&\
    conda install -y numpy
#apt: maybe also...
  # libxml2 swig bzip2 ca-certificates curl build-essential apache2 python-dev libapache2-mod-php
#conda: maybe also...
  #cdms2 genutil lxml

# create installation directory
ENV VISUS_HOME=/home/OpenVisus
RUN mkdir $VISUS_HOME

# copy and extract OpenVisus, XIDX, and webviewer installations
ADD visus_miniconda_install.tgz $VISUS_HOME
ADD visus_webviewer.tgz $VISUS_HOME

# tidy up the OpenVisus installation (dump unnecessary libraries and fix rpaths)
ADD tidy_up_visus_install_for_miniconda.sh $VISUS_HOME
RUN set -x &&\
    apt-get install -y patchelf &&\
    chmod a+x $VISUS_HOME/tidy_up_visus_install_for_miniconda.sh &&\
    $VISUS_HOME/tidy_up_visus_install_for_miniconda.sh $VISUS_HOME &&\
    rm $VISUS_HOME/tidy_up_visus_install_for_miniconda.sh


# (TODO) install: update paths and enable http

ENV VISUS_HOME=/home/OpenVisus
ENV PYTHONPATH $VISUS_HOME:$PYTHONPATH
# RUN python setup.py install

#<ctc> really??
#RUN ln -s $(python3 -c "import os, OpenVisus; print(os.path.dirname(OpenVisus.__file__))") ${VISUS_HOME}

# configure apache
#ADD ../000-default.conf     /etc/apache2/sites-enabled/000-default.conf
#ADD ../httpd-foreground.sh  /usr/local/bin/httpd-foreground.sh
#RUN echo "LoadModule visus_module $VISUS_HOME/libmod_visus.so" > /etc/apache2/mods-available/visus.load
#RUN chmod a+x /usr/local/bin/httpd-foreground.sh 
#RUN a2enmod headers 
#RUN a2enmod visus

# install webviewer
# RUN git clone https://github.com/sci-visus/visus_javascript $VISUS_HOME/webviewer
#<ctc> where is webviewer.conf?
# ADD ./files/webviewer.conf  /etc/apache2/sites-enabled/webviewer.conf
#<ctc> do we really need .htpasswd?
# ADD ./files/.htpasswd       $VISUS_HOME/.htpasswd
#<ctc> wuh????
# RUN  rm                     /etc/apache2/sites-enabled/000-default.conf

#<ctc> Hmm...
# download certificate
# RUN mkdir -p /var/www/visus && \
#     cd /var/www/visus && \
#     curl --remote-name --time-cond cacert.pem https://curl.haxx.se/ca/cacert.pem && \
#     chown -R www-data .

#<ctc> Hmm...
# add a sample dataset
# ADD ./files/visus.config    $VISUS_HOME/visus.config
# RUN chown -R www-data $VISUS_HOME
# RUN chmod -R a+rX  $VISUS_HOME 

# copy apache run script
#<ctc> can't we just run `exec /usr/sbin/apache2 -DFOREGROUND` instead of all that other stuff?
#      maybe save that crap for the debug server, not the install
#      ALSO... check ondemand/code/start_service.sh since it does some other seemingly important tasks
#ADD httpd-foreground.sh  /usr/local/bin/httpd-foreground.sh
#RUN chmod a+x /usr/local/bin/httpd-foreground.sh 

#<ctc> Hmm... seriously doubt it
#RUN echo "<include url='/mnt/visus_datasets/visus.config' />" > ${VISUS_HOME}/visus.config
#RUN chmod -R 755 ${VISUS_HOME}


# (TODO) finally, set the open port and run the server
# EXPOSE 80
# CMD ["/usr/local/bin/httpd-foreground.sh"]
