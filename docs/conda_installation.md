# Install OpenVisus (conda)

You need a `conda` installation. Follow the instructions and make sure conda is [installed and in PATH](https://conda.io/projects/conda/en/latest/user-guide/install/index.html). You can also install the Anaconda GUI from [here](https://conda.io/projects/conda/en/latest/user-guide/install/index.html).

(OPTIONAL) To avoid conflicts with CPython installations, it is better to disable auto activating the base environment in conda.
```
conda config --set auto_activate_base false
```

Now create an environment, and install required packages:
```bash
conda create -n ovenv python=3.7 
conda install -n ovenv -y -c visus openvisus
```

For OSX and Linux you may need to add this variable to avoid conflicts with `pip` CPython installed packages in `~/.local` (see [this link](https://github.com/conda/conda/issues/7173) for more info):
```bash
conda env config vars set PYTHONNOUSERSITE=1 -n ovenv
```

Finally we configure and activate the environment and test it (just ignore segmentation fault error on some Linux distributions in the `configure` step):
```bash
# Activate environment
conda activate ovenv
# one time configuration
python -m OpenVisus configure 
# Test the installation
python -c "from OpenVisus import *"
```

If you would like to use conda for the rest of the configuration, create an environment variable: `USE_CONDA=1`
Otherwise, by default, the configuration process will use pip.

(OPTIONAL) If you get *numpy import error* such as  `Library not loaded: @rpath/libopenblas.dylib`, usually this is because a faulty installation of numpy. So reinstalling it should fix this problem:
```bash
conda uninstall -n ovenv -y numpy
conda install   -n ovenv -y nomkl numpy scipy scikit-learn numexpr openblas blas
conda remove    -n ovenv -y mkl mkl-service
# test numpy
python -c "import numpy"
```

You can also run the viewer (when conda environment is activated):
```bash
python -m OpenVisus viewer
```

# Remove OpenVisus (conda)
Just remove the whole environment.
```bash
conda deactivate
conda remove -n ovenv --all
```