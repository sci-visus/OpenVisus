import os,sys,io,types,threading,time,logging
import numpy as np

from .backend import Aborted
from .canvas import Canvas
from .widgets import Widgets
from .querynode import QueryNode

from bokeh.models import Column,Row

logger = logging.getLogger(__name__)

# ////////////////////////////////////////////////////////////////////////////////////
class Slice(Widgets):
	
	# constructor
	def __init__(self,
			show_options=["palette","timestep","field","direction","offset","viewdep","quality","!num_refinements","status_bar"]):

		super().__init__()
		self.render_id     = 0
		self.aborted       = Aborted()
		self.new_job       = False
		self.current_img   = None
		self.options={}
		self.canvas = Canvas(self.color_bar, self.color_mapper, sizing_mode='stretch_both')
		self.last_logic_box = None
		self.last_canvas_size = [0,0]
		self.layout=self.createGui(central_layout=self.canvas.figure, options=show_options)
		self.query=QueryNode()
		self.query.startThread()
		
	# setDataset
	def setDataset(self, db, compatible=False):

		self.db=db
		self.access=self.db.createAccess()

		timesteps=db.getTimesteps()
		fields=db.getFields()

		timestep                   = self.getTimestep() if compatible else timesteps[0]
		timestep_delta             = self.getTimestepDelta() if compatible else 1
		field                      = self.getField() if compatible else fields[0]
		direction                  = self.getDirection() if compatible else 2
		palette, palette_range     = self.getPalette(),self.getPaletteRange()
		viewdep                    = self.getViewDepedent()

		self.setTimesteps(timesteps)
		self.setTimestepDelta(timestep_delta)
		self.setDirection(direction)
		self.setTimestep(timestep)
		self.setFields(fields)
		self.setField(field)
		self.setPalette(palette,palette_range) 
		self.setViewDependent(viewdep)
		self.last_canvas_size=[0,0] if not compatible else self.last_canvas_size 


		self.refresh()

	# refresh
	def refresh(self):
		self.aborted.setTrue()
		self.new_job=True
   
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
			ret[dir]=self.getOffset()

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
  
	# renderResultIfNeeded
	def renderResultIfNeeded(self):
		result=self.query.popResult(last_only=True) 
		if result is None: return
		data=result['data']
		query_box=result['logic_box']
		(x1,y1),(x2,y2)=self.project(query_box)
		self.canvas.setImage(data,x1,y1,x2,y2)
		tot_pixels=data.shape[0]*data.shape[1]
		canvas_pixels=self.canvas.getWidth()*self.canvas.getHeight()
		MaxH=self.db.getMaxResolution()
		self.widgets.status_bar["response"].value=f"{result['I']}/{result['N']} {str(query_box).replace(' ','')} {data.shape[0]}x{data.shape[1]} H={result['H']}/{MaxH} {result['msec']}msec"
		self.render_id+=1     
  
	# pushJobIfNeeded
	def pushJobIfNeeded(self):
 
		logic_box=self.getLogicBox()
		pdim=self.getPointDim()
		if not self.new_job and str(self.last_logic_box)==str(logic_box):
			return

		# abort the last one
		self.aborted.setTrue()
		self.query.waitIdle()
		num_refinements = self.getNumberOfRefinements()
		if num_refinements==0:
			num_refinements=3 if pdim==2 else 4
		self.aborted=Aborted()

		quality=self.getQuality()

		if self.getViewDepedent():
			canvas_w,canvas_h=(self.canvas.getWidth(),self.canvas.getHeight())
			endh=None
			max_pixels=canvas_w*canvas_h
			if quality<0:
				max_pixels=int(max_pixels/pow(1.3,abs(quality))) # decrease the quality
			elif quality>0:
				max_pixels=int(max_pixels*pow(1.3,abs(quality))) # increase the quality
		else:
			max_pixels=None
			endh=self.db.getMaxResolution()+quality
		
		timestep=self.getTimestep()
		field=self.getField()
		box_i=[[int(it) for it in jt] for jt in logic_box]
		self.widgets.status_bar["request"].value=f"t={timestep} b={str(box_i).replace(' ','')} {canvas_w}x{canvas_h}"

		self.query.pushJob({
			"db": self.db, 
			"access": self.access,
			"timestep":timestep, 
			"field":field, 
			"logic_box":logic_box, 
			"max_pixels":max_pixels, 
			"num_refinements":num_refinements, 
			"endh":endh, 
			"aborted":self.aborted
		})
		self.last_logic_box=logic_box
		self.new_job=False    
  
  	# onCanvasResize
	def watchForCanvasResize(self):
		canvas_w,canvas_h=(self.canvas.getWidth(),self.canvas.getHeight())
		if canvas_w>0 and canvas_h>0 and self.last_canvas_size[0]<=0 and self.last_canvas_size[0]<=0:
			self.setDirection(self.getDirection())
			self.last_canvas_size=[canvas_w,canvas_h]
			self.refresh()
  
	# onIdle
	def onIdle(self):
     
		super().onIdle()
     
		# ready for jobs?
		canvas_w,canvas_h=(self.canvas.getWidth(),self.canvas.getHeight())
		if canvas_w==0 or canvas_h==0 or not self.db:
			return

		self.watchForCanvasResize()
		self.renderResultIfNeeded()
		self.pushJobIfNeeded()
		