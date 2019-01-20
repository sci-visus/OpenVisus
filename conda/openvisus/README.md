# Conda instructions

To compile OpenVisus in conda. FIrst of all update your conda:

```
conda install conda-build
conda update conda
conda update conda-build

# make sure you are using a conda python
which python
```

On Windows install visual studio 2015 with C++.

Then compile OpenVisus:

```
cd conda
conda-build openvisus
```

Take note of the openvisus package name in case you want to upload it.

Finally install and test it:

```
conda install --use-local openvisus

# on Windows evaluate the command and cd manually
cd $(python -m OpenVisus dirname)

python Samples/python/Array.py
python Samples/python/Dataflow.py
python Samples/python/Idx.py
```


In case you want to upload:

```
anaconda login
anaconda upload <conda/openvisus/filename/here>
```



