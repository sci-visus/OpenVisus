
import os
import subprocess
import shutil

from OpenVisus import *

os.environ["VISUS_DISABLE_WRITE_LOCK"]="1"

# be careful to the perfect alignment (division must be exact)
piece=(1024,1024,1024)

src=LoadDataset(r"D:\GoogleSci\visus_dataset\2kbit1\lz4\rowmajor\visus.idx")
Ssize=src.getLogicBox()[1]
print("Ssize",Ssize)

Durl=r"D:\GoogleSci\visus_dataset\2kbit1\huge\raw\visus.idx"
nx,ny,nz=4,4,2
Dsize=(nx*Ssize[0],ny*Ssize[1],nz*Ssize[2])
print("Dsize",Dsize)

# create the idx
field=src.getField()
field.default_compression="raw"

shutil.rmtree(os.path.dirname(Durl), ignore_errors=True)
dst=CreateIdx(url=Durl, dims=Dsize,fields=[field])
print(dst.getDatasetBody().toString())
print("memsize",StringUtils.getStringFromByteSize(field.dtype.getByteSize(PointNi(Dsize))))


for Z in range(0,Dsize[2],piece[2]):
	for Y in range(0,Dsize[1],piece[1]):
		for X in range(0,Dsize[0],piece[0]):
			print(X,Y,Z)
			x1=X % Ssize[0];x2=x1+piece[0]
			y1=Y % Ssize[1];y2=y1+piece[1] 
			z1=Z % Ssize[2];z2=z1+piece[2] 
			data=src.read(x=[x1,x2],y=[y1,y2],z=[z1,z2])
			dst.write(data,x=X,y=Y,z=Z)