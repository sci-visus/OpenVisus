---
layout: default
title: OpenVisus.dataset.PyDataset.read
parent: All Functions
grand_parent: Python Functions
nav_order: 2
---

# OpenVisus.dataset.PyDataset.read

Describe function here.

# Function Definition

```python
	def read(self, logic_box=None, x=None, y=None, z=None, time=None, field=None, num_refinements=1, quality=0, max_resolution=None, disable_filters=False, access=None):
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

		field=self.getField() if field is None else self.getField(field)	
			
		if time is None:
			time = self.getTime()			

		if logic_box is None:
			logic_box=self.getLogicBox(x,y,z)

		if isinstance(logic_box,(tuple,list)):
			logic_box=BoxNi(PointNi(logic_box[0]),PointNi(logic_box[1]))

		query = self.db.createBoxQuery(BoxNi(logic_box), field , time, ord('r'))
		
		if disable_filters:
			query.disableFilters()
		else:
			query.enableFilters()
		
		if max_resolution is None:
			max_resolution=self.getMaxResolution()
		
		# example quality -3 means not full resolution
		Assert(quality<=0)
		max_resolution=max_resolution+quality 
		
		for I in reversed(range(num_refinements)):
			res=max_resolution-(pdim*I)
			if res>=0:
				query.end_resolutions.push_back(res)
		
		self.db.beginBoxQuery(query)
		
		if not query.isRunning():
			raise Exception("begin query failed {0}".format(query.errormsg))
			
		if not access:
			access=self.db.createAccess()
			
		def NoGenerator():
			if not self.db.executeBoxQuery(access, query):
				raise Exception("query error {0}".format(query.errormsg))
			# i cannot be sure how the numpy will be used outside or when the query will dealllocate the buffer
			data=Array.toNumPy(query.buffer, bShareMem=False) 
			return data
			
		def WithGenerator():
			while query.isRunning():

				if not self.db.executeBoxQuery(access, query):
					raise Exception("query error {0}".format(query.errormsg))

				# i cannot be sure how the numpy will be used outside or when the query will dealllocate the buffer
				data=Array.toNumPy(query.buffer, bShareMem=False) 
				yield data
				self.db.nextBoxQuery(query)	

		return NoGenerator() if query.end_resolutions.size()==1 else WithGenerator()
```