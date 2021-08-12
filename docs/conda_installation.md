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

The activate conda:

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


```
python -m OpenVisus configure 
python -c "from OpenVisus import *"
```

Then run the viewer:

```
python -m OpenVisus viewer
```

Optionally remove the enviroment:

```
conda deactivate
conda remove --name myenv --all
```
