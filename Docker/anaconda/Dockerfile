#
# OpenVisus Docker for Anaconda environment
# Follow mod_visus instructions in Docker/README.md
#
# Using multi-image technique in order to save space in final image.
#  First image needs pip and git in order to install openvisus and clone webviewer
#  Second image doesn't need to include pip or git, saving hundreds of mb.
#

#
# first image
#
FROM continuumio/miniconda3 as pip_install
ENV VISUS_HOME=/home/OpenVisus
ENV CONDA_PREFIX=/opt/conda

# install and configure OpenVisus, fixup rpaths
# (note: visus.sh isn't necessary and doesn't work from conda bin, but might be expected, so just symlinking to visus)
RUN pip install OpenVisus
RUN ln -s $(python3 -c "import os, OpenVisus; print(os.path.dirname(OpenVisus.__file__))") ${VISUS_HOME} \
  && python ${VISUS_HOME}/configure.py \
  && rm ${VISUS_HOME}/visus.sh \
  && ln -s ${VISUS_HOME}/bin/visus ${VISUS_HOME}/visus.sh

RUN conda install -y patchelf \
  && ${CONDA_PREFIX}/bin/patchelf --set-rpath '$ORIGIN:/opt/conda/lib' ${VISUS_HOME}/bin/visus \
  && ${CONDA_PREFIX}/bin/patchelf --set-rpath '$ORIGIN:/opt/conda/lib' ${VISUS_HOME}/bin/libmod_visus.so

# clone webviewer
RUN git clone https://github.com/sci-visus/visus_javascript.git /webviewer

#
# second image
#
FROM continuumio/miniconda3
ENV VISUS_HOME=/home/OpenVisus
ENV CONDA_PREFIX=/opt/conda

# update conda
RUN conda update -y -n base conda

# install apache and jove (a handy, tiny emacs-like editor)
RUN apt-get update \
  && apt-get install jove \
  && apt-get install -y apache2

# copy openvisus pip distro and numpy from first image
COPY --from=pip_install /opt/conda/lib/python3.7/site-packages/OpenVisus  /opt/conda/lib/python3.7/site-packages/OpenVisus
COPY --from=pip_install /opt/conda/lib/python3.7/site-packages/numpy      /opt/conda/lib/python3.7/site-packages/numpy

# symlink visus install dir to VISUS_HOME and create symlinks in conda's bin directory
RUN ln -s $(python3 -c "import os, OpenVisus; print(os.path.dirname(OpenVisus.__file__))") ${VISUS_HOME} \
  && echo "export VISUS_HOME=${VISUS_HOME}" >> ~/.bashrc \
  && ln -s ${VISUS_HOME}/visus.sh  ${CONDA_PREFIX}/bin \
  && ln -s ${VISUS_HOME}/bin/visus ${CONDA_PREFIX}/bin

# copy webviewer clone from first image
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

