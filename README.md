# OpenViSUS Visualization project  

![GitHub Actions](https://github.com/sci-visus/OpenVisus/workflows/BuildOpenVisus/badge.svg)
[![Binder](https://mybinder.org/badge_logo.svg)](https://mybinder.org/v2/gh/sci-visus/OpenVisus/master?filepath=Samples%2Fjupyter)

 
## Mission

The mission of ViSUS.org is to provide support for the scientific community with Big Data, management, analysis and visualization tools.

In this website we provide access to open source software tools and libraries such as the ViSUS framework and the PIDX library.
These softwares are distributed under the permissive BSD license (see [LICENSE](https://github.com/sci-visus/OpenVisus/tree/master/LICENSE) file).

## Installation

For `conda` see [docs/conda-installation.md](./docs/conda-installation.md).

Make sure `pip` is [installed, updated and in PATH](https://pip.pypa.io/en/stable/installation/).

```bash
pip install --upgrade OpenVisus
# configure OpenVisus (one time)
python -m OpenVisus configure 
# test installation
python -c "from OpenVisus import *"
```

Notes:

- if you get *permission denied* error, use `pip install --user`.
- if you need a *minimal installation* without the GUI replace `OpenVisus` with `OpenVisusNoGui`
- If you want to create an isolated *virtual environment* with [`virtualenv`](https://pip.pypa.io/en/stable/installation/):
- 
```bash
# make sure venv is latest
pip install --upgrade virtualenv
# create a virtual environment in current directory
venv ./ovenv
# activate the virtual environment
source ./ovenv/bin/activate
```

Run the OpenVisus viewer:

```bash
python -m OpenVisus viewer
```

## Handle with PyQt errors

Sometimes, PyQt (or other packages like `pyqt5-sip`) is already installed in system and OpenVisus viewer gets confused which package to use. 

To solve that issue, follow these steps before main installation:

- If on linux, make sure PyQt5 or any of it's related packages are not installed system-wide.
  - For Ubuntu use `sudo apt remove python3-pyqt5` to remove pyqt5 and all other related packages listed [here](https://launchpad.net/ubuntu/+source/pyqt5).
  - For any Arch based distro, use `sudo pacman -Rs python-pyqt5`. Same for all other packages like `python-pyqt5-sip`.
- Remove all pyqt5 packages with pip:
  - `pip uninstall pyqt5 PyQt5-sip`

## Documentation

You can find OpenViSUS documentation regarding the install, configuration, viewer, and Python package [here](https://sci-visus.github.io/OpenVisus/).

## Quick Tour and Tutorials

Start with 
[quick_tour.ipynb](./Samples/jupyter/quick_tour.ipynb) 
Jupyter Notebook.

See 
[Samples/jupyter](./Samples/jupyter)
directory. 

To run the tutorials on the cloud click this [binder link](https://mybinder.org/v2/gh/sci-visus/OpenVisus/master?filepath=Samples%2Fjupyter).


## Other documentation

Run single `Docker` OpenVisus server:  

- [Docker/mod_visus/ReadMe.md](./Docker/mod_visus/ReadMe.md).

Run load-balanced `Docker Swarm` OpenVisus servers: 

- [Docker/mod_visus/ReadMe.swarm.md](./Docker/mod_visus/ReadMe.swarm.md).

Run `Docker` OpenVisus server with group security:

- [Docker/mod_visus/group-security/ReadMe.md](./Docker/mod_visus/group-security/ReadMe.md).

Debug mod_visus:

- [docs/debug-modvisus.md](./docs/debug-modvisus.md).

Runload-balanced `Kubernetes` OpenVisus servers:

- [docs/kubernetes.md](./docs/kubernetes.md)

Compile OpenVisus:

- [docs/compilation.md](./docs/compilation.md).

Convert to OpenVisus file format, and similar:

- [docs/convert.md](./docs/convert.md)
- [docs/compression.md](./docs/compression.md)

Convert to using a proxy:

- [docs/connect-proxy.md](./docs/connect-proxy.md).

Enable IDX2:

- [docs/IDX2.md](./docs/IDX2.md).
