# Instructions

If you want to use precompiled conda packages just do:

```
conda install -c visus openvisus
```


If you want to use Docker to build the openvisus conda recipe:

```
cd conda/openvisus
sudo docker build .
```

If you want to build the openvisus recipe in your current shell , please keep reading.
Prerequisites for windows: remeber to install "Visual Studio 2015" with C++.
Check that both this components are installed: "Common tools for visual c++ 2015" AND "Tools and Windows XX SDK"

Prerequisites for all platforms:

```
conda install conda-build
conda update conda
conda update conda-build
```

Then:

```
cd conda
conda-build openvisus

conda install --use-local openvisus

cd $(python -m OpenVisus dirname)
python Samples/python/Array.py
python Samples/python/Dataflow.py
python Samples/python/Idx.py
```


In case you want to upload to conda rep:

```
anaconda login
anaconda upload /type/your/conda/openvisus/filename/here
```



