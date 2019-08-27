import sys
from skimage import io
from OpenVisus import *
import numpy as np

def ASSERT(cond):
	if not cond: raise Exception("Assert failed")	
		
if __name__ == '__main__':
	
	""" 
	Example of data conversion:
	
	Input is a TIF stack of images
	
	Ouput is an IDX
	For each source slice the original data is shifted by 5 pixels:
		first slice is shifted by (0,0)
		second slice is shifted by (5,0)
		third slice is shifted by (10,0)
		...
		
	"""

	SetCommandLine("__main__")
	IdxModule.attach()
	
	# trick to speed up the conversion
	os.environ["VISUS_DISABLE_WRITE_LOCK"]="1"
	
	img = io.imread('data.tif')
	print(img.shape)
	
	idx_name='./tmp/visus.idx'
	depth, height, width = img.shape[0], img.shape[1],img.shape[2]
	print("image",idx_name,"has dimensions",width,height,depth)
	
	# to disable offset just set this to 0
	offset_x=5
	
	dims=NdPoint.one(int(width + offset_x*depth),int(height),int(depth))
	idx_file = IdxFile()
	idx_file.box=NdBox(NdPoint(0,0,0),dims)
	idx_file.fields.push_back(Field('channel0',DType.fromString("uint16")))
	success = idx_file.save(idx_name)
	ASSERT(success)
	print("Created IDX file")
	
	dataset=LoadDataset(idx_name)
	access=dataset.createAccess()
	ASSERT(dataset)
  
	for Z in range(0,depth):
		
		print("Processing slice %d" % Z)
		data = img[Z,:,:]
		
		slice_box=dataset.getBox().getZSlab(Z,Z+1)
		ASSERT(slice_box.size()[0]==dims[0] and slice_box.size()[1]==dims[1])
			
		query=Query(dataset,ord('w'))
		query.position=Position(slice_box)
		ASSERT(dataset.beginQuery(query))

		buffer=Array(query.nsamples,query.field.dtype)
		buffer.fillWithValue(0)
		
		fill=Array.toNumPy(buffer,bSqueeze=True,bShareMem=True)
		y1=0
		y2=height
		x1=offset_x*Z
		x2=x1+width
		fill[y1:y2,x1:x2]=data
			
		query.buffer=buffer
		ASSERT(dataset.executeQuery(access,query))	
		
		# disable these two lines after debugging
		ArrayUtils.saveImageUINT8("tmp/slice%d.orig.png" % (Z,),Array.fromNumPy(data))
		ArrayUtils.saveImageUINT8("tmp/slice%d.offset.png" % (Z,),Array.fromNumPy(fill))
	
	IdxModule.detach()
	print("Done with conversion")