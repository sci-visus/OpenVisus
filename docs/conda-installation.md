# Install OpenVisus on Conda


First you need a conda environment. Here we are showing how to install miniforge3, a lightweighted conda installation

```
curl -L -O https://github.com/conda-forge/miniforge/releases/latest/download/Miniforge3-Linux-x86_64.sh

# -b is `batch mode`
bash Miniforge3-Linux-x86_64.sh -b
```

Then you typically want the conda to automatically setup at login:

```
~/miniforge3/bin/conda init bash init bash
source ~/.bashrc
```

Create a new environment and activate it:


```
ENV_NAME=myenv
conda create --name ${ENV_NAME} -y python=3.9 mamba  
conda activate ${ENV_NAME}
```

if you want make the environment as the default:

```
cat <<EOF >>~/.bashrc
conda activate ${ENV_NAME}
EOF
```

Check python is the right one:

```
which python
# e.g. ~/miniforge3/envs/${ENV_NAME}/bin/python
```


Avoid conflicts with PIP packages in ~/.local directory (equivalent to `PYTHONNOUSERSITE=True`):

```
conda install -c conda-forge conda-ecosystem-user-package-isolation
```

Install the environment in the kernel list:

```
mamba install -y ipykernel
python -m ipykernel install --user --name "${ENV_NAME}" --display-name "${ENV_NAME}"
```

Check it has been installed:

```
jupyter kernelspec list
```

Install the packages that may be needed (this is a pretty comprensive list, customize as needed):

```
mamba install -y -c conda-forge \
  numpy \
  h5py \
  urllib3 \
  pillow \
  colorcet \
  patchelf \
  conda \
  boto3 \
  pandas \
  matplotlib \
  scikit-image \
  packaging \
  jupyter \
  notebook \
  jupyter_bokeh \
  jupyterlab \
  jupyter-server-proxy \
  jupyter_nbextensions_configurator \
  jupyter_contrib_nbextensions \
  ipywidgets \
  ipympl \
  pyviz_comms \
  bokeh \
  vtk \
  voila \
  panel
```

(OPTIONAL) if you have an old version of JupyterLab you may need to run this:

```
jupyter labextension install @pyviz/jupyterlab_pyviz
```


Install openvisus:
- NOTE: if you have problems you can install `openvisusnogui` which does not include the OpenVisus viewer

```
mamba install -c visus -y openvisus
```

Run the OpenVisus configuration step:
- configuration step will install PyQt5
- will relink OpenVisus binaries to use the PyQt5 version (using `patchelf` internally)
- if you are using the `openvisusnogui` version, configure is generally not needed

```
# first time the configure will cause segmentation fault 
python -m OpenVisus configure
python -m OpenVisus configure

# configure is doing:
# python -m pip install --upgrade pip 
# python -m pip install numpy 
# python -m pip install PyQt5~=5.12.0 PyQtWebEngine~=5.12.0 PyQt5-sip 
# ... relinking of openvisus binaries...
```

Check it is working:

```
python -c "from OpenVisus import *"
```

if you want to run a jupyter notebook:
- NOTE: check the status of the firewall (e.g., `sudo ufw status`)
- you may check connectivity using curl (e.g., `curl -v -L http://<hostname>:8888/?token=XXXX`)

```
jupyter notebook --ip="$(curl ifconfig.me)" --port 8888 --no-browser --debug \
```


