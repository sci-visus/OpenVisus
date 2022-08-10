---
layout: default
title: OpenVisus.dataset.PyDataset.writeSlabs
parent: All Functions
grand_parent: Python Functions
nav_order: 2
---

# OpenVisus.dataset.PyDataset.writeSlabs

Describe function here.

# Function Definition

```python
	def writeSlabs(self,slices, x=0, y=0, z=0, time=None, field=None, max_memsize=4*1024*1024*1024, access=None):
		
		os.environ["VISUS_DISABLE_WRITE_LOCK"]="1"
		
		slab=[]
		memsize=0
		
		for slice in slices:
			slab.append(slice)
			memsize+=slice.nbytes
			
			# flush
			if memsize>=max_memsize: 
				data=numpy.stack(slab,axis=0)
				self.write(data , x=x, y=y, z=z,field=field,time=time, access=access)
				z+=len(slab)
				slab=[]
				memsize=0

		# flush
		if slab: 
			data=numpy.stack(slab,axis=0)
			self.write(data , x=x, y=y, z=z,field=field,time=time, access=access)
```