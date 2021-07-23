# OpenViSUS Visualization project  
     
![GitHub Actions](https://github.com/sci-visus/OpenVisus/workflows/BuildOpenVisus/badge.svg)
[![Binder](https://mybinder.org/badge_logo.svg)](https://mybinder.org/v2/gh/sci-visus/OpenVisus/master?filepath=Samples%2Fjupyter)
 
 
# Mission

The mission of ViSUS.org is to provide support for the scientific community with Big Data, management, analysis and visualization tools.
In this website we provide access to open source software tools and libraries such as the ViSUS framework and the PIDX library.
These softwares are distributed under the permissive BSD license (see [LICENSE](https://github.com/sci-visus/OpenVisus/tree/master/LICENSE) file).

# Installation

If you are using conda see [docs/conda_installation.md](https://github.com/sci-visus/OpenVisus/blob/master/docs/conda_installation.md) document.

To install OpenVisus using `pip`:

```
python -m pip install --upgrade pip
python -m pip install --upgrade OpenVisus
python -m OpenVisus configure 
```

If you get *permission denied* errors, just use `python -m pip install  --user` 


Run the viewer:

```
python -m OpenVisus viewer
```

If you want to use venv to *experiment* with OpenVisus without corrupting your python:

```
python -m pip install virtualenv
python -m venv ~/my-virtual-environment
source ~/my-virtual-environment/bin/activate
```

# Compilation

See [docs/compilation.md](https://github.com/sci-visus/OpenVisus/blob/master/docs/compilation.md) document

# Server side

To run an OpenVisus server see [docs/docker_modvisus](https://github.com/sci-visus/OpenVisus/blob/master/docs/docker_modvisus.md) document.

To run multiple OpenVisus servers with a load-balancer in `Kubernetes` see [docs/kubernetes.md](https://github.com/sci-visus/OpenVisus/blob/master/docs/kubernetes.md) 


To run multiple OpenVisus servers with a load-balancer in `Docker Swarm` see [docs/docker_swarm_modvisus.md](https://github.com/sci-visus/OpenVisus/blob/master/docs/docker_swarm_modvisus.md) 


# Tutorials

Give a loot [Samples/jupyter](https://github.com/sci-visus/OpenVisus/tree/master/Samples/jupyter) in particular to [Samples/jupyter/quick_tour.ipynb](https://github.com/sci-visus/OpenVisus/blob/master/Samples/jupyter/quick_tour.ipynb).



