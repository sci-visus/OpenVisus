---
layout: default
title: Climate
parent: Jupyter Notebook Examples
nav_order: 2
---

# Table of contents
{: .no_toc .text-delta }

1. TOC
{:toc}

---

**OpenViSUS: read from a remote dataset**


```python
import os,sys

# Here are commands to install a package (OpenVisus) directly from a jupyter notebook
# after you install those once you can comment those comment
# !{sys.executable} -m pip install numpy matplotlib OpenVisus
```


```python
import math
import matplotlib.pyplot as plt
import numpy
import ipywidgets 

def ShowData(data):
    fig = plt.figure(figsize = (70,20))
    ax = fig.add_subplot(1,1,1)
    ax.imshow(data, origin='lower')
    plt.show()
```


```python
from OpenVisus import *
```

    Starting OpenVisus c:\python38\lib\site-packages\OpenVisus\__init__.py 3.8.6 (tags/v3.8.6:db45529, Sep 23 2020, 15:52:53) [MSC v.1927 64 bit (AMD64)] sys.version_info(major=3, minor=8, micro=6, releaselevel='final', serial=0) ...



```python
db=LoadDataset("http://atlantis.sci.utah.edu/mod_visus?dataset=BlueMarble")
print(db.getDatasetBody().toString())

# in 2D this means more or less a 2048x1024 image in power-2 logic box 
max_resolution=21
```

    <dataset url="http://atlantis.sci.utah.edu/mod_visus?dataset=BlueMarble" typename="IdxDataset">
    	<idxfile>
    		<version value="6" />
    		<bitmask value="V001010101010101010101010101010101" />
    		<box value="0 86400 0 43200" />
    		<bitsperblock value="16" />
    		<blocksperfile value="256" />
    		<block_interleaving value="0" />
    		<filename_template value="./bluemarble-compressed/%02x/%04x.bin" />
    		<missing_blocks value="False" />
    		<time_template value="time%02d/" />
    		<physic_box value="0 5760 0 2881" />
    		<field name="data" description="" index="" default_compression="zip" default_layout="row_major" default_value="0" filter="" dtype="uint8[3]" />
    		<timestep from="0" to="11" step="1" />
    	</idxfile>
    </dataset>



```python
logic_box=db.getLogicBox()
data=db.read(time=0,max_resolution=max_resolution)
ShowData(data)
```


    
![png](Climate_files/Climate_5_0.png)
    



```python
def RefreshAll(time,num_refinements=3):
    for data in db.read(logic_box=logic_box, time=time,max_resolution=max_resolution,num_refinements=num_refinements):
        ShowData(data)

ipywidgets.interact(
    RefreshAll,
    time=ipywidgets.widgets.IntSlider(value=0,min=0,max=4,step=1), 
    resolution=ipywidgets.widgets.IntSlider(value=9,min=1,max=db.getMaxResolution(),step=2))
```


    interactive(children=(IntSlider(value=0, description='time', max=4), IntSlider(value=3, description='num_refin…





    <function __main__.RefreshAll(time, num_refinements=3)>




```python

```
