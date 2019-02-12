#
# ViSUS Docker for Anaconda environment (FROM Debian-based continuumio/miniconda3)
#
# Follow mod_visus instructions in Docker/README.md
#

FROM continuumio/miniconda3 as pip_install
ENV CONDA_PREFIX=/opt/conda
ENV VISUS_HOME=/home/OpenVisus

# update conda
RUN conda update -y -n base conda

# install and configure OpenVisus, fixup rpaths, and create symlinks in conda's bin directory
# note: visus.sh isn't necessary and doesn't work from conda bin, so just symlinking to visus
RUN pip install OpenVisus \
  && echo "export VISUS_HOME=${VISUS_HOME}" >> ~/.bashrc \
  && ln -s $(python3 -c "import os, OpenVisus; print(os.path.dirname(OpenVisus.__file__))") ${VISUS_HOME} \
  && python ${VISUS_HOME}/configure.py \
  && rm ${VISUS_HOME}/visus.sh \
  && ln -s ${VISUS_HOME}/bin/visus ${VISUS_HOME}/visus.sh \
  && ln -s ${VISUS_HOME}/visus.sh  ${CONDA_PREFIX}/bin \
  && ln -s ${VISUS_HOME}/bin/visus ${CONDA_PREFIX}/bin \
  && conda install -y patchelf \
  && ${CONDA_PREFIX}/bin/patchelf --set-rpath '$ORIGIN:/opt/conda/lib' ${VISUS_HOME}/bin/visus \
  && ${CONDA_PREFIX}/bin/patchelf --set-rpath '$ORIGIN:/opt/conda/lib' ${VISUS_HOME}/bin/libmod_visus.so


FROM continuumio/miniconda3
ENV VISUS_HOME=/home/OpenVisus

# install apache and jove (a handy, tiny emacs-like editor)
RUN set -x \
  && apt-get update \
  && apt-get install jove \
  && apt-get install -y apache2

RUN conda clean -da
COPY --from=pip_install /usr/local/lib/python3.5/dist-packages/OpenVisus  /usr/local/lib/python3.5/dist-packages/OpenVisus
COPY --from=pip_install /usr/local/lib/python3.5/dist-packages/numpy      /usr/local/lib/python3.5/dist-packages/numpy

# install webviewer
RUN set -x \
  && git clone https://github.com/sci-visus/visus_javascript.git ${VISUS_HOME}/webviewer \
  && rm /etc/apache2/sites-enabled/000-default.conf
COPY webviewer.conf  /etc/apache2/sites-enabled
COPY .htpasswd       ${VISUS_HOME}

# download certificate
RUN set -x \
  && mkdir -p /var/www/visus \
  && cd /var/www/visus \
  && curl --remote-name --time-cond cacert.pem https://curl.haxx.se/ca/cacert.pem \
  && chown -R www-data .

# configure mod_visus
COPY visus.config ${VISUS_HOME}
ADD httpd-foreground.sh  /usr/local/bin
RUN echo "LoadModule visus_module ${VISUS_HOME}/bin/libmod_visus.so" > /etc/apache2/mods-available/visus.load \
  && a2enmod headers \
  && a2enmod visus \
  && chmod a+x /usr/local/bin/httpd-foreground.sh

# run
EXPOSE 80
CMD ["/usr/local/bin/httpd-foreground.sh"]
