---
layout: default
title: OpenVisus.dataset.PyDataset.write
parent: All Functions
grand_parent: Python Functions
nav_order: 2
---

# OpenVisus.dataset.PyDataset.write

Describe function here.

# Function Definition

```python
	def write(self, data, x=0, y=0, z=0,logic_box=None, time=None, field=None, access=None):

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
			time = self.getTime()


		dims=list(data.shape)
		
		# remove last components
		if field.dtype.ncomponents()>1:
			dims=dims[:-1]
		
			# could be I'm writing a slice, I need to increment the "dimension"
		while len(dims)<pdim: 
			dims=[1] + dims	
		
		dims=list(reversed(dims))	

		if logic_box is None:
			p1=PointNi([x,y,z][0:pdim])
			logic_box=BoxNi(p1,p1+PointNi(dims))

		if isinstance(logic_box,(tuple,list)):
			logic_box=BoxNi(PointNi(logic_box[0]),PointNi(logic_box[1]))

		query = self.db.createBoxQuery(logic_box, field , time , ord('w'))
		query.end_resolutions.push_back(self.getMaxResolution())
		
		self.db.beginBoxQuery(query)
		
		if not query.isRunning():
			raise Exception("begin query failed {0}".format(query.errormsg))
			
		if not access:
			access=IdxDiskAccess.create(self.db)
			access.disableAsync()
			access.disableWriteLock()
		
		# I need to change the shape of the buffer, since the last component is the channel (like RGB for example)
		buffer=Array.fromNumPy(data,bShareMem=True)
		Assert(buffer.c_size()==data.nbytes)
		buffer.resize(PointNi(dims),query.field.dtype,__file__,0)
		
		query.buffer=buffer
		
		if not self.db.executeBoxQuery(access, query):
			raise Exception("query error {0}".format(query.errormsg))
```