import os,sys


import PyQt5
from   PyQt5.QtCore	import *
from   PyQt5.QtWidgets import *
from   PyQt5.QtGui	 import *
import PyQt5.sip  as  sip

from OpenVisus	     import *
from VisusGuiPy	     import *
from VisusGuiNodesPy import *
from VisusAppKitPy   import *

from OpenVisus.PyUtils import *



# ////////////////////////////////////////////////////////////////
class PyViewer(Viewer):
	
	# constructor
	def __init__(self):	
		super(PyViewer, self).__init__()
		self.setMinimal()
		self.addGLCameraNode("lookat")	
		
	# run
	def run(self):
		bounds=self.getWorldBounds()
		self.getGLCamera().guessPosition(bounds)	
		GuiModule.execApplication()		
		
	# addVolumeRender
	def addVolumeRender(self, data, bounds):
		
		t1=Time.now()
		print("Adding Volume render...")	
		
		Assert(isinstance(data,numpy.ndarray))
		data=Array.fromNumPy(data,TargetDim=3,bounds=bounds)
		
		node=RenderArrayNode()
		node.setLightingEnabled(False)
		node.setPaletteEnabled(False)
		node.setData(data)
		self.addNode(self.getRoot(), node)
		
		print("done in ",t1.elapsedMsec(),"msec")
		
		
	# addIsoSurface
	def addIsoSurface(self, field=None, second_field=None, isovalue=0, bounds=None):

		Assert(isinstance(field,numpy.ndarray))
		
		field=Array.fromNumPy(field,TargetDim=3,bounds=bounds)

		print("Extracting isocontour...")
		t1=Time.now()
		isocontour=MarchingCube(field,isovalue).run()
		print("done in ",t1.elapsedMsec(),"msec")
		
		if second_field is not None:
			isocontour.second_field=Array.fromNumPy(second_field,TargetDim=3,bounds=bounds)
					
		node=IsoContourRenderNode()	
		node.setIsoContour(isocontour)
		self.addNode(self.getRoot(),node)
		
		print("Added IsoContourRenderNode")	