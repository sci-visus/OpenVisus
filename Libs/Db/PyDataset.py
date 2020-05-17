import os,sys
import cv2,numpy

from OpenVisus import *
from OpenVisus.PyImage import *


# ////////////////////////////////////////////////////////////////////////////
def Create2DDatasetFromImage(idx_filename,filename):
	
	T1=Time.now()
	os.environ["VISUS_DISABLE_WRITE_LOCK"]="1"
	
	img = cv2.imread(filename, -1)
	img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
	# img=cv2.flip(img,0)	
	#cv2.imshow('RGB Image',img )
	#cv2.waitKey(0)

	height,width,nchannels=img.shape[0],img.shape[1],img.shape[2] if len(img.shape)>2 else 1
	img_bytesize=width*height*nchannels
	MB=1024*1024
	GB=MB*1024

	print("Image",filename,"width",width,"height",height,"nchannels",nchannels,"bytesize",int(img_bytesize/MB),"MB")

	field=Field()
	field.name="DATA"
	field.dtype=DType.fromString("uint8[%d]" % (nchannels,))
	field.default_layout="rowmajor"

	idxfile=IdxFile()
	idxfile.fields.push_back(field)
	idxfile.logic_box=BoxNi(PointNi(0,0),PointNi(width,height))
	if not idxfile.save(idx_filename):
		raise Exception("cannot save the idx filename")
		
	print("Idx file is the following:")
	print("/////////////////////////////////////////////")
	print(idxfile.toString())
	print("/////////////////////////////////////////////")

	dataset=LoadDataset(idx_filename)
	access=dataset.createAccess()
	field=dataset.getDefaultField()
	time=dataset.getDefaultTime()
	
	print("\tWriting idx")
	array=Array.fromNumPy(img,TargetDim=2,bShareMem=True)
	if not dataset.writeFullResolutionData(access,field,time,array,idxfile.logic_box):
		raise Exception("cannot write data")	

	print("Conversion done in",T1.elapsedMsec(),"msec")	
	

# ////////////////////////////////////////////////////////////////////////////
def Create3DDatasetFromImages(idx_filename,filenames):
	
	T1=Time.now()
	os.environ["VISUS_DISABLE_WRITE_LOCK"]="1"
	
	img = cv2.imread(filenames[0], -1)
	#img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
	#cv2.imshow('RGB Image',img )
	#cv2.waitKey(0)

	depth,height,width,nchannels=len(filenames),img.shape[0],img.shape[1],img.shape[2] if len(img.shape)>2 else 1
	img_bytesize=width*height*nchannels
	MB=1024*1024
	GB=MB*1024

	print("Found",depth,"filenames","width",width,"height",height,"nchannels",nchannels)
	print("Fist file",filenames[0])
	print("Last file",filenames[-1])
	print("Each image has raw bytesize of ",int(img_bytesize/MB),"MB")
	
	max_bytesize=1*GB
	
	def next_power_of_2(x):
		return 1 if x == 0 else 2**math.ceil(math.log2(x))	
	
	deltaz=next_power_of_2(int(max_bytesize/img_bytesize))
	
	print("Choosed a deltaz of ",deltaz, " Slab bytesize",int(img_bytesize*deltaz/MB),"MB")
	
	field=Field()
	field.name="DATA"
	field.dtype=DType.fromString("uint8[%d]" % (nchannels,))
	field.default_layout="rowmajor"

	idxfile=IdxFile()
	idxfile.fields.push_back(field)
	idxfile.logic_box=BoxNi(PointNi(0,0,0),PointNi(width,height,depth))
	if not idxfile.save(idx_filename):
		raise Exception("cannot save the idx filename")
		
	print("Idx file is the following:")
	print("/////////////////////////////////////////////")
	print(idxfile.toString())
	print("/////////////////////////////////////////////")

	dataset=LoadDataset(idx_filename)
	access=dataset.createAccess()
	field=dataset.getDefaultField()
	time=dataset.getDefaultTime()
	
	print("Starting conversion")
		
	for Z in range(0,depth,deltaz):
		
		if Z:
			sec=T1.elapsedSec()
			progress=Z/float(depth)
			eta_sec=int(sec/progress)
			print("Progress perc(%d) elapsed_seconds(%d) eta_seconds(%d) eta_minutes(%.1f) ",int(progress*100.0),sec,eta_sec,eta_sec/60.0)		
		
		Z1,Z2=Z,min((Z+deltaz,depth))
		print("Convert %d/%d filenames" % (Z,depth))
		
		slab_images=[]
		for I in range(Z2-Z1):
			filename=filenames[Z+I]
			print("\tReading",filename)
			img=cv2.imread(filename, -1)
			if img is None:
				raise Exception("failed to read file" % (filename,))
				
			img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
			slab_images.append(img)
				
		print("\tStacking images")
		slab=numpy.stack(slab_images,axis=0) 
		if slab.shape!=(Z2-Z1,height,width,nchannels):
			raise Exception("internal error")
			
		logic_box=BoxNi(PointNi(0,0,Z1),PointNi(width,height,Z2))

		print("\tWriting idx")
		array=Array.fromNumPy(slab,TargetDim=3,bShareMem=True)
		if not dataset.writeFullResolutionData(access,field,time,array,logic_box):
			raise Exception("cannot write data")	

	print("Conversion done in",T1.elapsedMsec(),"msec")



# //////////////////////////////////////////////
class PyDataset(object):
	
	# constructor
	def __init__(self,url):
		self.dataset = LoadDataset(url)
		
	# __getattr__
	def __getattr__(self,attr):
	    return getattr(self.dataset, attr)	
	    
	# createAccess
	def createAccess(self):
		return self.dataset.createAccess()

	# readData
	# NOTE: if resolution <=0 that it is used as a delta i.e. resolution=dataset.getMaxResolution()+resolution
	def readData(self,box, resolution=0,time=None, field=None):
		t1=Time.now()
		
		# in alpha coordinate
		if type(box) in [list, tuple]: 
			alpha=box
			logic_box=BoxNi(
					PointNi(int(alpha[0]*self.getLogicBox().size()[0]),int(alpha[2]*self.getLogicBox().size()[1]),int(alpha[4]*self.getLogicBox().size()[2])),
					PointNi(int(alpha[1]*self.getLogicBox().size()[0]),int(alpha[3]*self.getLogicBox().size()[1]),int(alpha[5]*self.getLogicBox().size()[2])))
		else:
			logic_box=box
			
		if time is None:
			time=self.getDefaultTime()
			
		if field is None:
			field=self.getDefaultField()

		# read data
		query = BoxQuery(self.dataset,field ,time ,  ord('r'))
		query.logic_box = logic_box
		resolution=resolution if resolution>0 else self.dataset.getMaxResolution()+resolution
		query.end_resolutions.push_back(resolution)
		self.dataset.beginQuery(query)
		print("Extracting data...","logic_box",logic_box.toString(),"dims",query.getNumberOfSamples().toString())
		if not self.dataset.executeQuery(self.createAccess(), query):
			raise Exception("query error %s" % (query.getLastErrorMsg(),))					
			
		query.buffer.bounds = Position(self.dataset.logicToPhysic(),Position(logic_box))
		data=query.buffer
		
		# compact dimension
		if True:
			dims=[data.dims[I] for I in range(data.dims.getPointDim()) if data.dims[I]>1 ]
			data.resize(PointNi(dims),data.dtype,__file__,0)
			
		print("done in %dmsec dims(%s) dtype(%s)" % (t1.elapsedMsec(),data.dims.toString(),data.dtype.toString()))
		return Array.toNumPy(data),Position(data.bounds)	
		
	# readSlice
	def readSlice(self,axis,offset):
		logic_box=BoxNi(self.getLogicBox())
		logic_box.p1.set(axis,offset+0)
		logic_box.p2.set(axis,offset+1)	
		data, physic_bounds=self.readData(logic_box)
		return (data,logic_box,physic_bounds.toAxisAlignedBox())
		
	# readSlices
	def readSlices(self, axis, delta=1):
		offsets=range(self.getLogicBox()	.p1[axis],self.getLogicBox().p2[axis],delta)
		for I,offset in enumerate(offsets):
			print("Doing","I","of",len(offsets))
			yield self.readSlice(axis,offset)	
			
	# exportSlicesToMovie
	def exportSlicesToMovie(self,out_movie, axis,preserve_ratio=True):
		
		for img, logic_box, physic_box in self.readSlices(axis):
			
			# resize to preserve the right ratio
			if preserve_ratio:
				density=[float(logic_box.size()[I])/float(physic_box.size()[I]) for I in range(3)]
				max_density=max(density)
				num_pixels=[int(physic_box.size()[I] * max_density) for I in range(3)]
				perm=((1,2,0),(0,2,1),(0,1,2))
				X,Y=perm[axis][0],perm[axis][1]				
				new_size=(num_pixels[X],num_pixels[Y])
				img=cv2.resize(img,new_size)

			out_movie.writeFrame(SwapRedBlue(img))
			
		out_movie.release()			
		
		
def LoadDatasetPy(url):
	return PyDataset(url)
	