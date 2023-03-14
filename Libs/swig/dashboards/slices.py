
import os,sys,io,types,threading,time,logging
import numpy as np

from bokeh.models import Select,Column,Row

from .widgets import Widgets
from .slice   import Slice

logger = logging.getLogger(__name__)

# //////////////////////////////////////////////////////////////////////////////////////
class Slices(Widgets):

	# constructor
	def __init__(self, doc=None,panel_state=None,sizing_mode='stretch_both', show_options=[
			  "num_views","palette","timestep","field","viewdep","quality","!num_refinements",
			"!direction","!offset" # this will be show on the single SLice
		 ]):
		super().__init__(doc=doc)
		self.logic_to_pixel=[(0.0,1.0)] * 3 # translation/scaling for each dimensions
		self.panel_state=panel_state 
		self.sizing_mode=sizing_mode
		self.slice_show_options=["direction","offset"]
		self.palette="Greys256"
		self.palette_range=[0,255]
		self.layout=self.createGui(central_layout=None, options=show_options)

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

		 # clear current central layout
		while len(self.layout.children)>1:
			self.layout.children.pop()  

		# create central panel
		if value is None or value==0:
			value=1

		self.children=[]

		for direction in range(value):
			it=Slice(doc=self.doc,sizing_mode=self.sizing_mode,logic_to_pixel=self.logic_to_pixel,show_options=self.slice_show_options)
			it.canvas.enableDoubleTap(lambda x,y: self.gotoPoint(self.unproject([x,y])))
			it.setDataset(self.db)
			it.setDirection(direction % 3)
			self.children.append(it)

		if value==1:
			self.children=[self.children[0]]
			self.layout.children.append(self.children[0].layout)

		elif value==2:
			self.layout.children.append(Row(
				   self.children[0].layout,
				  self.children[1].layout, 
				sizing_mode=self.sizing_mode) )

		elif value==3:
			self.layout.children.append(Row(
				   self.children[2].layout, 
				  Column(
					self.children[0].layout,
					 self.children[1].layout,
					  sizing_mode=self.sizing_mode),
				sizing_mode=self.sizing_mode))
   
		elif value==4:
			self.layout.children.append(Grid(children=[
				self.children[0].layout,
				self.children[1].layout,
				self.children[2].layout,
				self.children[3].layout,
			   ],
			nrows=2, 
			ncols=2, 
			sizing_mode=self.sizing_mode))

		else:
			raise Exception("internal error")

		self.setDataset(self.db, compatible=True)

