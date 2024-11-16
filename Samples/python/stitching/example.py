
import os
import rasterio
import argparse
import subprocess
import pathlib
import shutil
from glob import glob
from PIL import Image,ImageOps
import shutil

from OpenVisus           import *
from OpenVisus.__main__ import MidxToIdx

DbModule.attach()

# little optimization
# since all writing are done serially I can disable file locks
os.environ["VISUS_DISABLE_WRITE_LOCK"]="1"

# prerequisites for windows: download and install GDAL and rasterio from here https://www.lfd.uci.edu/~gohlke/pythonlibs
dst_directory="./tmp"

# find images
images=glob.glob("./NIWO/NIWO/*.tif",recursive=False)

print("Found images",images)

# if you want to limit the images
# images=images[0:2]

# convert to idx
sx,sy=1.0, 1.0
tiles=[]
for I,filename in enumerate(images):
	metadata =rasterio.open(filename)
	name=os.path.splitext(os.path.basename(filename))[0]
	width,height=metadata.width,metadata.height
	x1,y1,x2,y2=metadata.bounds.left,metadata.bounds.bottom, metadata.bounds.right, metadata.bounds.top
	
	# compute scaling to keep all pixels
	if I==0:
		sx=width /(x2-x1)
		sy=height/(y2-y1)
		
	x1,y1,x2,y2=sx*x1,sy*y1,sx*x2,sy*y2
	tile={"name": name, "size" : (width,height), "bounds" : (x1,y1,x2,y2)}
	print(I,"/",len(images), name,"width", width, "height", height,"x1", "y1", x1, y1, "x2-x1", x2-x1, "y2-y1", y2-y1)
	tiles.append(tile) 
	
	# avoid creation multiple times (useful for debugging)
	if not os.path.isfile(os.path.join(dst_directory,name,"visus.idx")):
		data=Image.open(filename)
		data=ImageOps.flip(data)
		
		idx_filename=os.path.join(dst_directory,name,"visus.idx")
		shutil.rmtree(os.path.dirname(idx_filename), ignore_errors=True)
		CreateIdx(url=idx_filename, dim=2,data=numpy.asarray(data))

# create midx
X1=min([tile["bounds"][0] for tile in tiles])
Y1=min([tile["bounds"][1] for tile in tiles])
midx_filename=os.path.join(dst_directory,"visus.midx")
with open(midx_filename,"wt") as file:
	file.writelines([
		"<dataset typename='IdxMultipleDataset'>\n",
		"\t<field name='voronoi'><code>output=voronoi()</code></field>\n",
		*["\t<dataset url='./{}/visus.idx' name='{}' offset='{} {}'/>\n".format(tile["name"],tile["name"],tile["bounds"][0]-X1,tile["bounds"][1]-Y1) for tile in tiles],
		"</dataset>\n"
	])

# to see automatically computed idx file
db=LoadDataset(midx_filename)
print(db.getDatasetBody().toString())

# midx-to-idx
idx_filename=os.path.join(dst_directory, "visus.idx")
MidxToIdx(["--midx", midx_filename, "--field","output=voronoi()", "--tile-size","4*1024", "--idx", idx_filename])
print("ALL DONE")