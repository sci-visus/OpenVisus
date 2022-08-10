---
layout: default
title: OpenVisus.dataset.PyDataset.getXYSlice
parent: All Functions
grand_parent: Python Functions
nav_order: 2
---

# OpenVisus.dataset.PyDataset.getXYSlice

Describe function here.

# Function Definition

```python
	def getXYSlice(self, position=None, XY_MinMax=None, resolution=-1, resample_output=True, time=None, field=None):
		"""
		Get a slice orthogonal to the X axis.
		resample_output=True	(resample to the full   resolutiuon) 
		resample_output=False (resample to the lower resolutiuon) 
		resample_output= (x,y) (resample to a (x,y) grid) 
		"""
		def normalize_position(position,axis,myLogicBox,resolution=-1):
			normalizationFactor = 2 ** (-resolution)
			axis_dim = myLogicBox[1][axis]
			if position == None:  # if None pick middle slice
				position = axis_dim // 2
			elif isinstance(position, float):
				if position > 1:  # if out of range bring back to the limit
					position = int(axis_dim - 1)
				elif position < 0:
					position = int(0)
				else:  # Convert the ratio to an integer
					position = int(position * (axis_dim - 1))

			# adjust to slices that exist at this level of resolution
			position = (position // normalizationFactor) * normalizationFactor
			return position

		#xMin = normalize_position(XY_MinMax[0][0],0 myLogicBox,resolution=resolution)

		myLogicBox = self.getLogicBox()
		spaceDim = len(myLogicBox[1])
		x_dim = myLogicBox[1][0]
		y_dim = myLogicBox[1][1]
		if spaceDim == 2:
			z_dim = position = None
		else:
			z_dim = myLogicBox[1][2]
			position  = normalize_position(position,2,myLogicBox,resolution=resolution)

		if XY_MinMax==None:
			x_min, x_max, y_min, y_max = 0, x_dim, 0, y_dim
		else:
			x_min = normalize_position(XY_MinMax[0],0,myLogicBox,resolution=resolution)
			x_max = normalize_position(XY_MinMax[1],0,myLogicBox,resolution=resolution)
			y_min = normalize_position(XY_MinMax[2],1,myLogicBox,resolution=resolution)
			y_max = normalize_position(XY_MinMax[3],1,myLogicBox,resolution=resolution)
		#print("XY_MinMax=",x_min, x_max, y_min, y_max)
		x_dim, y_dim = x_max-x_min, y_max-y_min
		# One slice is a volume
		if spaceDim == 2:
			data = self.read(x=[x_min,x_max], y=[y_min,y_max],  quality=resolution*2, time=time, field=field)
		else:
			data = self.read(x=[x_min,x_max], y=[y_min,y_max], z=[position,position+1],quality=resolution*3, time=time, field=field)
		#print("data.shape=",data.shape)
		if spaceDim > 2:
			data = data[0,:,:]

		from skimage.transform import resize

		if resample_output==True:
			data = resize(data, (z_dim,y_dim), preserve_range=True).astype(data.dtype)
			
		elif	type(resample_output) is tuple:
			data = resize(data, resample_output, preserve_range=True).astype(data.dtype)

			
		return data
```