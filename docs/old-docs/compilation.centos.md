---
layout: default
parent: Old Docs
nav_order: 2
---

# Buttons
{: .no_toc }

## Table of contents
{: .no_toc .text-delta }

1. TOC
{:toc}

---

# Instructions for RedHat based distributions (Fedora, Centos, Redhat)

```
yum install -y gcc-c++
yum install -y patchelf swig3 cmake3
yum install -y python3 python3-devel python3-pip

git clone https://github.com/sci-visus/OpenVisus
cd OpenVisus
mkdir build 
cd build
cmake3 -DPython_EXECUTABLE=$(which python3) -DVISUS_GUI=0 _DVISUS_MODVISUS=0 -DVISUS_SLAM=0 ../
make-j # remove "-j" in case of errors
make install

export PYTHONPATH=$(pwd)/Release 
python3 -c "from OpenVisus import *"
```

