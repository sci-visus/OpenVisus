
import os,sys,io,types,threading,time,logging
import numpy as np

from bokeh.models import Select,Column,Row

from .widgets import Widgets
from .slice import Slice

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
		self.slices=[]
		self.widgets.num_views=Select(title='#Views',  options=["1","2","3","4"],value='3',width=50,sizing_mode='stretch_height')
		self.widgets.num_views.on_change("value",lambda attr, old, new: self.setNumberOfViews(int(new)))
		self.slice_show_options=["direction","offset"]
		self.palette="Greys256"
		self.palette_range=[0,255]
		self.createGui(show_options)

	# createGui
	def createGui(self,options):

		# create first row
		v=[]

		if "num_views" in options:
			v.append(self.widgets.num_views)
   
		if "palette" in options:
			v.append(self.widgets.palette)
  
		if "timestep" in options:  
			v.append(self.widgets.timestep)

		if "timestep_delta" in options:
			v.append(self.widgets.timestep_delta)

		if "field" in options:
			v.append(self.widgets.field)
   
		if "viewdep" in options:
			v.append(self.widgets.viewdep)

		if "quality" in options:
			v.append(self.widgets.quality) 
  
		if "num_refinements" in options:
			v.append(self.widgets.num_refinements)  

		if "play-button" in options:
			v.append(self.play.button)
			
		if "play-msec" in options:
			v.append(self.play.sec)

		self.layout=Column(
			Row(*v, sizing_mode='stretch_width'),
			sizing_mode=self.sizing_mode)

	# __iter__
	def __iter__(self):
		return iter(self.slices)

	# setWidgetsEnabled
	def setWidgetsDisabled(self,value):
		super().setWidgetsDisabled(value)
		self.widgets.num_views.disabled=value
		for slice in self.slices:
			slice.setWidgetsDisabled(value)

	# setTimestep
	def setTimestep(self, value):
		super().setTimestep(value)
		for slice in self.slices:
			slice.setTimestep(value)

	# setField
	def setField(self, value):
		super().setField(value)
		for slice in self.slices:
			slice.setField(value)
  
	# setPalette
	def setPalette(self,value, palette_range=None):
		logger.info(f"Slices::setPalette value={value} palette_range={palette_range} #slices={len(self.slices)}")
		super().setPalette(value, palette_range=palette_range)
		for I,slice in enumerate(self.slices):
			slice.setPalette(value,palette_range=palette_range)

	# setQuality
	def setQuality(self,value):
		logger.info(f"Slices::setQuality value={value} ")    
		super().setQuality(value)
		for slice in self.slices:
			slice.setQuality(value)   
   
	 # setNumberOfRefinements
	def setNumberOfRefinements(self,value):
		logger.info(f"Slices::setNumberOfRefinements value={value} ")    
		super().setNumberOfRefinements(value)
		for slice in self.slices:
			slice.setNumberOfRefinements(value)     

	# setTimestepDelta
	def setTimestepDelta(self,value):
		logger.info(f"Slices::setTimestepDelta value={value} ")  
		super().setTimestepDelta(value)
		for slice in self.slices:
			slice.setTimestepDelta(value)     


	# setViewDependent
	def setViewDependent(self,value):
		super().setViewDependent(value)
		for slice in self.slices:
			slice.setViewDependent(value)   

	# setDataset
	def setDataset(self, db, compatible=False):
		self.db=db
		
		for slice in self.slices:
			dir=slice.getDirection()
			slice.setDataset(db)
			slice.setDirection(dir) # keep at least the direction, others will be set here

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

		assert(self.db)

		self.widgets.num_views.value=str(value)

		# clear all slices
		for slice in self.slices:
			slice.aborted.setTrue()
			slice.query.stopThread()

		self.slices=[]  

		 # clear current central layout
		while len(self.layout.children)>1:
			self.layout.children.pop()  

		# create central panel
		if value is None or value==0:
			value=1

		self.slices=[]

		for direction in range(value):
			slice=Slice(doc=self.doc,sizing_mode=self.sizing_mode,logic_to_pixel=self.logic_to_pixel,show_options=self.slice_show_options)
			slice.canvas.enableDoubleTap(lambda x,y: self.gotoPoint(self.unproject([x,y])))
			slice.setDataset(self.db)
			slice.setDirection(direction % 3)
			self.slices.append(slice)

		if value==1:
			self.slices=[self.slices[0]]
			self.layout.children.append(self.slices[0].layout)

		elif value==2:
			self.layout.children.append(Row(
				   self.slices[0].layout,
				  self.slices[1].layout, 
				sizing_mode=self.sizing_mode) )

		elif value==3:
			self.layout.children.append(Row(
				   self.slices[2].layout, 
				  Column(
					self.slices[0].layout,
					 self.slices[1].layout,
					  sizing_mode=self.sizing_mode),
				sizing_mode=self.sizing_mode))
   
		elif value==4:
			self.layout.children.append(Grid(children=[
				self.slices[0].layout,
				self.slices[1].layout,
				self.slices[2].layout,
				self.slices[3].layout,
			   ],
			nrows=2, 
			ncols=2, 
			sizing_mode=self.sizing_mode))

		else:
			raise Exception("internal error")

		self.setDataset(self.db, compatible=True)

	# gotoPoint
	def gotoPoint(self, p):
		for slice in self.slices:
			slice.gotoPoint(p)
