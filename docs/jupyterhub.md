

# Instructions

Links:
- https://tljh.jupyter.org/en/latest/install/custom-server.html



```
sudo apt install python3 python3-dev git curl

# disable firewall for testing
sudo ufw disable


curl -L https://tljh.jupyter.org/bootstrap.py | sudo -E python3 - --admin <jupyter-hub-admin-name>
```

Open the Jupyter Hub;
- From Control Panel, Add User
- Open a new Terminal:

```

sudo -E conda install -y conda-ecosystem-user-package-isolation
sudo -E conda install -y mamba 
# sudo -E conda uinstall -y mamba 
# sudo -E conda install  -y mamba 

sudo -E mamba install -y -c conda-forge \
    numpy h5py urllib3 pillow colorcet patchelf conda boto3 pandas matplotlib scikit-image packaging jupyter \
    notebook ipykernel jupyter_bokeh jupyterlab  jupyter-server-proxy jupyter_nbextensions_configurator \
    jupyter_contrib_nbextensions ipywidgets ipympl pyviz_comms bokeh vtk voila panel

sudo -E mamba install -c visus -y openvisusnogui
sudo -E python -m OpenVisus configure
sudo -E python -m OpenVisus configure
sudo -E python -c "from OpenVisus import *"
```