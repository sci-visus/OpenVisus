
import os,sys,io,types,threading,time,logging
import numpy as np

from bokeh.models import Select,Column,Row,Grid

from .widgets import Widgets
from .slice   import Slice

logger = logging.getLogger(__name__)

# //////////////////////////////////////////////////////////////////////////////////////
class Slices(Widgets):

	# constructor
	def __init__(self, 
			show_options=["num_views","palette","timestep","field","viewdep","quality"],
			slice_show_options=["direction","offset","viewdep","status_bar"]):
		super().__init__()
		self.slice_show_options=slice_show_options
		self.central_layout=Column(sizing_mode='stretch_both')
		self.layout=self.createGui(central_layout=self.central_layout, options=show_options)

	# setDataset
	def setDataset(self, db, compatible=False):
		self.db=db
		
		for it in self.children:
			dir=it.getDirection()
			it.setDataset(db)
			it.setDirection(dir) # keep at least the direction, others will be set here

		# my settings should override the single slices
		# NOTE: the slices could be brand new in case of setNumberOfViews

		timesteps=db.getTimesteps()
		fields=db.getFields()

		timestep_delta        = self.getTimestepDelta() if compatible else 1
		timestep              = self.getTimestep()      if compatible else timesteps[0]
		field                 = self.getField()         if compatible else fields[0]
		palette,palette_range = self.getPalette(),self.getPaletteRange()
		viewdep               = self.getViewDepedent()

		self.setTimestepDelta(timestep_delta) 
		self.setTimesteps(timesteps)
		self.setTimestep(timestep)
		self.setFields(fields)
		self.setField(field)
		self.setPalette(palette,palette_range) 
		self.setViewDependent(viewdep)

	# setNumberOfViews
	def setNumberOfViews(self,value):
		super().setNumberOfViews(value)
		
		assert(self.db)

		# clear all children
		for it in self.children:
			it.aborted.setTrue()
			it.query.stopThread()
		self.children=[]

		for direction in range(value):
			it=Slice(show_options=self.slice_show_options)
			it.canvas.enableDoubleTap(lambda x,y: self.gotoPoint(self.unproject([x,y])))
			it.setLogicToPixel(self.getLogicToPixel())
			it.setDataset(self.db)
			it.setDirection(direction % 3)
			self.children.append(it)
   
		layouts=[it.layout for it in self.children]

		if value<=2:
			self.central_layout.children=[Row(*layouts, sizing_mode='stretch_both')]

		elif value==3:
			self.central_layout.children=[Row(layouts[2], Column(*layouts[0:2],sizing_mode='stretch_both'),sizing_mode='stretch_both')]
   
		elif value==4:
			self.central_layout.children=[Grid(*layouts,nrows=2, ncols=2, sizing_mode='stretch_both')]

		else:
			raise Exception("internal error")

		self.setDataset(self.db, compatible=True)

