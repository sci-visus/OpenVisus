import os,sys,logging,types,time

from .utils import cbool
from .config import DIRECTIONS, PALETTES
from .backend import LoadDataset

from bokeh.models import Select,LinearColorMapper,ColorBar,Button,Slider,TextInput,Row,Column,Div
from bokeh.io import curdoc

logger = logging.getLogger(__name__)

# //////////////////////////////////////////////////////////////////////////////////////
class Widgets:

	# constructor
	def __init__(self):
   
		self.db=None
		self.access=None
		self.render_id=None # by default I am not rendering
		self.logic_to_pixel=[(0.0,1.0)]*3
		self.children=[]
  
		self.palette='Greys256'
		self.palette_range=(0,255) 

		self.widgets=types.SimpleNamespace()

		# dataset
		self.widgets.dataset = Select(title="Dataset", options=[],width=100) 
		self.widgets.dataset.on_change("value",lambda attr,old,new: self.setDataset(LoadDataset(new),compatible=True)) 
 
		# palette
		self.widgets.palette = Select(title='Palette', options=PALETTES,value=self.palette,width=100)
		self.widgets.palette.on_change("value",lambda attr, old, new: self.setPalette(new))  
		self.color_mapper = LinearColorMapper() 
		self.color_mapper.palette=self.widgets.palette.value
		self.color_mapper.low,self.color_mapper.high=self.palette_range
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
		self.widgets.direction = Select(title='Direction', options=DIRECTIONS,value='2',width=100)
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
		self.play.callback=None
		self.widgets.play_button = Button(label="Play",width=80,sizing_mode='stretch_height')
		self.widgets.play_button.on_click(self.togglePlay)
		self.widgets.play_sec = Select(title="Play sec",options=["0.01","0.1","0.2","0.1","1","2"], value="0.01",width=120)
   
  
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
  
	# getDatasets
	def getDatasets(self):
		return self.widgets.dataset.options

	# getDatasets
	def setDatasets(self,value,title=None):
		self.widgets.dataset.options=value
		if title is not None: self.widgets.dataset.title=title
  
	# getLogicToPixel
	def getLogicToPixel(self):
		return self.logic_to_pixel

	# setLogicToPixel
	def setLogicToPixel(self,value):
		self.logic_to_pixel=value
		self.refresh()

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
		pass

	# addIdleCallback
	def addIdleCallback(self, callback, msec=10):
		return curdoc().add_periodic_callback(callback, msec)
  
	# removeIdleCallback
	def removeIdleCallback(self,callback):
		curdoc().remove_periodic_callback(callback)
  
	# ///////////////////////////////////////// PLAY

	# togglePlay
	def togglePlay(self,evt=None):
		if self.play.callback is not None:
			self.stopPlay()  
		else:
			self.startPlay()
			
	# startPlay
	def startPlay(self):
		self.play.callback=self.addIdleCallback(self.onPlayTimer)
		self.play.t1=time.time()
		self.play.wait_render_id=None
		self.play.num_refinements=self.getNumberOfRefinements()
		self.setNumberOfRefinements(1)
		self.setWidgetsDisabled(True)
		self.widgets.play_button.disabled=False
		self.widgets.play_button.label="Stop"
	
	# stopPlay
	def stopPlay(self):
		callback,self.play.callback=self.play.callback,None
		self.removeIdleCallback(callback)
		self.play.wait_render_id=None
		self.setNumberOfRefinements(self.play.num_refinements)
		self.setWidgetsDisabled(False)
		self.widgets.play_button.disabled=False
		self.widgets.play_button.label="Play"
  
	# stillRendering
	def stillRendering(self):
		for it in self.children:
			if it.stillRendering(): return True
		return False

	# onPlayTimer
	def onPlayTimer(self):

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
		
		# I will wait for the resolution to be displayed
		self.play.wait_render_id=[(it+1) if it is not None else None for it in render_id]
		self.play.t1=time.time()
		self.setTimestep(T) 

	# getPointDim
	def getPointDim(self):
		return self.db.getPointDim() if self.db else 2

	# gotoPoint
	def gotoPoint(self, p):
		for it in self.children:
			it.gotoPoint(p)

	# getNumberOfViews
	def getNumberOfViews(self):
		return int(self.widgets.num_views.value)

	# setNumberOfViews
	def setNumberOfViews(self,value):
		self.widgets.num_views.value=str(value)

	# getTimesteps
	def getTimesteps(self):
		return [int(value) for value in self.db.db.getTimesteps().asVector()]

	# getFields
	def getFields(self):
		return self.db.getFields()
  
	# setTimestep
	def setTimestep(self, value):
		self.widgets.timestep.value=value
		for it in self.children:
			it.setTimestep(value)  
		self.refresh()

	# getTimestep
	def getTimestep(self):	
		return int(self.widgets.timestep.value)

	# setTimestepDelta
	def setTimestepDelta(self,value):
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

	# getTimestepDelta
	def getTimestepDelta(self):
		return int(self.widgets.timestep_delta.value)

	# setField
	def setField(self,value):
		if value is None: return
		self.widgets.field.value=value
		for it in self.children:
			it.setField(value)  
		self.refresh()
  
	# getField
	def getField(self):
		return str(self.widgets.field.value)

	# setDirection
	def setDirection(self,dir):
		pdim=self.getPointDim()
		if pdim==2: dir=2
		dims=[int(it) for it in self.db.getLogicSize()]
		self.widgets.direction.value = str(dir)

		# 2d there is no direction 
		pdim=self.getPointDim()
		if pdim==2:
			assert dir==2
			self.widgets.offset.start, self.widgets.offset.end= 0,1-1
			self.widgets.offset.value = 0
		else:
			self.widgets.offset.start, self.widgets.offset.end = 0,int(dims[dir])-1
			self.widgets.offset.value = int(dims[dir]//2)
   
		self.refresh()

	# getDirection
	def getDirection(self):
		return int(self.widgets.direction.value)

	# setPalette
	def setPalette(self, value, palette_range=None):
	 
		logger.info(f"Slice::setPalette value={value} palette_range={palette_range}")
  
		if palette_range is None:
			palette_range=self.palette_range
  
		self.palette=value
		self.palette_range=palette_range
  
		self.widgets.palette.value=value

		if value.startswith("colorcet."):
			import colorcet
			value=getattr(colorcet,value[len("colorcet."):])   

		self.color_mapper.palette=value
		self.color_mapper.low, self.color_mapper.high=palette_range
  
		for I,it in enumerate(self.children):
			it.setPalette(value,palette_range=palette_range)  
  
		self.refresh()

	# getPalette
	def getPalette(self):
		return self.palette

	# getPaletteRange
	def getPaletteRange(self):
		return self.palette_range

	# setOffset (3d only)
	def setOffset(self,value):
		self.widgets.offset.value=value
		self.refresh()

	# getOffset
	def getOffset(self):
		return self.widgets.offset.value

	# setNumberOfRefinements
	def setNumberOfRefinements(self,value):
		self.widgets.num_refinements.value=value
		for it in self.children:
			it.setNumberOfRefinements(value)      
		self.refresh()

	# getNumberOfRefinements
	def getNumberOfRefinements(self):
		return self.widgets.num_refinements.value

	# setQuality
	def setQuality(self,value):
		self.widgets.quality.value=value
		for it in self.children:
			it.setQuality(value) 
		self.refresh()

	# getQuality
	def getQuality(self):
		return self.widgets.quality.value

	# setFields
	def setFields(self, options):
		self.widgets.field.options =list(options)

	# getFields
	def getFields(self):
		return self.widgets.field.options 

	# setTimesteps
	def setTimesteps(self,timesteps):
		self.widgets.timestep.start =  timesteps[0]
		self.widgets.timestep.end   =  timesteps[-1]
		self.widgets.timestep.step  = 1

	# getViewDepedent
	def getViewDepedent(self):
		return cbool(self.widgets.viewdep.value)

	# setViewDependent
	def setViewDependent(self,value):
		self.widgets.viewdep.value=str(int(value))
		for it in self.children:
			it.setViewDependent(value)     
		self.refresh()