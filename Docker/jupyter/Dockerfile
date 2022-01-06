FROM jupyter/scipy-notebook:hub-2.0.1

# make sure the architecture is right
RUN uname -m

# for pip
# python -m pip install boto3 ipywidgets ipympl ipycanvas matplotlib numpy pandas  pillow scipy scikit-image urllib3 bokeh mpl_interactions paramiko bs4
# python -m pip install libglu patchelf
# python -m jupyter nbextension enable --py widgetsnbextension

RUN conda install --quiet -y -c conda-forge \
	boto3 \
	ipywidgets \
	ipympl  \
	ipycanvas \
	libglu \
	matplotlib \
	numpy \
	pandas \
	patchelf \
	pillow \
	scipy \
	scikit-image \
	urllib3 \
	bokeh

ARG TAG=2.1.188
RUN conda install -y --channel visus openvisusnogui=$TAG

ENV CONDA_PREFIX /opt/conda 
RUN python -m OpenVisus configure

COPY templates/ /etc/jupyterhub/custom/templates/
RUN ls /etc/jupyterhub/custom/templates















