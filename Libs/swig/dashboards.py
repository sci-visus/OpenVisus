import sys,io,types,threading,time
import os,sys
import numpy as np

from datetime import datetime

import bokeh.io
import bokeh.events
import bokeh.models
import bokeh.plotting 

import OpenVisus as ov
from OpenVisus.pyquery import PyQuery
from OpenVisus.image_utils import SplitChannels, InterleaveChannels

import logging
logger = logging.getLogger(__name__)

if ov.cbool(os.environ.get("VISUS_DASHBOARDS_VERBOSE","0")) == True:
	ov.SetupLogger(logger)

DIRECTIONS=[('0','X'),('1','Y'),('2','Z')]

# ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
def GetPalettes():
	import colorcet 
	return [
	"Greys256", "Inferno256", "Magma256", "Plasma256", "Viridis256", "Cividis256", "Turbo256"
	] + [
		it  for it in [
		'colorcet.blueternary', 
		'colorcet.coolwarm', 
		'colorcet.cyclicgrey', 
		'colorcet.depth', 
		'colorcet.divbjy', 
		'colorcet.fire', 
		'colorcet.geographic', 
		'colorcet.geographic2', 
		'colorcet.gouldian', 
		'colorcet.gray', 
		'colorcet.greenternary', 
		'colorcet.grey', 
		'colorcet.heat', 
		'colorcet.phase2', 
		'colorcet.phase4', 
		'colorcet.rainbow', 
		'colorcet.rainbow2', 
		'colorcet.rainbow3', 
		'colorcet.rainbow4', 
		'colorcet.redternary', 
		'colorcet.reducedgrey', 
		'colorcet.yellowheat']
		if hasattr(colorcet,it[9:])
	]  

# ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class DiscreteSlider():
	
	def __init__(self, name="",options=None, value=None, on_change=None,sizing_mode='stretch_width'):
		self.options=options
		self.sizing_mode=sizing_mode
		self.name=bokeh.models.Div(text=name)
		self.slider=bokeh.models.Slider(title='', start=0,end=len(options)-1, step=1,value=options.index(value), sizing_mode=sizing_mode,show_value=False)
		self.value=bokeh.models.Div(text="")
		self.on_change=on_change
		self.slider.on_change("value",lambda attr, old, new: self.onSliderChange(self.options[new]))
		self.layout=bokeh.models.Column(self.slider, bokeh.models.Row(self.name, self.value, sizing_mode=sizing_mode), sizing_mode=sizing_mode)
		self.refreshText()

	def refreshText(self):
		value=self.getValue()
		self.value.text=f"{value}"

	def getValue(self):
		idx=self.slider.value
		return self.options[idx]

	def setValue(self,value):
		idx=self.options.index(value)
		self.slider.value=idx

	def onSliderChange(self,value):
		self.refreshText()
		if self.on_change: self.on_change(value)

# ////////////////////////////////////////////////////////////////////////////////////
class Canvas:
  
	# constructor
	def __init__(self, color_bar, color_mapper,sizing_mode='stretch_both'):
		self.sizing_mode=sizing_mode
		self.color_bar=color_bar
		self.color_mapper=color_mapper
		self.figure=bokeh.plotting.figure(active_scroll = "wheel_zoom") 
		self.figure.x_range = bokeh.models.Range1d(0,0)   
		self.figure.y_range = bokeh.models.Range1d(512,512) 
		self.figure.toolbar_location="below"
		self.figure.sizing_mode = self.sizing_mode
		# self.figure.add_tools(bokeh.models.HoverTool(tooltips=[ ("(x, y)", "($x, $y)"),("RGB", "(@R, @G, @B)")])) # is it working?
  
		self.source_image = bokeh.models.ColumnDataSource(data={"image": [np.random.random((300,300))*255], "x":[0], "y":[0], "dw":[256], "dh":[256]})  
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

	# __getImage
	def __getImage(self, data, normalize_float=True):
		height,width=data.shape[0],data.shape[1]

		# typycal case
		if data.dtype==np.uint8:

			# (height,width)::uint8... grayscale, I will apply the colormap
			if len(data.shape)==2:
				Gray=data
				return Gray 
	
			# (height,depth,channel)
			if len(data.shape)!=3:
				raise Exception(f"Wrong dtype={data.dtype} shape={data.shape}")

			channels=SplitChannels(data)

			if len(channels)==1:
				Gray=channels[0]
				return Gray

			if len(channels)==2:
				G,A=channels
				return  InterleaveChannels([G,G,G,A]).view(dtype=np.uint32).reshape([height,width]) 
	 
			elif len(channels)==3:
				R,G,B=channels
				A=np.full(channels[0].shape, 255, np.uint8)
				return  InterleaveChannels([R,G,B,A]).view(dtype=np.uint32).reshape([height,width]) 

			elif len(channels)==4:
				R,G,B,A=channels
				return InterleaveChannels([R,G,B,A]).view(dtype=np.uint32).reshape([height,width]) 
			
		else:

			# (height,depth) ... I will apply matplotlib colormap 
			if len(data.shape)==2:
				G=data.astype(np.float32)
				return G
			
			# (height,depth,channel)
			if len(data.shape)!=3:
				raise Exception(f"Wrong dtype={data.dtype} shape={data.shape}")  
		
			# convert all channels in float32
			channels=SplitChannels(data)
			channels=[channel.astype(np.float32) for channel in channels]

			if normalize_float:
				for C,channel in enumerate(channels):
					m,M=np.min(channel),np.max(channel)
					channels[C]=(channel-m)/(M-m)

			if len(channels)==1:
				G=channels[0]
				return G

			if len(channels)==2:
				G,A=channels
				return InterleaveChannels([G,G,G,A])
	 
			elif len(channels)==3:
				R,G,B=channels
				A=np.full(channels[0].shape, 1.0, np.float32)
				return InterleaveChannels([R,G,B,A])

			elif len(channels)==4:
				R,G,B,A=channels
				return InterleaveChannels([R,G,B,A])
		
		raise Exception(f"Wrong dtype={data.dtype} shape={data.shape}") 


	# setImage
	def setImage(self, data, x1, y1, x2, y2):
		img=self.__getImage(data)
		dtype=img.dtype
 
		if self.dtype==dtype :
			# current dtype is 'compatible' with the new image dtype, just change the source _data
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

# //////////////////////////////////////////////////////////////////////////////////////
class Widgets:

	# constructor
	def __init__(self):
   
		self.db=None
  
		self.palette='Greys256'
		self.palette_range=(0,255) 

		self.widgets=types.SimpleNamespace()

		# palette
		self.widgets.palette = bokeh.models.Select(title='Palette',  options=GetPalettes(),value=self.palette,width=120,sizing_mode='stretch_height')
		self.widgets.palette.on_change("value",lambda attr, old, new: self.setPalette(new))  
 
		 # color mapper
		self.color_mapper = bokeh.models.LinearColorMapper() # LogColorMapper
		self.color_mapper.palette=self.widgets.palette.value
  
		self.color_mapper.low,self.color_mapper.high=self.palette_range

		# colorbar
		self.color_bar = bokeh.models.ColorBar(color_mapper=self.color_mapper)
 
		# timestep
		self.widgets.timestep = bokeh.models.Slider(title='Time', value=0, start=0, end=1, sizing_mode='stretch_width')
		self.widgets.timestep.on_change ("value",lambda attr, old, new: self.setTimestep(int(new)))  

		# timestep delta
		self.widgets.timestep_delta=bokeh.models.Select(title="Time delta",options=["1","2","5","10","50","100","200"], value="1", width=120)
		self.widgets.timestep_delta.on_change("value", lambda attr, old, new: self.setTimestepDelta(int(new)))   

		# play time
		self.play=types.SimpleNamespace()
		self.play.callback=None
		self.play.button = bokeh.models.Button(label="Play",sizing_mode='stretch_height',width=80)
		self.play.button.on_click(self.startOrStopPlay)
		self.play.sec = bokeh.models.Select(title="Play sec",options=["0.01","0.1","0.2","0.1","1","2"], value="0.01",width=120)
  
		# field
		self.widgets.field = bokeh.models.Select(title='Field',  options=[],value='data',width=120)
		self.widgets.field.on_change("value",lambda attr, old, new: self.setField(new))  
  
		# direction 
		self.widgets.direction = bokeh.models.Select(title='Direction', options=DIRECTIONS,value='2',width=80)
		self.widgets.direction.on_change ("value",lambda attr, old, new: self.setDirection(int(new)))  
  
		# offset 
		self.widgets.offset = bokeh.models.Slider(title='Offset', value=0, start=0, end=1024, sizing_mode='stretch_width')
		self.widgets.offset.on_change ("value",lambda attr, old, new: self.setOffset(int(new)))
  
		# num_refimements (0==guess)
		self.widgets.num_refinements=bokeh.models.Slider(title='#Refinements', value=0, start=0, end=4, sizing_mode='stretch_width')
		self.widgets.num_refinements.on_change ("value",lambda attr, old, new: self.setNumberOfRefinements(int(new)))
  
		# quality (0==full quality, -1==decreased quality by half-pixels, +1==increase quality by doubling pixels etc)
		self.widgets.quality = bokeh.models.Slider(title='Quality', value=0, start=-12, end=+12, width=120)
		self.widgets.quality.on_change("value",lambda attr, old, new: self.setQuality(int(new)))  
  
		# status_bar
		self.widgets.status_bar= {}
		self.widgets.status_bar["request" ]=bokeh.models.TextInput(title="" ,sizing_mode='stretch_width')
		self.widgets.status_bar["response"]=bokeh.models.TextInput(title="" ,sizing_mode='stretch_width')
		self.widgets.status_bar["request" ].disabled=True
		self.widgets.status_bar["response"].disabled=True

	# setWidgetsDisabled
	def setWidgetsDisabled(self,value):
		self.widgets.palette.disabled=value
		self.widgets.timestep.disabled=value
		self.widgets.field.disabled=value
		self.widgets.direction.disabled=value
		self.widgets.offset.disabled=value
		self.widgets.num_refinements.disabled=value

	# refresh (to override if needed)
	def refresh(self):
		pass

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
		doc=self.doc if self.doc else bokeh.io.curdoc();assert(doc)
		self.play.callback=doc.add_periodic_callback(self.onPlayTimer,10)
		self.play.button.label="Stop"
	
	# stopPlay
	def stopPlay(self):
		assert(self.play.callback is not None)
		self.setNumberOfRefinements(self.play.num_refinements)
		self.setWidgetsDisabled(False)
		doc=self.doc if self.doc else bokeh.io.curdoc();assert(doc)
		doc.remove_periodic_callback(self.play.callback)
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

	# getTimesteps
	def getTimesteps(self):
		return [int(value) for value in self.db.db.getTimesteps().asVector()]

	# getFields
	def getFields(self):
		return self.db.getFields()
  
	# setTimestep
	def setTimestep(self, value):
		self.widgets.timestep.value=value
		self.refresh()

	# getTimestep
	def getTimestep(self):	
		return int(self.widgets.timestep.value)

	# setTimestepDelta
	def setTimestepDelta(self,D):
		self.widgets.timestep_delta.value=str(D)
		self.widgets.timestep.step=D
		A=self.widgets.timestep.start
		B=self.widgets.timestep.end
		T=self.getTimestep()
		T=A+D*int((T-A)/D)
		T=min(B,max(A,T))
		self.setTimestep(T)
		self.refresh()

	# getTimestepDelta
	def getTimestepDelta(self):
		return int(self.widgets.timestep_delta.value)

	# setField
	def setField(self,value):
		self.widgets.field.value=value
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
			self.widgets.offset.start, self.widgets.offset.end= 0,1
			self.widgets.offset.value = 0
		else:
			self.widgets.offset.start, self.widgets.offset.end = 0,int(dims[dir])
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
		self.refresh()

	# getNumberOfRefinements
	def getNumberOfRefinements(self):
		return self.widgets.num_refinements.value

	# setQuality
	def setQuality(self,value):
		self.widgets.quality.value=value
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

# ////////////////////////////////////////////////////////////////////////////////////
class Slice(Widgets):
	
	# constructor
	def __init__(self,doc=None, sizing_mode='stretch_both',timer_msec=100,logic_to_pixel=[(0.0,1.0)]*3, 
			show_options=["palette","timestep","field","direction","offset","quality","!num_refinements","status_bar"]):
		super().__init__()
		self.logic_to_pixel=logic_to_pixel
		self.sizing_mode=sizing_mode
		self.access=None
		self.lock          = threading.Lock()
		self.aborted       = ov.Aborted()
		self.new_job       = False
		self.current_img   = None
		self.options={}
		self.render_id = 0
		self.canvas = Canvas(self.color_bar, self.color_mapper, sizing_mode=self.sizing_mode)
		self.last_logic_box = None
		self.last_canvas_size = [0,0]

		self.createGui(show_options)
	
		if doc is None: doc=bokeh.io.curdoc()
		self.callback = doc.add_periodic_callback(self.onTimer, 10) 
   
		self.query=PyQuery()
		self.query.startThread()
		
	# createGui
	def createGui(self,options):
		self.layout=bokeh.layouts.column(children=[],sizing_mode=self.sizing_mode)

		v=[]
  
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

		if "quality" in options:
			v.append(self.widgets.quality)     
			
		if "num_refinements" in options:
			v.append(self.widgets.num_refinements)
       
		if self.getPointDim()==3 and "direction" in options:
			v.append(self.widgets.direction)
   
		if self.getPointDim()==3 and "offset" in options:
			v.append(self.widgets.offset)

		self.layout.children.append(
			bokeh.layouts.Row(*v, sizing_mode='stretch_width')
		)  

		self.layout.children.append(self.canvas.figure)
		
		if "status_bar" in options:
			self.layout.children.append(bokeh.layouts.Row(
				self.widgets.status_bar["request"],
				self.widgets.status_bar["response"], 
				sizing_mode='stretch_width'))
  
	# setDataset
	def setDataset(self, db, compatible=False):

		self.db=db
		self.access=self.db.createAccessForBlockQuery()

		timesteps=db.getTimesteps()
		fields=db.getFields()

		timestep                   = self.getTimestep() if compatible else timesteps[0]
		timestep_delta             = self.getTimestepDelta() if compatible else 1
		field                      = self.getField() if compatible else fields[0]
		direction                  = self.getDirection() if compatible else 2
		palette, palette_range     = self.getPalette(),self.getPaletteRange()

		self.setTimesteps(timesteps)
		self.setTimestepDelta(timestep_delta)
		self.setDirection(direction)
		self.setTimestep(timestep)
		self.setFields(fields)
		self.setField(field)
		self.setPalette(palette,palette_range) 
		self.last_canvas_size=[0,0] if not compatible else self.last_canvas_size 

		self.refresh()

	# refresh
	def refresh(self):
		self.aborted.setTrue()
		self.new_job=True

	# enableDoubleTap
	def enableDoubleTap(self,fn):
		self.canvas.figure.on_event(bokeh.events.DoubleTap, lambda evt: fn(self.unproject((evt.x,evt.y))))
   
	# project
	def project(self,value):

		pdim=self.getPointDim()
		dir=self.getDirection()
		assert(pdim,len(value))

		# is a box
		if hasattr(value[0],"__iter__"):
			p1,p2=[self.project(p) for p in value]
			return [p1,p2]

		# apply scaling and translating
		ret=[self.logic_to_pixel[I][0] + self.logic_to_pixel[I][1]*value[I] for I in range(pdim)]

		if pdim==3:
			del ret[dir]

		assert(len(ret)==2)
		return ret

	# unproject
	def unproject(self,value):

		assert(len(value)==2)

		pdim=self.getPointDim() 
		dir=self.getDirection()

		# is a box?
		if hasattr(value[0],"__iter__"):
			p1,p2=[self.unproject(p) for p in value]
			if pdim==3: 
				p2[dir]+=1 # make full dimensional
			return [p1,p2]

		ret=list(value)

		# reinsert removed coordinate
		if pdim==3:
			ret.insert(dir, 0)

		assert(len(ret)==pdim)

		# scaling/translatation
		ret=[(ret[I]-self.logic_to_pixel[I][0])/self.logic_to_pixel[I][1] for I in range(pdim)]

		
		# this is the right value in logic domain
		if pdim==3:
			ret[dir]=self.widgets.offset.value

		assert(len(ret)==pdim)
		return ret
  
	# getLogicBox
	def getLogicBox(self):
		x1,y1,x2,y2=self.canvas.getViewport()
		return self.unproject(((x1,y1),(x2,y2)))

	# setLogicBox (NOTE: it ignores the coordinates on the direction)
	def setLogicBox(self,value):
		proj=self.project(value)
		self.canvas.setViewport(*(proj[0] + proj[1]))
		self.refresh()
  
	# getLogicCenter
	def getLogicCenter(self):
		pdim=self.getPointDim()  
		p1,p2=self.getLogicBox()
		assert(len(p1)==pdim and len(p2)==pdim)
		return [(p1[I]+p2[I])*0.5 for I in range(pdim)]

	# getLogicSize
	def getLogicSize(self):
		pdim=self.getPointDim()
		p1,p2=self.getLogicBox()
		assert(len(p1)==pdim and len(p2)==pdim)
		return [(p2[I]-p1[I]) for I in range(pdim)]

	# setAccess
	def setAccess(self, value):
		self.access=value
		self.refresh()

	# setDirection
	def setDirection(self,dir):
		super().setDirection(dir)
		dims=[int(it) for it in self.db.getLogicSize()]
		self.setLogicBox(([0]*self.getPointDim(),dims))
		self.refresh()
  
  # gotoPoint
	def gotoPoint(self,point):
		pdim=self.getPointDim()
		# go to the slice
		if pdim==3:
			dir=self.getDirection()
			self.setOffset(point[dir])
		# the point should be centered in p3d
		(p1,p2),dims=self.getLogicBox(),self.getLogicSize()
		p1,p2=list(p1),list(p2)
		for I in range(pdim):
			p1[I],p2[I]=point[I]-dims[I]/2,point[I]+dims[I]/2
		self.setLogicBox([p1,p2])
		self.canvas.renderPoints([self.project(point)])
  
	# renderImage
	def renderImage(self,result):
		data=result['data']
		query_box=result['logic_box']
		(x1,y1),(x2,y2)=self.project(query_box)
		self.canvas.setImage(data,x1,y1,x2,y2)
		tot_pixels=data.shape[0]*data.shape[1]
		canvas_pixels=self.canvas.getWidth()*self.canvas.getHeight()
		MaxH=self.db.getMaxResolution()
		self.widgets.status_bar["response"].value=f"{result['I']}/{result['N']} {str(query_box).replace(' ','')} {data.shape[0]}x{data.shape[1]} H={result['H']}/{MaxH} {result['msec']}msec"
		self.render_id+=1     


	# onTimer
	def onTimer(self):
		# ready for jobs?
		canvas_w,canvas_h=(self.canvas.getWidth(),self.canvas.getHeight())
		if canvas_w==0 or canvas_h==0 or not self.db:
			return

		# simulate fixAspectRatio (i cannot find a bokeh metod to watch for resize event)
		if canvas_w>0 and canvas_h>0 and self.last_canvas_size[0]<=0 and self.last_canvas_size[0]<=0:
			self.setDirection(self.getDirection())
			self.last_canvas_size=[canvas_w,canvas_h]
			self.refresh()
   
		# a new image is available?
		result=self.query.popResult(last_only=True) 
		if result is not None:
			self.renderImage(result)

		# note: the canvas does not comminicate viewport changes, so I need to keep track the last viewport/logic box I queries
		logic_box=self.getLogicBox()
		pdim=self.getPointDim()
		if self.new_job or str(self.last_logic_box)!=str(logic_box):

			# abort the last one
			self.aborted.setTrue()
			self.query.waitIdle()
			num_refinements = self.getNumberOfRefinements()
			if num_refinements==0:
				num_refinements=3 if pdim==2 else 4
			self.aborted=ov.Aborted()
			
			# compute max_pixels
			max_pixels=canvas_w*canvas_h
			quality=self.getQuality()
			if quality==0:
				pass
			elif quality<0:
				max_pixels=int(max_pixels/pow(1.3,abs(quality))) # decrease the quality
			else:
				max_pixels=int(max_pixels*pow(1.3,abs(quality))) # increase the quality
				
			timestep=self.getTimestep()
			field=self.getField()
			box_i=[[int(it) for it in jt] for jt in logic_box]
			self.widgets.status_bar["request"].value=f"t={timestep} b={str(box_i).replace(' ','')} {canvas_w}x{canvas_h}"
			self.query.pushJob(self.db, self.access, timestep, field, logic_box, max_pixels, num_refinements,self.aborted)
			self.last_logic_box=logic_box
			self.new_job=False
		


# //////////////////////////////////////////////////////////////////////////////////////
class Slices(Widgets):

	# constructor
	def __init__(self, doc=None,panel_state=None,sizing_mode='stretch_both', show_options=[
			  "num_views","palette","timestep","field","quality","!num_refinements",
			"!direction","!offset" # this will be show on the single SLice
		 ]):
		super().__init__()
		self.doc=doc 
		self.logic_to_pixel=[(0.0,1.0)] * 3 # translation/scaling for each dimensions
		self.panel_state=panel_state 
		self.sizing_mode=sizing_mode
		self.slices=[]
		self.widgets.num_views=bokeh.models.Select(title='#Views',  options=["1","2","3","4"],value='3',width=50,sizing_mode='stretch_height')
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
   
		if "quality" in options:
			v.append(self.widgets.quality) 
  
		if "num_refinements" in options:
			v.append(self.widgets.num_refinements)  

		if "play-button" in options:
			v.append(self.play.button)
			
		if "play-msec" in options:
			v.append(self.play.sec)

		self.layout=bokeh.layouts.column(
			bokeh.layouts.Row(*v, sizing_mode='stretch_width'),
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

		self.setTimestepDelta(timestep_delta) 
		self.setTimesteps(timesteps)
		self.setTimestep(timestep)
		self.setFields(fields)
		self.setField(field)
		self.setPalette(palette,palette_range) 

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
			slice=Slice(self.doc,sizing_mode=self.sizing_mode,logic_to_pixel=self.logic_to_pixel,show_options=self.slice_show_options)
			slice.enableDoubleTap(self.gotoPoint)
			slice.setDataset(self.db)
			slice.setDirection(direction % 3)
			self.slices.append(slice)

		if value==1:
			self.slices=[self.slices[0]]
			self.layout.children.append(self.slices[0].layout)

		elif value==2:
			self.layout.children.append(bokeh.layouts.Row(
				   self.slices[0].layout,
				  self.slices[1].layout, 
				sizing_mode=self.sizing_mode) )

		elif value==3:
			self.layout.children.append(bokeh.layouts.Row(
				   self.slices[2].layout, 
				  bokeh.layouts.Column(
					self.slices[0].layout,
					 self.slices[1].layout,
					  sizing_mode=self.sizing_mode),
				sizing_mode=self.sizing_mode))
   
		elif value==4:
			self.layout.children.append(bokeh.layouts.grid(children=[
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

