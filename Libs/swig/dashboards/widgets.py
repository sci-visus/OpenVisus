import os,sys,logging,types,time

from .utils import cbool
from .config import DIRECTIONS, PALETTES

from bokeh.models import Select,LinearColorMapper,ColorBar,Button,Slider,TextInput,Row,Column
from bokeh.io import curdoc

logger = logging.getLogger(__name__)

# //////////////////////////////////////////////////////////////////////////////////////
class Widgets:

	# constructor
	def __init__(self,doc=None):
   
		self.doc=doc
		self.db=None
		self.children=[]
  
		self.palette='Greys256'
		self.palette_range=(0,255) 

		self.widgets=types.SimpleNamespace()

		# palette
		self.widgets.palette = Select(title='Palette',  options=PALETTES,value=self.palette,width=120,sizing_mode='stretch_height')
		self.widgets.palette.on_change("value",lambda attr, old, new: self.setPalette(new))  
 
		 # color mapper
		self.color_mapper = LinearColorMapper() # LogColorMapper
		self.color_mapper.palette=self.widgets.palette.value
		self.color_mapper.low,self.color_mapper.high=self.palette_range

		# colorbar
		self.color_bar = ColorBar(color_mapper=self.color_mapper)
  
		self.widgets.num_views=Select(title='#Views',  options=["1","2","3","4"],value='3',width=50,sizing_mode='stretch_height')
		self.widgets.num_views.on_change("value",lambda attr, old, new: self.setNumberOfViews(int(new))) 
 
		# timestep
		self.widgets.timestep = Slider(title='Time', value=0, start=0, end=1, sizing_mode='stretch_width')
		self.widgets.timestep.on_change ("value",lambda attr, old, new: self.setTimestep(int(new)))  

		# timestep delta
		self.widgets.timestep_delta=Select(title="Time delta",options=["1","2","5","10","50","100","200"], value="1", width=120)
		self.widgets.timestep_delta.on_change("value", lambda attr, old, new: self.setTimestepDelta(int(new)))   

		# play time
		self.play=types.SimpleNamespace()
		self.play.callback=None
		self.play.button = Button(label="Play",sizing_mode='stretch_height',width=80)
		self.play.button.on_click(self.startOrStopPlay)
		self.play.sec = Select(title="Play sec",options=["0.01","0.1","0.2","0.1","1","2"], value="0.01",width=120)
  
		# field
		self.widgets.field = Select(title='Field',  options=[],value='data',width=120)
		self.widgets.field.on_change("value",lambda attr, old, new: self.setField(new))  
  
		# direction 
		self.widgets.direction = Select(title='Direction', options=DIRECTIONS,value='2',width=80)
		self.widgets.direction.on_change ("value",lambda attr, old, new: self.setDirection(int(new)))  
  
		# offset 
		self.widgets.offset = Slider(title='Offset', value=0, start=0, end=1024, sizing_mode='stretch_width')
		self.widgets.offset.on_change ("value",lambda attr, old, new: self.setOffset(int(new)))
  
		# num_refimements (0==guess)
		self.widgets.num_refinements=Slider(title='#Refinements', value=0, start=0, end=4, sizing_mode='stretch_width')
		self.widgets.num_refinements.on_change ("value",lambda attr, old, new: self.setNumberOfRefinements(int(new)))
  
		# quality (0==full quality, -1==decreased quality by half-pixels, +1==increase quality by doubling pixels etc)
		self.widgets.quality = Slider(title='Quality', value=0, start=-12, end=+12, width=120)
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
  
  
	# createGui
	def createGui(self,central_layout=None,options=[]):
		ret=Column(children=[],sizing_mode=self.sizing_mode)

		v=[]
  
		if "num_views" in options:
			v.append(self.widgets.num_views) 
  
		if "palette" in options:  
			v.append(self.widgets.palette)

		if "timestep" in options:  
			v.append(self.widgets.timestep)

		if "timestep_delta" in options:
			v.append(self.widgets.timestep_delta)

		if "direction" in options:
			v.append(self.widgets.direction)

		if "offset" in options:
			v.append(self.widgets.offset)

		if "field" in options:
			v.append(self.widgets.field)

		if "viewdep" in options:
			v.append(self.widgets.viewdep)

		if "quality" in options:
			v.append(self.widgets.quality)     
			
		if "num_refinements" in options:
			v.append(self.widgets.num_refinements)
	   
		if "direction" in options:
			v.append(self.widgets.direction)
   
		if "offset" in options:
			v.append(self.widgets.offset)
   
		if "play-button" in options:
				v.append(self.play.button)
				
		if "play-msec" in options:
			v.append(self.play.sec)  

		ret.children.append(
			Row(*v, sizing_mode='stretch_width')
		)  
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
		for widget in dir(self.widgets):
			widget.disabled=value
 		
		for it in self.children:
			it.setWidgetsDisabled(value)  

	# refresh (to override if needed)
	def refresh(self):
		pass

	# addIdleCallback
	def addIdleCallback(self, callback, msec=10):
		doc=self.doc if self.doc else curdoc()
		doc.add_periodic_callback(callback, msec)
  
	# removeIdleCallback
	def removeIdleCallback(self,callback):
		doc=self.doc if self.doc else curdoc()
		doc.remove_periodic_callback(callback)

	# startOrStopRunning
	def startOrStopPlay(self,evt=None):
		if not self.play.callback:
			self.startPlay()
		else:
			self.stopPlay()

	# startPlay
	def startPlay(self):
		assert(self.play.callback is None)
		self.play.t1=time.time()
		self.play.render_id=None
		self.play.num_refinements=self.getNumberOfRefinements()
		self.setNumberOfRefinements(1)
		self.setWidgetsDisabled(True)
		self.play.callback=self.addIdleCallback(self.onPlayTimer)
		self.play.button.label="Stop"
	
	# stopPlay
	def stopPlay(self):
		assert(self.play.callback is not None)
		self.setNumberOfRefinements(self.play.num_refinements)
		self.setWidgetsDisabled(False)
		self.removeIdleCallback(self.play.callback)
		self.play.callback=None
		self.play.button.label="Play"

	# onPlayTimer
	def onPlayTimer(self):

		# avoid playing too fast by waiting a minimum amount of time
		t2=time.time()
		if (t2-self.play.t1)<float(self.play.sec.value):
			return   
			
		if self.play.render_id is not None and any([a<b for a,b in zip([slice.render_id for slice in self.slices],self.play.render_id)]):
			# logger.info("fWaiting render {self.render_id} {render_id}")
			return

		# advance
		D=int(self.widgets.timestep_delta.value)
		T=self.getTimestep()+D

		# reached the end -> go to the beginning
		if T>=self.widgets.timestep.end: 
			T=self.timesteps.widgets.timestep.start
		
		# I will wait for the resolution to be displayed
		self.play.render_id=[slice.render_id+1 for slice in self.slices]
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