# OpenViSUS Visualization project  
     
![GitHub Actions](https://github.com/sci-visus/OpenVisus/workflows/BuildOpenVisus/badge.svg)
[![Binder](https://mybinder.org/badge_logo.svg)](https://mybinder.org/v2/gh/sci-visus/OpenVisus/master?filepath=Samples%2Fjupyter)
 
 
# Mission

The mission of ViSUS.org is to provide support for the scientific community with Big Data, management, analysis and visualization tools.
In this website we provide access to open source software tools and libraries such as the ViSUS framework and the PIDX library.
These softwares are distributed under the permissive BSD license.

# Pip install

To install OpenVisus using `pip` (add `--user` to `pip install` if you get permission denied errors):

```
# 
python -m pip install --upgrade pip

# uncomment the following lines if you want to use virtual env
# python -m pip install virtualenv
# python -m venv ~/my-virtual-environment
# source ~/my-virtual-environment/bin/activate

python -m pip install --upgrade OpenVisus
python -m OpenVisus configure 
```

And run the OpenVisus viewer:

```
python -m OpenVisus viewer
```

# Conda install

See 



Give a look to directory `Samples/python` and Jupyter examples:

[Samples/jupyter/quick_tour.ipynb](https://github.com/sci-visus/OpenVisus/blob/master/Samples/jupyter/quick_tour.ipynb)

[Samples/jupyter/Agricolture.ipynb](https://github.com/sci-visus/OpenVisus/blob/master/Samples/jupyter/Agricolture.ipynb)

[Samples/jupyter/Climate.ipynb](https://github.com/sci-visus/OpenVisus/blob/master/Samples/jupyter/Climate.ipynb)

[Samples/jupyter/ReadAndView.ipynb](https://github.com/sci-visus/OpenVisus/blob/master/Samples/jupyter/ReadAndView.ipynb)





<!--//////////////////////////////////////////////////////////////////////// -->
## Commit CI

For OpenVisus developers only:

```
TAG=$(python3 Libs/swig/setup.py new-tag) && echo ${TAG}
# replace manually the version in enviroment.yml if needed for binder
git commit -a -m "New tag" && git tag -a $TAG -m "$TAG" && git push origin $TAG && git push origin
```


