# OpenViSUS Visualization project  
     
![GitHub Actions](https://github.com/sci-visus/OpenVisus/workflows/BuildOpenVisus/badge.svg)
[![Binder](https://mybinder.org/badge_logo.svg)](https://mybinder.org/v2/gh/sci-visus/OpenVisus/master?filepath=Samples%2Fjupyter)

 
 
# Mission

The mission of ViSUS.org is to provide support for the scientific community with Big Data, management, analysis and visualization tools.
In this website we provide access to open source software tools and libraries such as the ViSUS framework and the PIDX library.
These softwares are distributed under the permissive BSD license (see [LICENSE](https://github.com/sci-visus/OpenVisus/tree/master/LICENSE) file).

# Installation

For `conda` see 
[docs/conda_installation.md](https://github.com/sci-visus/OpenVisus/blob/master/docs/conda_installation.md).


Install OpenVisus:

```
python -m pip install --upgrade pip
python -m pip install --upgrade OpenVisus
python -m OpenVisus configure 
python -c "from OpenVisus import *"
```

Notes:
- if you get *permission denied* error, use `python -m pip install  --user`.
- if you need a *minimal installation* without the GUI replace `OpenVisus` with `OpenVisusNoGui`
- If you want to create an isolated *virtual environment*:
	```
	python -m pip install --upgrade virtualenv
	python -m venv ~/my-virtual-environment
	source ~/my-virtual-environment/bin/activate
	```

Run the OpenVisus viewer:

```
python -m OpenVisus viewer
```

# Quick tour and Tutorials

Start with 
[quick_tour.ipynb](https://github.com/sci-visus/OpenVisus/blob/master/Samples/jupyter/quick_tour.ipynb) 
Jupyter Notebook.

See 
[Samples/jupyter](https://github.com/sci-visus/OpenVisus/tree/master/Samples/jupyter)
directory. 

To run the tutorials on the cloud click this [binder link](https://mybinder.org/v2/gh/sci-visus/OpenVisus/master?filepath=Samples%2Fjupyter).


# Run OpenVisus server

Run single `Docker` OpenVisus server:  
[docs/docker_modvisus](https://github.com/sci-visus/OpenVisus/blob/master/docs/docker_modvisus.md).

Runload-balanced `Kubernetes` OpenVisus servers: 
[docs/kubernetes.md](https://github.com/sci-visus/OpenVisus/blob/master/docs/kubernetes.md).


Run load-balanced `Docker Swarm` OpenVisus servers: 
[docs/docker_swarm_modvisus.md](https://github.com/sci-visus/OpenVisus/blob/master/docs/docker_swarm_modvisus.md).



# Compilation

See [docs/compilation.md](https://github.com/sci-visus/OpenVisus/blob/master/docs/compilation.md).


# IDX2

Make sure you have the IDX submodule (check if IDX2/directory is in Libs/).

In `cmake configure` step enable VISUS_IDX2 checkbox and `Build all`.

You can download a test file from here https://github.com/sci-visus/OpenVisus/releases/download/files/MIRANDA-DENSITY-.384-384-256.-Float64.raw.

Create an idx2 file:

```
# under Windows
# set PATH=%PATH%;build\RelWithDebInfo\OpenVisus\bin

idx2 --encode --input MIRANDA-DENSITY-[384-384-256]-Float64.raw --accuracy 1e-16 --num_levels 2 --brick_size 64 64 64 --bricks_per_tile 512 --tiles_per_file 512 --files_per_dir 512 --out_dir tmp
```

To retrieve the data

```
idx2 --decode --input tmp/MIRANDA/DENSITY.idx2 --in_dir . --first 0 0 0 --last 383 383 255 --level 1 --mask 128 --accuracy 0.001 --out_dir tmp --out_file decode.raw

# CHECK the file 
```

To test in OpenVisus viewer:

```
visusviewer tmp/MIRANDA/DENSITY.idx2
```

