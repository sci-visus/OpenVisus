import os,sys,logging,types,time

import colorcet

from .utils import cbool
from .config import PALETTES
from .backend import LoadDataset

from bokeh.models import Select,LinearColorMapper,ColorBar,Button,Slider,TextInput,Row,Column,Div

logger = logging.getLogger(__name__)

# //////////////////////////////////////////////////////////////////////////////////////
class Widgets:

	ID=0

	# constructor
	def __init__(self,doc=None,disable_timers=False):
   
		self.id=Widgets.ID
		Widgets.ID+=1
		self.db=None		
		self.url=None
		self.access=None
		self.render_id=None # by default I am not rendering
		self.logic_to_pixel=[(0.0,1.0)]*3
		self.children=[]
  
		self.palette='Greys256'

		self.widgets=types.SimpleNamespace()

		# dataset
		self.widgets.dataset = Select(title="Dataset", options=[],width=100) 
		self.widgets.dataset.on_change("value",lambda attr,old,new: self.setDataset(new)) 
 
		# palette
		self.widgets.palette = Select(title='Palette', options=PALETTES,value=self.palette,width=100)
		self.widgets.palette.on_change("value",lambda attr, old, new: self.setPalette(new))  
		self.color_mapper = LinearColorMapper() 
		self.color_mapper.palette=self.widgets.palette.value
		self.color_mapper.low,self.color_mapper.high=(0.0,255.0) 
		self.color_bar = ColorBar(color_mapper=self.color_mapper)
  
		# num_views
		self.widgets.num_views=Select(title='#Views',  options=["1","2","3","4"],value='3',width=100)
		self.widgets.num_views.on_change("value",lambda attr, old, new: self.setNumberOfViews(int(new))) 
 
		# timestep
		self.widgets.timestep = Slider(title='Time', value=0, start=0, end=1, sizing_mode='stretch_width')
		self.widgets.timestep.on_change ("value",lambda attr, old, new: self.setTimestep(int(new)))  

		# timestep delta
		self.widgets.timestep_delta=Select(title="Time delta",options=["1","2","5","10","50","100","200"], value="1",width=100)
		self.widgets.timestep_delta.on_change("value", lambda attr, old, new: self.setTimestepDelta(int(new)))   

		# field
		self.widgets.field = Select(title='Field',  options=[],value='data',width=100)
		self.widgets.field.on_change("value",lambda attr, old, new: self.setField(new))  
  
		# direction 
		self.widgets.direction = Select(title='Direction', options=[('0','X'), ('1','Y'), ('2','Z')],value='2',width=100)
		self.widgets.direction.on_change ("value",lambda attr, old, new: self.setDirection(int(new)))  
  
		# offset 
		self.widgets.offset = Slider(title='Offset', value=0, start=0, end=1024, sizing_mode='stretch_width')
		self.widgets.offset.on_change ("value",lambda attr, old, new: self.setOffset(int(new)))
  
		# num_refimements (0==guess)
		self.widgets.num_refinements=Slider(title='#Refinements', value=0, start=0, end=4,width=100)
		self.widgets.num_refinements.on_change ("value",lambda attr, old, new: self.setNumberOfRefinements(int(new)))
  
		# quality (0==full quality, -1==decreased quality by half-pixels, +1==increase quality by doubling pixels etc)
		self.widgets.quality = Slider(title='Quality', value=0, start=-12, end=+12,width=100)
		self.widgets.quality.on_change("value",lambda attr, old, new: self.setQuality(int(new)))  

		# viewdep
		self.widgets.viewdep = Select(title="View Dep",options=[('1','Enabled'),('0','Disabled')], value="True",width=100)
		self.widgets.viewdep.on_change("value",lambda attr, old, new: self.setViewDependent(int(new)))  
  
		# status_bar
		self.widgets.status_bar= {}
		self.widgets.status_bar["request" ]=TextInput(title="" ,sizing_mode='stretch_width')
		self.widgets.status_bar["response"]=TextInput(title="" ,sizing_mode='stretch_width')
		self.widgets.status_bar["request" ].disabled=True
		self.widgets.status_bar["response"].disabled=True
  
 		# play time
		self.play=types.SimpleNamespace()
		self.play.is_playing=False
		self.widgets.play_button = Button(label="Play",width=80,sizing_mode='stretch_height')
		self.widgets.play_button.on_click(self.togglePlay)
		self.widgets.play_sec = Select(title="Play sec",options=["0.01","0.1","0.2","0.1","1","2"], value="0.01",width=120)
  
		# timer
		if not disable_timers:
			if doc is None:
				from bokeh.io import curdoc
				doc=curdoc()
			doc.add_periodic_callback(self.onIdle, 10)
   
	# onIdle
	def onIdle(self):
		self.playNextIfNeeded()
		for it in self.children:
			it.onIdle()
  
	# createGui
	def createGui(self,central_layout=None,options=[]):
	 
		options=[it.replace("-","_") for it in options]
		first_row=[getattr(self.widgets,it) for it in options if it!="status_bar"]
	 
		ret=Column(sizing_mode='stretch_both')
  
		ret.children.append(Row(
	  		children=first_row,
			sizing_mode="stretch_width"))

		if central_layout:
			ret.children.append(central_layout)
		
		if "status_bar" in options:
			ret.children.append(Row(
				self.widgets.status_bar["request"],
				self.widgets.status_bar["response"], 
				sizing_mode='stretch_width'))
  
		return ret
  

	# setWidgetsDisabled
	def setWidgetsDisabled(self,value):
		self.widgets.dataset.disabled=value
		self.widgets.palette.disabled=value
		self.widgets.num_views.disabled=value
		self.widgets.timestep.disabled=value
		self.widgets.timestep_delta.disabled=value
		self.widgets.field.disabled=value
		self.widgets.direction.disabled=value
		self.widgets.offset.disabled=value
		self.widgets.num_refinements.disabled=value
		self.widgets.quality.disabled=value
		self.widgets.viewdep.disabled=value
		self.widgets.status_bar["request" ].disabled=value
		self.widgets.status_bar["response"].disabled=value
		self.widgets.play_button.disabled=value
		self.widgets.play_sec.disabled=value
 		
		for it in self.children:
			it.setWidgetsDisabled(value)  

	# refresh (to override if needed)
	def refresh(self):
		for it in self.children:
			it.refresh()
   
	# getPointDim
	def getPointDim(self):
		return self.db.getPointDim() if self.db else 2

	# gotoPoint
	def gotoPoint(self, p):
		for it in self.children:
			it.gotoPoint(p) 
  
	# getDatasets
	def getDatasets(self):
		return self.widgets.dataset.options

	# getDatasets
	def setDatasets(self,value,title=None):
		logger.info(f"Widgets[{self.id}]::setDatasets num={len(value)}")
		self.widgets.dataset.options=value
		if title is not None: 
			self.widgets.dataset.title=title
		for it in self.children:
			it.setDatasets(value,title=title)
  
	# getLogicToPixel
	def getLogicToPixel(self):
		return self.logic_to_pixel

	# setLogicToPixel
	def setLogicToPixel(self,value):
		logger.info(f"Widgets[{self.id}]::setLogicToPixel value={value}")
		self.logic_to_pixel=value
		for it in self.children:
			it.setLogicToPixel(value)
		self.refresh()
  
	# setDataset
	def setDataset(self, url, db=None):
	 
		# rehentrant call
		if self.url==url:
			return 

		logger.info(f"Widgets[{self.id}]::setDataset value={url}")

		self.url=url
	 
		try:
			self.widgets.dataset.value=url
		except:
			self.widgets.dataset.options=[url]
			self.widgets.dataset.value=url
   
		self.db=LoadDataset(url) if db is None else db 
		self.access=self.db.createAccess()
  
		for it in self.children:
			it.setDataset(url, db=self.db) # avoid reloading db multiple times

		# timestep
		timesteps =self.db.getTimesteps()
		self.setTimesteps(timesteps)
		self.setTimestepDelta(1)
		self.setTimestep(timesteps[0])
  
		# direction
		pdim = self.db.getPointDim()
		self.setDirection(2)
		for I,it in enumerate(self.children):
			it.setDirection((I % 3) if pdim==3 else 2)

		# field
		fields    =self.db.getFields()
		self.setFields(fields)
		self.setField(fields[0]) 
  
		self.refresh() 
  
	# getNumberOfViews
	def getNumberOfViews(self):
		return int(self.widgets.num_views.value)

	# setNumberOfViews
	def setNumberOfViews(self,value):
		logger.info(f"Widgets[{self.id}]::setNumberOfViews value={value}")
		self.widgets.num_views.value=str(value)

	# getTimesteps
	def getTimesteps(self):
		return [int(value) for value in self.db.db.getTimesteps().asVector()]

	# setTimesteps
	def setTimesteps(self,value):
		logger.info(f"Widgets[{self.id}]::setTimesteps start={value[0]} end={value[-1]}")
		self.widgets.timestep.start =  value[0]
		self.widgets.timestep.end   =  value[-1]
		self.widgets.timestep.step  = 1

	# getTimestepDelta
	def getTimestepDelta(self):
		return int(self.widgets.timestep_delta.value)

	# setTimestepDelta
	def setTimestepDelta(self,value):
		logger.info(f"Widgets[{self.id}]::setTimestepDelta value={value}")
		self.widgets.timestep_delta.value=str(value)
		self.widgets.timestep.step=value
		A=self.widgets.timestep.start
		B=self.widgets.timestep.end
		T=self.getTimestep()
		T=A+value*int((T-A)/value)
		T=min(B,max(A,T))
		self.setTimestep(T)
  
		for it in self.children:
			it.setTimestepDelta(value)  
  
		self.refresh()

	# getTimestep
	def getTimestep(self):	
		return int(self.widgets.timestep.value)

	# setTimestep
	def setTimestep(self, value):
		logger.info(f"Widgets[{self.id}]::setTimestep value={value}")
		self.widgets.timestep.value=value
		for it in self.children:
			it.setTimestep(value)  
		self.refresh()
  
	# getFields
	def getFields(self):
		return self.widgets.field.options 
  
	# setFields
	def setFields(self, value):
		logger.info(f"Widgets[{self.id}]::setFields value={value}")
		self.widgets.field.options =list(value)

	# getField
	def getField(self):
		return str(self.widgets.field.value)

	# setField
	def setField(self,value):
		logger.info(f"Widgets[{self.id}]::setField value={value}")
		if value is None: return
		self.widgets.field.value=value
		for it in self.children:
			it.setField(value)  
		self.refresh()

	# getPalette
	def getPalette(self):
		return self.palette

	# setPalette
	def setPalette(self, value):	 
		logger.info(f"Widgets[{self.id}]::setPalette value={value}")
		self.palette=value
		self.widgets.palette.value=value
		self.color_mapper.palette=getattr(colorcet,value[len("colorcet."):]) if value.startswith("colorcet.") else value
		for it in self.children:
			it.setPalette(value)  
		self.refresh()

	# getPaletteRange
	def getPaletteRange(self):
		return self.color_mapper.low,self.color_mapper.high

	# setPaletteRange
	def setPaletteRange(self,value):
		logger.info(f"Widgets[{self.id}]::setPaletteRange value={value}")
		self.color_mapper.low, self.color_mapper.high=value  
		for it in self.children:
			it.setPaletteRange(value)     

	# getNumberOfRefinements
	def getNumberOfRefinements(self):
		return self.widgets.num_refinements.value

	# setNumberOfRefinements
	def setNumberOfRefinements(self,value):
		logger.info(f"Widgets[{self.id}]::setNumberOfRefinements value={value}")
		self.widgets.num_refinements.value=value
		for it in self.children:
			it.setNumberOfRefinements(value)      
		self.refresh()

	# getQuality
	def getQuality(self):
		return self.widgets.quality.value

	# setQuality
	def setQuality(self,value):
		logger.info(f"Widgets[{self.id}]::setQuality value={value}")
		self.widgets.quality.value=value
		for it in self.children:
			it.setQuality(value) 
		self.refresh()

	# getViewDepedent
	def getViewDepedent(self):
		return cbool(self.widgets.viewdep.value)

	# setViewDependent
	def setViewDependent(self,value):
		logger.info(f"Widgets[{self.id}]::setViewDependent value={value}")
		self.widgets.viewdep.value=str(int(value))
		for it in self.children:
			it.setViewDependent(value)     
		self.refresh()

	# getDirections
	def getDirections(self):
		return self.widgets.direction.options

	# setDirections
	def setDirections(self,value):
		logger.info(f"Widgets[{self.id}]::setDirections value={value}")
		self.widgets.direction.options=value
		for it in self.children:
			it.setDirections(value)

	# getDirection
	def getDirection(self):
		return int(self.widgets.direction.value)

	# setDirection
	def setDirection(self,value):
		logger.info(f"Widgets[{self.id}]::setDirection value={value}")
		pdim=self.getPointDim()
		if pdim==2: value=2
		dims=[int(it) for it in self.db.getLogicSize()]
		self.widgets.direction.value = str(value)

		# 2d there is no direction 
		pdim=self.getPointDim()
		if pdim==2:
			assert value==2
			self.widgets.offset.start, self.widgets.offset.end= 0,1-1
			self.widgets.offset.value = 0
		else:
			self.widgets.offset.start, self.widgets.offset.end = 0,int(dims[value])-1
			self.widgets.offset.value = int(dims[value]//2)
   
		# DO NOT PROPAGATE
		# for it in self.children:
		#	it.setDirection(value)
   
		self.refresh()

	# getOffset
	def getOffset(self):
		return self.widgets.offset.value

	# setOffset (3d only)
	def setOffset(self,value):
		logger.info(f"Widgets[{self.id}]::getOffset value={value}")
		self.widgets.offset.value=value
  
 		# DO NOT PROPAGATE
		# for it in self.children:
		#	it.setDirection(dir) 
  
		self.refresh()
  
	# ///////////////////////////////////////// PLAY

	# togglePlay
	def togglePlay(self,evt=None):
		if self.play.is_playing:
			self.stopPlay()  
		else:
			self.startPlay()
			
	# startPlay
	def startPlay(self):
		logger.info(f"Widgets[{self.id}]::startPlay")
		self.play.is_playing=True
		self.play.t1=time.time()
		self.play.wait_render_id=None
		self.play.num_refinements=self.getNumberOfRefinements()
		self.setNumberOfRefinements(1)
		self.setWidgetsDisabled(True)
		self.widgets.play_button.disabled=False
		self.widgets.play_button.label="Stop"
	
	# stopPlay
	def stopPlay(self):
		logger.info(f"Widgets[{self.id}]::stopPlay")
		self.play.is_playing=False
		self.play.wait_render_id=None
		self.setNumberOfRefinements(self.play.num_refinements)
		self.setWidgetsDisabled(False)
		self.widgets.play_button.disabled=False
		self.widgets.play_button.label="Play"
  

	# playNextIfNeeded
	def playNextIfNeeded(self):
	 
		if not self.play.is_playing: 
			return

		# avoid playing too fast by waiting a minimum amount of time
		t2=time.time()
		if (t2-self.play.t1)<float(self.widgets.play_sec.value):
			return   

		render_id = [self.render_id] + [it.render_id for it in self.children]

		if self.play.wait_render_id is not None:
			if any([a<b for (a,b) in zip(render_id,self.play.wait_render_id) if a is not None and b is not None]):
				# logger.info(f"Waiting render {render_id} {self.play.wait_render_id}")
				return

		# advance
		T=self.getTimestep()+self.getTimestepDelta()

		# reached the end -> go to the beginning?
		if T>=self.widgets.timestep.end: 
			T=self.timesteps.widgets.timestep.start
   
		logger.info(f"Widgets[{self.id}]::playing timestep={T}")
		
		# I will wait for the resolution to be displayed
		self.play.wait_render_id=[(it+1) if it is not None else None for it in render_id]
		self.play.t1=time.time()
		self.setTimestep(T) 