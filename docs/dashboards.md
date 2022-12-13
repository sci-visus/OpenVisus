# OpenVisus dashboards

OpenVisus can run as python dashboard.

## modvisus **2d** viewer

Create a `run.py` file:

```python
import os,sys
import OpenVisus  as  ov
from  OpenVisus.dashboards  import  Slice

# change as needed
db=ov.LoadDataset("http://atlantis.sci.utah.edu/mod_visus?dataset=david_subsampled&cached=1")
slice=Slice()
slice.setDataset(db, direction=2)
slice.setPalette("Greys256", palette_range=(0,255))

import bokeh.io
bokeh.io.curdoc().theme = 'caliber'
bokeh.io.curdoc().add_root(slice.layout)
```

and run it:

```shell
python3 -m bokeh serve ./test.py 
```

![Animation](https://raw.githubusercontent.com/sci-visus/OpenVisus/docs/dashboards.00.gif)

## mod_visus **3d** slicer

Create a `run.py` file:

```python

import  os,sys
import  OpenVisus  as  ov
from  OpenVisus.dashboards  import  Slice

# change as needed
db=ov.LoadDataset("http://atlantis.sci.utah.edu/mod_visus?dataset=2kbit1&cached=1")
slice=Slice()
slice.setDataset(db, direction=2)
slice.setPalette("Greys256", palette_range=(0,255))

import bokeh.io
bokeh.io.curdoc().theme = 'caliber'
bokeh.io.curdoc().add_root(slice.layout)
```

and run it:

```shell
python3 -m bokeh serve ./test.py 
```


![Animation](https://raw.githubusercontent.com/sci-visus/OpenVisus/docs/dashboards.01.gif)

## Cloud Storage **3d** slicer

Create a `run.py` file:

```python
import  os,sys
import  OpenVisus  as  ov
from   OpenVisus.dashboards  import  Slice

# change as needed
db=ov.LoadDataset("https://s3.us-west-1.wasabisys.com/nsdf/visus-datasets/2kbit1/1mb/visus.idx?profile=wasabi&cached=idx")
slice=Slice()
slice.setDataset(db, direction=2)
slice.setPalette("Greys256", palette_range=(0,255))

import  bokeh.io
bokeh.io.curdoc().theme = 'caliber'
bokeh.io.curdoc().add_root(slice.layout)
```

and run it:

```shell
python3 -m bokeh serve ./test.py 
```


![Animation](https://raw.githubusercontent.com/sci-visus/OpenVisus/docs/dashboards.02.gif)


## **3d** multi Slicer


Create a `run.py` file:

- NOTE: to run this example you will need the WASABI credentials

```python
import  os,sys
import  OpenVisus  as  ov
from  OpenVisus.dashboards  import  Slices

# change as needed
db=ov.LoadDataset(f"https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112509.h5/r/idx/1mb/visus.idx?profile=wasabi&cached=idx&blob_extension=.bin.zz")
slices=Slices()
slices.setDataset(db,layout=4)
slices.setPalette("Greys256", palette_range=(0.0, 65535.0))

import  bokeh.io
bokeh.io.curdoc().theme = 'caliber'
bokeh.io.curdoc().add_root(slices.layout)
```

and run it:

```shell
python3 -m bokeh serve ./test.py 
```


![Animation](https://raw.githubusercontent.com/sci-visus/OpenVisus/docs/dashboards.03.gif)