import sys
from OpenVisus import *
import numpy as np
from os.path import abspath,splitext

def load_image(img_file):
	"""
	Load the image given by filename and return the approriate 
	numpy array 
	"""
	
	if splitext(img_file)[1] == '.tif' or splitext(img_file)[1] == '.tiff':
		from PIL import Image
		
		data = np.array(Image.open(img_file))
	else:
		raise TypeError('Image format not found')

	return data


if __name__ == '__main__':
	
	""" 
	Somewhat general data conversion tool from slices to 3D stacks
	
	Input is a list of slices 
	
	Ouput is an IDX
	
	"""

	if len(sys.argv) < 3:
		print("Usage: {} <slice0> .... <sliceN> <target.idx>".format(sys.argv[0]))
		sys.exit(1)

	SetCommandLine("__main__")
	DbModule.attach()

	# trick to speed up the conversion
	os.environ["VISUS_DISABLE_WRITE_LOCK"]="1"

	# figure out the format, dimensions etc.
	img_file = sys.argv[1]
	
	data = load_image(img_file)
	
	
	width = data.shape[0]
	height = data.shape[1]
	depth = len(sys.argv[1:-1])


	print("Final image will have width: {}  height: {} depth: {}".format(width,height,depth))
	
	idx_name = abspath(sys.argv[-1])
	
	# numpy dtype -> OpenVisus dtype
	typestr=data.__array_interface__["typestr"]
	dtype=DType(typestr[1]=="u", typestr[1]=="f", int(typestr[2])*8)      	
	
	dims = PointNi(int(width),int(height),int(depth))
	idx_file = IdxFile()
	idx_file.logic_box = BoxNi(PointNi(0,0,0),dims)
	idx_file.fields.push_back(Field('density',dtype))
	idx_file.save(idx_name)
			
	print("Created IDX file")

	# Load the dataset back into memory	
	dataset = LoadDataset(idx_name)
	
	# And create a handle to manipulate the data
	access = dataset.createAccess()
	if not dataset:
		raise Exception("Assert failed")	
  
	for z,img in enumerate(sys.argv[1:-1]):  			
		print("Processing slice %d" % z)
		data = load_image(img)
		
		# Create the correct logical box where to put this data
		slice_box = dataset.getLogicBox().getZSlab(z,z+1)
		if not (slice_box.size()[0]==dims[0] and slice_box.size()[1]==dims[1]):
			raise Exception("Image dimensions do not match")	
			
		# Create a query into the dataset. Note the 'w' to indicate a write query
		# and the fact that for now we assume a single time step
		query = BoxQuery(dataset,dataset.getDefaultField(),dataset.getDefaultTime(),ord('w'))
		query.logic_box = slice_box
		
		# Start the query and make sure that it works
		dataset.beginQuery(query)
		if not query.isRunning():
			raise Exception("Could not start write query")	
		
		# Some type casting gymnastics
		buffer = Array(query.getNumberOfSamples(),query.field.dtype)	
		handle = Array.toNumPy(buffer,bSqueeze=True,bShareMem=True)
		handle = data
		
		# Assign the data
		query.buffer = buffer
		
		# Actually commit the data 
		if not (dataset.executeQuery(access,query))	:
			raise Exception("Could not execute write query")
		
		# enable/disable these two lines after debugging
		# ArrayUtils.saveImageUINT8("tmp/slice%d.orig.png" % (Z,),Array.fromNumPy(data))
		# ArrayUtils.saveImageUINT8("tmp/slice%d.offset.png" % (Z,),Array.fromNumPy(fill))
	
	DbModule.detach()
	print("Done with conversion")
	sys.exit(0)