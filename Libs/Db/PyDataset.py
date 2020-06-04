import os,sys
import numpy 
import inspect

from OpenVisus import *

# //////////////////////////////////////////////////////////
def CreateIdx(**args):

	if not "url" in args:
		raise Exception("url not specified")

	url=args["url"]

	idx=IdxFile()
		
	if "data" in args:
		data=args["data"]
		dim=int(args["dim"])
		Assert(dim>=2) # you must specify the point dim since it could be that data has multiple components
		buffer=Array.fromNumPy(data,TargetDim=dim)
		idx.logic_box=BoxNi(PointNi.zero(dim),PointNi(buffer.dims))
		N=1 if dim==len(data.shape) else data.shape[-1]
		idx.fields.push_back(Field.fromString("DATA uint8[{}] default_layout(row_major)".format(N)))

	else:
		if not "dims" in args:
			raise Exception("please specify dimensions")

		dims=PointNi(args["dims"])
		idx.logic_box=BoxNi(PointNi.zero(dims.getPointDim()),dims)

	# add fields
	if "fields" in args:
		for field in  args["fields"]:
			idx.fields.push_back(field)

	# bitsperblock
	if "bitsperblock" in args:
		idx.bitsperblock=int(args["bitsperblock"])

	# blocks per file
	if "blocksperfile" in args:
		idx.samplesperblock=int(args["blocksperfile"])
		
	# is the user specifying filters?
	if "filters" in args:
		filters=args["filters"]
		for I in range(idx.fields.size()):
			idx.fields[I].filter=filters[I]

	if "time" in args:
		A,B,time_template=args["time"]
		idx.timesteps=DatasetTimesteps(A,B,1.0)
		idx.time_template=time_template

	idx.save(url)
	db=PyDataset(url)

	if "data" in args:
		db.write(args["data"])
			
	return db

# //////////////////////////////////////////////
class PyDataset(object):
	
	# constructor
	def __init__(self,url):
		self.db = LoadDataset(url)

	# Create
	@staticmethod
	def Create(**args):
		return CreateIdx(args)

	# __getattr__
	def __getattr__(self,attr):
	    return getattr(self.db, attr)	

	# getPointDim
	def getPointDim(self):
		return self.db.getPointDim()

	# getMaxResolution
	def getMaxResolution(self):
		return self.db.getMaxResolution()

	# getLogicBox
	def getLogicBox(self,x=None,y=None,z=None):
		pdim=self.getPointDim()
		lbox=self.db.getLogicBox()
		A=[lbox.p1[I] for I in range(pdim)]
		B=[lbox.p2[I] for I in range(pdim)]
		p1,p2=[0]*pdim,[0]*pdim
		for I in range(pdim):
			r=(x,y,z)[I]
			if r is None: r=[A[I],B[I]]
			p1[I] = int( A[I]+r[0]*(B[I]-A[I]) if isinstance(r[0],float) else r[0])
			p2[I] = int( A[I]+r[1]*(B[I]-A[I]) if isinstance(r[1],float) else r[1])
		return (p1,p2)
		
	# getSliceLogicBox
	def getSliceLogicBox(self,axis,offset):
		ret=self.getLogicBox()
		p1[axis]=offset+0
		p1[axis]=offset+1
		return (p1,p2)
		
	# getBounds
	def getBounds(self, logic_box):
		
		if isinstance(logic_box,(tuple,list)):
			logic_box=BoxNi(PointNi(logic_box[0]),PointNi(logic_box[1]))
			
		return Position(self.logicToPhysic(),Position(BoxNi(logic_box)))

	# getLogicSize
	def getLogicSize(self):
		p1,p2=self.getLogicBox()
		return numpy.subtract(p2,p1)

	# getFields
	def getFields(self):
		return [field.name for field in self.db.getFields()]
		
	# getField
	def getField(self,value):
		
		if value is None:
			return self.db.getDefaultField()

		if isinstance(value,str):
			return self.db.getFieldByName(value)
			
		return value
		
	# createAccess
	def createAccess(self):
		return self.db.createAccess()

	# read
	def read(self, logic_box=None, x=None, y=None, z=None, time=None, field=None, num_refinements=1, quality=0, max_resolution=None, disable_filters=False):
		"""
		db=PyDataset.Load(url)
		
		# example of reading a single slice in logic coordinates
		data=db.read(z=[512,513]) 
		
		# example of reading a single slice in normalized coordinates (i.e. [0,1])
		data.db.read(x=[0,0.1],y=[0,0.1],z=[0,0.1])
		
		# example of reading a single slice with 3 refinements
		for data in db.read(z=[512,513],num_refinements=3):
			print(data)

		"""
		
		pdim=self.getPointDim()


		field=self.getField(field)	
			
		if time is None:
			time = self.getDefaultTime()			

		query = BoxQuery(self.db, field , time, ord('r'))
		
		if disable_filters:
			query.disableFilters()
		
		if logic_box is None:
			logic_box=self.getLogicBox(x,y,z)
			
		if isinstance(logic_box,(tuple,list)):
			logic_box=BoxNi(PointNi(logic_box[0]),PointNi(logic_box[1]))
			
		query.logic_box=BoxNi(logic_box)

		if max_resolution is None:
			max_resolution=self.getMaxResolution()
		
		# example quality -3 means not full resolution
		Assert(quality<=0)
		max_resolution=max_resolution+quality 
		
		for I in reversed(range(num_refinements)):
			res=max_resolution-(pdim*I)
			if res>=0:
				query.end_resolutions.push_back(res)
		
		self.db.beginQuery(query)
		
		if not query.isRunning():
			raise Exception("begin query failed {0}".format(query.getLastErrorMsg()))
			
		access=self.db.createAccess()
		while query.isRunning():

			if not self.db.executeQuery(access, query):
				raise Exception("query error {0}".format(query.getLastErrorMsg()))
				
			# i cannot be sure how the numpy will be used outside or when the query will dealllocate the buffer
			data=Array.toNumPy(query.buffer, bShareMem=False) 
			yield data
			
			self.db.nextQuery(query)	

	# write
	def write(self, data, x=0, y=0, z=0, time=None, field=None):

		"""
		db=PyDataset.Load(url)
		width,height,depth=db.getSize()

		# write single slice
		data=numpy.zeros([height,width,3],dtype.uint8)
		db.write(data,z=[512,513]) 

		# write several slices in one-shot
		nslices=10
		data=numpy.zeros([nslices,height,width,10,3],dtype.uint8)
		db.write(data,z=[512,512+nslices])

		# write several slices with a generator
		nslices=10
		def gen():
			for I in range(nslices):
				yield=p.zeros([height,width,3],dtype.uint8)
		db.write(gen,z=512)
		"""
		
		pdim=self.getPointDim()
		
		field=self.getField(field)
		
		if time is None:
			time = self.getDefaultTime()
			
		query = BoxQuery(self.db, field , time , ord('w'))
		
		dims=list(data.shape)
		
		# remove last components
		if field.dtype.ncomponents()>1:
			dims=dims[:-1]
		
			# could be I'm writing a slice, I need to increment the "dimension"
		while len(dims)<pdim: 
			dims=[1] + dims	
		
		dims=list(reversed(dims))	
		
		p1=PointNi([x,y,z][0:pdim])
		query.logic_box=BoxNi(p1,p1+PointNi(dims))
		
		query.end_resolutions.push_back(self.getMaxResolution())
		
		self.db.beginQuery(query)
		
		if not query.isRunning():
			raise Exception("begin query failed {0}".format(query.getLastErrorMsg()))
			
		access=self.createAccess()
		
		# I need to change the shape of the buffer, since the last component is the channel (like RGB for example)
		buffer=Array.fromNumPy(data,bShareMem=True)
		Assert(buffer.c_size()==data.nbytes)
		buffer.resize(PointNi(dims),query.field.dtype,__file__,0)
		
		query.buffer=buffer
		
		if not self.db.executeQuery(access, query):
			raise Exception("query error {0}".format(query.getLastErrorMsg()))
			
	# writeSlabs
	def writeSlabs(self,slices, x=0, y=0, z=0, time=None, field=None, max_memsize=1024*1024*1024):
		
		os.environ["VISUS_DISABLE_WRITE_LOCK"]="1"
		
		slab=[]
		memsize=0
		
		for slice in slices:
			slab.append(slice)
			memsize+=slice.nbytes
			
			# flush
			if memsize>=max_memsize: 
				data=numpy.stack(slab,axis=0)
				self.write(data , x=x, y=y, z=z,field=field,time=time)
				z+=len(slabs)
				slab=[]
				memsize=0

		# flush
		if slab: 
			data=numpy.stack(slab,axis=0)
			self.write(data , x=x, y=y, z=z,field=field,time=time)		



			



