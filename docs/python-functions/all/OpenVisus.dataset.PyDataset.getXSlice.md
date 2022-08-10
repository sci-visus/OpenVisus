---
layout: default
title: OpenVisus.dataset.PyDataset.getXSlice
parent: All Functions
grand_parent: Python Functions
nav_order: 2
---

# OpenVisus.dataset.PyDataset.getXSlice

Describe function here.

# Function Definition

```python
	def getXSlice(self, position=None, resolution=-1,resample_output=True): 
		"""
		Get a slice orthogonal to the X axis.
		resample_output=True	(resample to the full   resolutiuon) 
		resample_output=False (resample to the lower resolutiuon) 
		resample_output= (x,y) (resample to a (x,y) grid) 
		"""

		myLogicBox = self.getLogicBox()
		x_dim = myLogicBox[1][0]
		y_dim = myLogicBox[1][1]
		z_dim = myLogicBox[1][2]
		normalizationFactor = 2**(-resolution)
		
		if position==None:
			position = x_dim//2
		
		# adjust to slices that exist at this level of resolution
		position = (position//normalizationFactor) *normalizationFactor 
		
		# One slice is a volume
		data = self.read(x=[position,position+1],  y=[0,y_dim], z=[0,z_dim],quality=resolution*3)
		data = data[:,:,0]

		from skimage.transform import resize

		if resample_output==True:
			data = resize(data, (z_dim,y_dim), preserve_range=True).astype(data.dtype)
			
		elif	type(resample_output) is tuple:
			data = resize(data, resample_output, preserve_range=True).astype(data.dtype)
			
		return data
```