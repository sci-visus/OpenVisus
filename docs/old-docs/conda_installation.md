---
layout: default
title: Conda Installation
parent: Older Documentation
nav_order: 2
---

# Table of contents
{: .no_toc .text-delta }

1. TOC
{:toc}

---

# Install OpenVisus

You need a `conda` installation. For Windows, go to [this link](https://www.anaconda.com/products/individual) and follow instructions. At the end you will have an `Anacronda Prompt`. On OSX you can use `brew install` or:

```
curl -O https://repo.anaconda.com/archive/Anaconda3-2020.07-MacOSX-x86_64.sh
bash Anaconda3-2020.07-MacOSX-x86_64.sh 
```

On Linux:

```
curl -O https://repo.anaconda.com/archive/Anaconda3-2019.03-Linux-x86_64.sh
bash Anaconda3-2019.03-Linux-x86_64.sh
```

(OPTIONAL) To avoid conflicts with CPython installations:

```
conda config --set auto_activate_base false
```

Then activate conda:

```
conda activate
```


For OSX and Linux you may need to add this variable to avoid conflicts with `pip` CPython installed packages in `~/.local` (see [this link](https://github.com/conda/conda/issues/7173) for more info):

```
export PYTHONNOUSERSITE=True 
```


Open a Windows `Anaconda prompt` or a shell and type (change python version and env name as needed): 

```
conda create -n myenv python=3.7 
conda activate myenv
conda install --name myenv  -y conda
conda install --name myenv  -y --channel visus openvisus
```

Test it (just ignore segmentation fault error on some Linux distributions in the `configure` step):
If you would like to use conda for the rest of the configuration, create an environment variable: `USE_CONDA=1`
Otherwise, by default, the configuration process will use pip.

```
python -m OpenVisus configure 
python -c "from OpenVisus import *"
```

(OPTIONAL) If you get *numpy import error* such as  `Library not loaded: @rpath/libopenblas.dylib`,  this:

```
conda uninstall --name myenv  -y numpy
conda install   --name myenv  -y nomkl numpy scipy scikit-learn numexpr openblas blas
conda remove    --name myenv  -y mkl mkl-service
python -c "import numpy"
```

Run the viewer:

```
python -m OpenVisus viewer
```





(OPTIONAL) Remove the enviroment:

```
conda deactivate
conda remove --name myenv --all
```
