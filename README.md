# OpenViSUS Visualization project  
     
![GitHub Actions](https://github.com/sci-visus/OpenVisus/workflows/BuildOpenVisus/badge.svg)
[![Binder](https://mybinder.org/badge_logo.svg)](https://mybinder.org/v2/gh/sci-visus/OpenVisus/master?filepath=Samples%2Fjupyter)

 
# Mission

The mission of ViSUS.org is to provide support for the scientific community with Big Data, management, analysis and visualization tools.

In this website we provide access to open source software tools and libraries such as the ViSUS framework and the PIDX library.
These softwares are distributed under the permissive BSD license (see [LICENSE](https://github.com/sci-visus/OpenVisus/tree/master/LICENSE) file).

# Installation

For `conda` see [docs/conda_installation.md](./docs/conda_installation.md).

Make sure `pip` is [installed, updated and in PATH](https://pip.pypa.io/en/stable/installation/). The installation is tested on python3.7, so it is recommended. 

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

__PyQt errors__:
Sometimes, PyQt (or other packages like `pyqt5-sip`) is already installed in system and OpenVisus viewer gets confused which package to use. To solve that issue, follow these steps before main installation:
- If on linux, make sure PyQt5 or any of it's related packages are not installed system-wide.
  - For Ubuntu use `sudo apt remove python3-pyqt5` to remove pyqt5 and all other related packages listed [here](https://launchpad.net/ubuntu/+source/pyqt5).
  - For any Arch based distro, use `sudo pacman -Rs python-pyqt5`. Same for all other packages like `python-pyqt5-sip`.
- Remove all pyqt5 packages with pip:
  - `pip uninstall pyqt5 PyQt5-sip`

# Documentation

You can find OpenViSUS documentation regarding the install, configuration, viewer, and Python package [here](https://sci-visus.github.io/OpenVisus/).

# Quick Tour and Tutorials

Start with 
[quick_tour.ipynb](./Samples/jupyter/quick_tour.ipynb) 
Jupyter Notebook.

See 
[Samples/jupyter](./Samples/jupyter)
directory. 

To run the tutorials on the cloud click this [binder link](https://mybinder.org/v2/gh/sci-visus/OpenVisus/master?filepath=Samples%2Fjupyter).


# Run OpenVisus server

Run single `Docker` OpenVisus server:  
[docs/docker_modvisus](./docs/docker_modvisus.md).

Runload-balanced `Kubernetes` OpenVisus servers: 
[docs/kubernetes.md](./docs/kubernetes.md).


Run load-balanced `Docker Swarm` OpenVisus servers: 
[docs/docker_swarm_modvisus.md](./docs/docker_swarm_modvisus.md).



# Compilation

See [docs/compilation.md](./docs/compilation.md).

# Convert (and similar)

See [docs/convert.md](./docs/convert.md).

# Connecting to OpenViSUS Server Using a Proxy

In your visus.config, you can specify a proxy scheme, ip, and port for the client (either the viewer or the python package) to use when connecting to an OpenViSUS server. This can be useful in accessing a server that is hosted on an internal network which is only accessible through SSH, for example.

An example visus.config file containing proxy information would look like this:

```
<visus>
	<Configuration>
		<NetService proxy="socks5://localhost" proxyport="55051"/>
	</Configuration>
	... (The rest of the config follows)
</visus>
```

The "proxy" variable above contains both the scheme (SOCKS5) and ip (localhost).

A user would need to start the SOCKS5 proxy connection using a client. This can be done using ssh on Linux/MacOS with the following command:

```
ssh -D 55051 user@server
```

On Windows, you can enable a SOCKS5 proxy by using PuTTY. More information on that can be found [here](https://www.simplified.guide/putty/create-socks-proxy).

Keep in mind that the port you open the connection with must match the one specified in the visus.config file.
