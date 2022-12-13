import os,sys
import numpy as np

import bokeh.io
import bokeh.events
import bokeh.models
import bokeh.plotting 

# ////////////////////////////////////////////////////////////////////////////////////
class Canvas:
  
	# constructor
	def __init__(self,color_bar, color_mapper):

		self.color_bar=color_bar
		self.color_mapper=color_mapper
  
		self.figure=bokeh.plotting.Figure(active_scroll = "wheel_zoom")
		self.figure.x_range = bokeh.models.Range1d(0,1024)   
		self.figure.y_range = bokeh.models.Range1d(0,768) 
		self.figure.toolbar_location="below"
		self.figure.sizing_mode = 'stretch_both'
		# self.figure.add_tools(bokeh.models.HoverTool(tooltips=[ ("(x, y)", "($x, $y)"),("RGB", "(@R, @G, @B)")])) # is it working?
  
		self.source_image = bokeh.models.ColumnDataSource(data={"image": [np.random.random((300,300))*255], "x":[0], "y":[0], "dw":[256], "dh":[256]})  
		self.figure.image("image", source=self.source_image, x="x", y="y", dw="dw", dh="dh", color_mapper=self.color_mapper)  
		self.figure.add_layout(self.color_bar, 'right')
 
		self.points     = None
		self.dtype      = None

	# getWidth
	def getWidth(self):
		return self.figure.inner_width

	# getHeight
	def getHeight(self):
		return self.figure.inner_height    

  	# getViewport
	def getViewport(self):
		x1,x2=self.figure.x_range.start, self.figure.x_range.end
		y1,y2=self.figure.y_range.start, self.figure.y_range.end 
		return (x1,y1,x2,y2)

  	# getViewport
	def setViewport(self,x1,y1,x2,y2):
		# fix aspect ratio
		W,H=self.getWidth(),self.getHeight()
		if W and H: 
			ratio=W/H
			w, h, cx, cy=(x2-x1),(y2-y1),0.5*(x1+x2),0.5*(y1+y2)
			w,h=(h*ratio,h) if W>H else (w,w/ratio) 
			x1,y1,x2,y2=cx-w/2,cy-h/2, cx+w/2, cy+h/2
		self.figure.x_range.start=x1
		self.figure.x_range.end  =x2
		self.figure.y_range.start=y1
		self.figure.y_range.end  =y2

	# renderPoints
	def renderPoints(self,points,size=20,color="red",marker="cross"):
		print("renderPoints",points)
		if self.points is not None: 
			self.figure.renderers.remove(self.points)
		self.points = self.figure.scatter(x=[p[0] for p in points], y=[p[1] for p in points], size=size, color=color, marker=marker)   
		assert self.points in self.figure.renderers

	# __getImage
	def __getImage(self, data):

		# grayscale is just fine
		if len(data.shape)==2:
			return data
		else:
    
			assert(len(data.shape)==3)   # the 3rd is the number of channels
			nchannels=data.shape[-1]
	  
			if nchannels==2:
				G,A=data[:,:,1],data[:,:,0] # gray,alpha -> RGBA
				ret=np.dstack([G,G,G,A])
	 
			elif nchannels==3:
				default_alpha=255 if data.dtype==np.uint8 else 1.0 # NOT SURE ABOUT THIS!
				R,G,B=data[:,:,0],data[:,:,1],data[:,:,2] # RGB -> RGBA
				A=np.full(R.shape, default_alpha,dtype=R.dtype)
				ret= np.dstack([R,G,B,A])

			else:
				raise Exception(f"to handle shape={data.shape} dtype={data.dtype} nchannels={nchannels}")   
 
			# what to do with other dtypes?
			assert(data.dtype==np.uint8) 
			return ret.view(dtype=np.uint32).reshape(ret.shape[0:2])

	# renderImage
	def renderImage(self, data, x1, y1, x2, y2):
   
		img=self.__getImage(data)
		dtype=img.dtype
 
		if self.dtype==dtype :
			# just change the soource _data
			self.source_image.data={"image":[img], "x":[x1], "y":[y1], "dw":[x2-x1], "dh":[y2-y1]}
		else:
			# need to create a new one from scratch
			self.figure.renderers=[]
			self.source_image = bokeh.models.ColumnDataSource(data={"image":[img], "x":[x1], "y":[y1], "dw":[x2-x1], "dh":[y2-y1]})
			if img.dtype==np.uint32:	
				self.image_rgba=self.figure.image_rgba("image", source=self.source_image, x="x", y="y", dw="dw", dh="dh") 
			else:
				self.img=self.figure.image("image", source=self.source_image, x="x", y="y", dw="dw", dh="dh", color_mapper=self.color_mapper) 
			self.dtype=img.dtype