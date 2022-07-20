---
layout: default
title: Home
nav_order: 1
permalink: /
---

# OpenViSUS Documentation
{: .no_toc }
The mission of ViSUS.org is to provide support for the scientific community with Big Data, management, analysis and visualization tools. In this website we provide access to open source software tools and libraries such as the ViSUS framework and the PIDX library. These softwares are distributed under the permissive BSD license (see LICENSE file).

## Getting Started

## Table of contents
{: .no_toc .text-delta }

1. TOC
{:toc}

---

### Installation
OpenViSUS is available through [pip](https://pypi.org/project/OpenVisus/) and [conda](https://anaconda.org/ViSUS/openvisus).

Through `pip`:

```
$ python -m pip install OpenVisus
```

Through `conda`:
```
$ conda install -c visus openvisus
```

Once installed through your Python package manager of choice, run the configuration:
```
$ python -m OpenVisus configure
```

### Using OpenViSUS with Python/Jupyter
See the different functions available for use in Python and Jupyter [here]({{ site.baseurl }}{% link docs/python-features/python-features.md %}).

### Using the OpenViSUS Viewer
Launch the OpenViSUS viewer using the following command:
```
$ python -m OpenVisus viewer
```

See the different features of the viewer [here]({{ site.baseurl }}{% link docs/viewer-features/viewer-features.md %}).

### Contributing to Documentation


## Older Documentation
Older documentation can be found on [this page]({{ site.baseurl }}{% link docs/old-docs/old-docs.md %}).
