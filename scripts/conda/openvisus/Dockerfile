# to run it:
# cd ./conda/openvisus
# sudo docker build .

FROM continuumio/miniconda3

ARG GIT_BRANCH=master

RUN apt-get update -y
RUN apt-get -qq install -y git apache2 apache2-dev
RUN conda install -y conda-build
RUN conda update  -y conda conda-build

COPY bld.bat    /tmp/conda/recipes/openvisus/
COPY build.sh   /tmp/conda/recipes/openvisus/
COPY LICENSE    /tmp/conda/recipes/openvisus/
COPY meta.yaml  /tmp/conda/recipes/openvisus/

WORKDIR /tmp/conda/recipes

RUN conda-build openvisus

RUN conda install --use-local -y openvisus

RUN cd $(python -m OpenVisus dirname) && \
	python Samples/python/Array.py && \
	python Samples/python/Dataflow.py && \
	python Samples/python/Idx.py














