

import os,sys,logging
import numpy as np

from .utils import ConvertDataForRendering

from bokeh.plotting import figure
from bokeh.models   import ColumnDataSource,Range1d
from bokeh.events   import DoubleTap

logger = logging.getLogger(__name__)

# ////////////////////////////////////////////////////////////////////////////////////
class Canvas:
  
	# constructor
	def __init__(self, color_bar, color_mapper,sizing_mode='stretch_both'):
		self.sizing_mode=sizing_mode
		self.color_bar=color_bar
		self.color_mapper=color_mapper
		self.figure=figure(active_scroll = "wheel_zoom") 
		self.figure.x_range = Range1d(0,0)   
		self.figure.y_range = Range1d(512,512) 
		self.figure.toolbar_location="below"
		self.figure.sizing_mode = self.sizing_mode
		# self.figure.add_tools(bokeh.models.HoverTool(tooltips=[ ("(x, y)", "($x, $y)"),("RGB", "(@R, @G, @B)")])) # is it working?
  
		self.source_image = ColumnDataSource(data={"image": [np.random.random((300,300))*255], "x":[0], "y":[0], "dw":[256], "dh":[256]})  
		self.figure.image("image", source=self.source_image, x="x", y="y", dw="dw", dh="dh", color_mapper=self.color_mapper)  
		self.figure.add_layout(self.color_bar, 'right')
 
		self.points     = None
		self.dtype      = None


	# getWidth (this is number of pixels along X for the canvas)
	def getWidth(self):
		return self.figure.inner_width

	# getHeight (this is number of pixels along Y  for the canvas)
	def getHeight(self):
		return self.figure.inner_height    

	# enableDoubleTap
	def enableDoubleTap(self,fn):
		self.figure.on_event(DoubleTap, lambda evt: fn(evt.x,evt.y))

	  # getViewport
	def getViewport(self):
		return [
			self.figure.x_range.start,
			self.figure.y_range.start,
			self.figure.x_range.end,
			self.figure.y_range.end
		]

	  # getViewport
	def setViewport(self,x1,y1,x2,y2):
		if (x2<x1): x1,x2=x2,x1
		if (y2<y1): y1,y2=y2,y1

		W,H=self.getWidth(),self.getHeight()

		# fix aspect ratio
		if W>0 and H>0:
			assert(W>0 and H>0)
			w,cx =(x2-x1),x1+0.5*(x2-x1)
			h,cy =(y2-y1),y1+0.5*(y2-y1)
			if (w/W) > (h/H): 
				h=(w/W)*H 
			else: 
				w=(h/H)*W
			x1,y1=cx-w/2,cy-h/2
			x2,y2=cx+w/2,cy+h/2

		self.figure.x_range.start=x1
		self.figure.y_range.start=y1
		self.figure.x_range.end  =x2
		self.figure.y_range.end  =y2


	# renderPoints
	def renderPoints(self,points, size=20, color="red", marker="cross"):
		if self.points is not None: 
			self.figure.renderers.remove(self.points)
		self.points = self.figure.scatter(x=[p[0] for p in points], y=[p[1] for p in points], size=size, color=color, marker=marker)   
		assert self.points in self.figure.renderers


	# setImage
	def setImage(self, data, x1, y1, x2, y2):

		img=ConvertDataForRendering(data)
		dtype=img.dtype
 
		if self.dtype==dtype :
			# current dtype is 'compatible' with the new image dtype, just change the source _data
			self.source_image.data={"image":[img], "x":[x1], "y":[y1], "dw":[x2-x1], "dh":[y2-y1]}
		else:
			# need to create a new one from scratch
			self.figure.renderers=[]
			self.source_image = ColumnDataSource(data={"image":[img], "x":[x1], "y":[y1], "dw":[x2-x1], "dh":[y2-y1]})
			if img.dtype==np.uint32:	
				self.image_rgba=self.figure.image_rgba("image", source=self.source_image, x="x", y="y", dw="dw", dh="dh") 
			else:
				self.img=self.figure.image("image", source=self.source_image, x="x", y="y", dw="dw", dh="dh", color_mapper=self.color_mapper) 
			self.dtype=img.dtype
