import sys
from OpenVisus import *
import numpy as np


if __name__ == '__main__':
	
	""" 
	Example of data conversion:
	
	Input is a 3d images (replace by whatever you have)
	
	Ouput is an IDX
	For each source slice the original data is shifted by 5 pixels:
		first  slice is shifted by ( 0,0)
		second slice is shifted by ( 5,0)
		third  slice is shifted by (10,0)
		...
	"""

	SetCommandLine("__main__")
	DbModule.attach()

	# trick to speed up the conversion
	os.environ["VISUS_DISABLE_WRITE_LOCK"]="1"
	
	# numpy display is Z,Y,X
	width,height,depth=2,3,4
	img = np.zeros((depth,height,width),dtype=np.uint16)
	if not (img.shape[0]==depth and img.shape[1]==height and img.shape[2]==width):
		raise Exception("Assert failed")	
	
	idx_name='./tmp/visus.idx'
	print("image",idx_name,"has dimensions",width,height,depth)
	
	# to disable offset just set this to 0
	offset_x=5
	
	# numpy dtype -> OpenVisus dtype
	typestr=img.__array_interface__["typestr"]
	dtype=DType(typestr[1]=="u", typestr[1]=="f", int(typestr[2])*8)      	
	
	dims=PointNi(int(width + offset_x*depth),int(height),int(depth))
	idx_file = IdxFile()
	idx_file.logic_box=BoxNi(PointNi(0,0,0),dims)
	idx_file.fields.push_back(Field('channel0',dtype))
	idx_file.save(idx_name)
			
	print("Created IDX file")
	
	dataset=LoadDataset(idx_name)
	access=dataset.createAccess()
	if not dataset:
		raise Exception("Assert failed")	
  
	for Z in range(0,depth):
		
		print("Processing slice %d" % Z)
		data = img[Z,:,:]
		
		slice_box=dataset.getLogicBox().getZSlab(Z,Z+1)
		if not (slice_box.size()[0]==dims[0] and slice_box.size()[1]==dims[1]):
			raise Exception("Assert failed")	
			
		query=BoxQuery(dataset,dataset.getDefaultField(),dataset.getDefaultTime(),ord('w'))
		query.logic_box=slice_box
		dataset.beginQuery(query)
		if not query.isRunning():
			raise Exception("Assert failed")	

		buffer=Array(query.getNumberOfSamples(),query.field.dtype)
		buffer.fillWithValue(0)
		
		fill=Array.toNumPy(buffer,bSqueeze=True,bShareMem=True)
		y1=0
		y2=height
		x1=offset_x*Z
		x2=x1+width
		fill[y1:y2,x1:x2]=data
			
		query.buffer=buffer
		if not (dataset.executeQuery(access,query))	:
			raise Exception("Assert failed")
		
		# enable/disable these two lines after debugging
		# ArrayUtils.saveImageUINT8("tmp/slice%d.orig.png" % (Z,),Array.fromNumPy(data))
		# ArrayUtils.saveImageUINT8("tmp/slice%d.offset.png" % (Z,),Array.fromNumPy(fill))
	
	DbModule.detach()
	print("Done with conversion")
	sys.exit(0)