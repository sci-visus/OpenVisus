#
# ViSUS Docker *install* for miniconda3 [Debian] environment
#
# Must build this Docker image from the OpenVisus directory (..):
#  `docker build -t OpenVisus:miniconda_install -f docker/miniconda/installer/Dockerfile .`
#
# NOTES:
# - use miniconda_builder to build and test code, then create tgzs and Docker installer
# - See detailed instructions in ../README.md
# - on startup, runs apache in the foreground, start w/ `docker run -it /bin/bash otherwise
#

FROM continuumio/miniconda3

# install all packages, update conda and pip
RUN set -x \
  && apt-get update \
  && apt-get install -y libgomp1 apache2 \
#apt: maybe also...
  # libxml2 swig bzip2 ca-certificates build-essential python-dev libapache2-mod-php
  && conda update -y -n base conda \
  && conda install -y numpy \
  && python3 -m pip install --upgrade pip
#conda: maybe also... (cdms2 should be in ondemand's Dockerfile)
  # cdms2 genutil lxml 

# set environment variables
ENV VISUS_HOME=/home/OpenVisus
ENV PYTHONPATH $VISUS_HOME:$PYTHONPATH

# create installation directory, copy and extract OpenVisus, XIDX, and webviewer installations
RUN mkdir $VISUS_HOME \
  && printf "export PATH=$VISUS_HOME/bin:$PATH\n" >> ~/.bashrc
ADD visus_miniconda_install.tgz $VISUS_HOME
ADD visus_webviewer.tgz $VISUS_HOME

# tidy up the OpenVisus installation (dump unnecessary libraries and fix rpaths)
ADD tidy_up_visus_install_for_miniconda.sh $VISUS_HOME
RUN set -x \
  && conda install -y patchelf \
  && chmod a+x $VISUS_HOME/tidy_up_visus_install_for_miniconda.sh \
  && $VISUS_HOME/tidy_up_visus_install_for_miniconda.sh $VISUS_HOME \
  && rm $VISUS_HOME/tidy_up_visus_install_for_miniconda.sh

# not super necessary, but...
# create and activate visus conda env (and make it the default)
RUN set -x \
  && conda create -n visus \
  && sed -i 's/conda activate base/conda activate visus/' ~/.bashrc \
  && conda activate visus

# setup OpenVisus (maybe this should/needs be done)
# RUN python setup.py install

# configure mod_visus (should this be a script instead?)
ADD 000-default.conf /etc/apache2/sites-enabled/000-default.conf
ADD httpd-foreground.sh  /usr/local/bin/httpd-foreground.sh
RUN echo "LoadModule visus_module ${VISUS_HOME}/bin/libmod_visus.so" > /etc/apache2/mods-available/visus.load \
  && a2enmod headers \
  && a2enmod visus \
  && chmod a+x /usr/local/bin/httpd-foreground.sh \
# I suspect /mnt/visus_datasets doesn't exist...
  && echo "<include url='/mnt/visus_datasets/visus.config' />" > ${VISUS_HOME}/visus.config \
  && chmod -R 755 ${VISUS_HOME}


# download certificate
# RUN mkdir -p /var/www/visus && \
#     cd /var/www/visus && \
#     curl --remote-name --time-cond cacert.pem https://curl.haxx.se/ca/cacert.pem && \
#     chown -R www-data .

# add a sample dataset
# ADD ./files/visus.config    $VISUS_HOME/visus.config
# RUN chown -R www-data $VISUS_HOME
# RUN chmod -R a+rX  $VISUS_HOME 


EXPOSE 80
# NOTE: check ondemand/code/start_service.sh since it does some other seemingly important tasks
CMD ["/usr/local/bin/httpd-foreground.sh"]


