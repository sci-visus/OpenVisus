import sys
import numpy as np
from OpenVisus import *


if __name__ == '__main__':
	
	"""
	Example of data conversion
	
	Input is composed of 3 images, one for each channel (RGB)
	each channel it's a 3d array
	
	Ouput is an IDX
	
	For each source slice the original data is shifted by 5 pixels:
		first  slice is shifted by ( 0,0)
		second slice is shifted by ( 5,0)
		third  slice is shifted by (10,0)	
	
	"""

	SetCommandLine("__main__")
	DbModule.attach()

	# trick to speed up the conversion
	os.environ["VISUS_DISABLE_WRITE_LOCK"]="1"
	
	# 3 channels (RGB) each one is a 3d array
	width,height,depth=2,3,4

	channels = [
	    np.zeros((depth,height,width),dtype=np.uint8)	,
	    np.zeros((depth,height,width),dtype=np.uint8)	,
	    np.zeros((depth,height,width),dtype=np.uint8)	,
	]
	              
	idx_filename='visus.idx'

	# for each slize along Z axis I shift the original data of offset[0],offset[1],offset[2] (like a shear)
	offset = (5,0,0)

	dataset,access,fields=None,None,None

	for ChannelId,channel in enumerate(channels):

	    # scrgiorgio: just for testing, if I get a 2d image instead of a 3d image stack I simulate a stack
	    if len(channel.shape)==2: 
	        channel=np.stack([channel]*7)
	    
	    dims=list(reversed(channel.shape))
	    
	    # numpy dtype -> OpenVisus dtype
	    typestr=channel.__array_interface__["typestr"]
	    dtype=DType(typestr[1]=="u", typestr[1]=="f", int(typestr[2])*8)        
	    
	    idx_logic_box=BoxNi(PointNi(0,0,0),PointNi(
	            dims[0]+ offset[0]*dims[2],
	            dims[1]+ offset[1]*dims[2],
	            dims[2]+ offset[2]*dims[2]))
	            
	    # create the idx 
	    if ChannelId==0:
	        print("Creating idx file")
	        idx_file = IdxFile()
	        idx_file.logic_box=idx_logic_box
	        print("\tbox",idx_logic_box.toString())
	        
	        # assuming all the files are the same type
	        for I in range(len(channels)): 
	            field=Field('channel_%d' % (I,),dtype)
	            print("\tfield",field.name,field.dtype.toString())
	            idx_file.fields.push_back(field)
	        
	        idx_file.save(idx_filename)  
	        	
	        dataset = LoadDataset(idx_filename)
	        	
	        access = dataset.createAccess()
	        if not access: raise Exception("Cannot create access")
	        	
	        fields=idx_file.fields
	        
	    else:
	        # check dimensions and dtype are compatible
	        if not (dataset.getLogicBox()==idx_logic_box):
	        	raise Exception("Logic box is wrong")
	        	
	        if not (dtype==fields[ChannelId].dtype):
	        	raise Exception("dtype is wrong")
	        
	    print("Adding","channel",ChannelId,"dims",dims,"dtype",dtype.toString())  
	        
	    # Now we go through each slice and add them one by one since we have to take care of the offset
	    for Z in range(0,dims[2]):
	    
	        # target area in idx (i.e. apply a shear)
	        x1,x2 = 0+offset[0]*Z , dims[0]+offset[0]*Z
	        y1,y2 = 0+offset[1]*Z , dims[1]+offset[1]*Z
	        z1,z2 = Z+offset[2]*Z , Z+1    +offset[2]*Z
	      
	        print("\tProcessing Z",Z,"target_p1",(x1, y1, z1),"target_p2",(x2, y2, z2))
	               
	        query = BoxQuery(dataset,dataset.getDefaultField(),dataset.getDefaultTime(),ord('w')) 
	        query.logic_box = BoxNi(PointNi(x1,y1,z1),PointNi(x2,y2,z2))
	        query.field = fields[ChannelId] 
	        dataset.beginQuery(query)
	        
	        if not query.isRunning():
	        	raise Exception("Begin query failed")
	        	
	        query.buffer=Array.fromNumPy(channel[Z,:,:],bShareMem=True)
	        	
	        if not dataset.executeQuery(access,query):
	        	raise Exception("Cannot execute query")
	        	

	DbModule.detach()
	print("Done with conversion")
	sys.exit(0)