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

## VISUS_IDX2

Links:
- https://github.com/sci-visus/idx2.git


if you are debugging in Windows (change as needed):

```bash
SET PATH=%PATH%;C:\Python310;C:\Python310\Lib\site-packages\PyQt5\Qt\bin;c:\projects\OpenVisus\build\RelWithDebInfo\OpenVisus\bin
SET VISUS_VERBOSE_DISKACCESS=0
SET VISUS_CPP_VERBOSE=0
```

Download a test file from here:

```bash
curl -L https://github.com/sci-visus/OpenVisus/releases/download/files/MIRANDA-DENSITY-.384-384-256.-Float64.raw -O
python.exe Samples/python/extract_slices.py MIRANDA-DENSITY-.384-384-256.-Float64.raw  384 384 256 float64 ./tmp/input
```

Test pure IDX2, with IDX2 legacy file format (not cachable, not cloud-ready)

```bash
set VISUS_IDX2_USE_LEGACY_FILE_FORMAT=1

# this will create 
#    `tmp/legagy/Miranda/Density.idx` 
#    `tmp/legacy/Miranda/Density/L00.bin`

rmdir /S /Q tmp\legacy
visus.exe idx2 --encode "MIRANDA-DENSITY-.384-384-256.-Float64.raw" --name Miranda --field Density --dims 384 384 256 --type float64 --tolerance 1e-16 --num_levels 2 --out_dir tmp/legacy

# this will extract some data from the IDX2 file and `L*.bin` files
#   NOTE the dimension are 1+value/2
cd tmp\legacy
visus.exe idx2 --decode Miranda/Density.idx2 --downsampling 1 1 1 --tolerance 0.001 --out_file "output.legacy.raw"
python.exe ../../Samples/python/extract_slices.py output.legacy.raw 193 193 129 float64 ./decoded

cd ..

unset VISUS_IDX2_USE_LEGACY_FILE_FORMAT
```

Then test using one-chunk per file (useful for CloudAccess, DiskAccess etc):
- you will see a lot of files, one per block/chunk
- probably blocks are too small

```bash

# this will create 
#    `tmp/arco/Miranda/Density.idx` and 
#    `tmp/arco/Miranda/Density/0==<timestep>/Density==<fieldname>/0000/0000/0000/0000.bin`

rmdir /S /Q tmp\arco
visus.exe idx2 --encode MIRANDA-DENSITY-.384-384-256.-Float64.raw --name Miranda --field Density --dims  384 384 256 --type float64 --tolerance 1e-16 --num_levels 2 --out_dir tmp/arco

# NOTE: don't need to compress since the compression/decompression is really the IDX encoding/decoding
# in fact you will see blocks of different sizes

# export the output (NOTE the dimension are 1+value/2)
#   NOTE the dimension are 1+value/2
cd tmp\arco
visus.exe idx2  --decode "Miranda\Density.idx2" --downsampling 1 1 1 --tolerance 0.001 --out_file output.arco.raw
python.exe ../../Samples/python/extract_slices.py output.arco.raw 193 193 129 float64 ./decoded
cd ..\..

# (OPTIONAL) if you want the data on the cloud
#   e.g.  s3://utah/idx2/Miranda/Density.idx2
#   e.g.  s3://utah/idx2/Miranda/Density/0/Density/0000/0000/0000/0000.bin
aws s3 --profile sealstorage --no-verify-ssl sync ./tmp/arco/Miranda s3://utah/idx2/Miranda/
```

You can create a `visus.config` with:
- NOTE: compression will be automatically set to `raw` since the compression/decompression is really IDX2 encoding/decoding
-       current implementation does read blocks (i.e. IDX2 chunks) serially so first network access will be SLOW. Caching will help later.
-       atomic IO operation are "chunk" reading. Need to investigate if a chunk is made of mantissa/exponent or is selectively possible to read piece of info (IMPORTANT!)

```xml

<dataset name="idx2-legacy"   url="tmp/legacy/Miranda/Density.idx2?legacy=1" />
<dataset name="idx2-arco"     url='tmp/arco/Miranda/Density.idx2' />
<dataset name="idx2-cloud"    url='https://maritime.sealstorage.io/api/v0/s3/utah/idx2/Miranda/Density.idx2?profile=sealstorage&amp;cached=arco' />
```