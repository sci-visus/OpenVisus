# Instructions

## Use precompiled openvisus conda packages 

From a shell:

```
conda install -c visus openvisus
```


## Build conda openvisus recipe

### Build conda openvisus on Windows

[BROKEN right now]

### Build conda openvisus using Docker

Type:

```
cd conda/openvisus
sudo docker build .
```

### Build OpenVisus on Linux/OSX

In a shell:

```
PYTHON_VERSION=3.6  ./CMake/build_conda.sh
```

