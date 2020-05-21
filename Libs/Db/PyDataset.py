import os,sys
import numpy 
import inspect

from OpenVisus import *


# //////////////////////////////////////////////
class PyDataset(object):
	
	# constructor
	def __init__(self,url):
		self.db = LoadDataset(url)

	# Create
	@staticmethod
	def Create(url,**args):
		
		dims,fields,data=None, None,None
		
		if "data" in args:
			data=args["data"]
			dim=int(args["dim"]); Assert(dim>=2)
			buffer=Array.fromNumPy(data,TargetDim=dim, bShareMem=True)
			field=Field("data",buffer.dtype,"row_major")
			dims=buffer.dims
			fields=[field]

		if "dims" in args:
			dims=args["dims"]

		if "fields" in args:
			fields=args["fields"]
		
		dims=PointNi(dims)
		pdim=dims.getPointDim()

		idx=IdxFile()
		idx.logic_box=BoxNi(PointNi.zero(pdim),dims)
		
		for field in fields:
			idx.fields.push_back(field)

		idx.save(url)

		if data is not None:
			db=PyDataset(url)
			db.write(data)

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

		if isinstance(field,string):
			return self.db.getFieldByName(name)
			
		return field
		
	# createAccess
	def createAccess(self):
		return self.db.createAccess()

	# read
	def read(self, logic_box=None, x=None, y=None, z=None, time=None, field=None, num_refinements=1, quality=0, max_resolution=None):
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
		print(data.shape,dims,query.logic_box.toString())
		
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


				

			
			





