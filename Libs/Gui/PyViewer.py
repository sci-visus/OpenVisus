import os,sys

import PyQt5
from   PyQt5.QtCore	import *
from   PyQt5.QtWidgets import *
from   PyQt5.QtGui	 import *
import PyQt5.sip  as  sip

from OpenVisus	           import *
from OpenVisus.VisusGuiPy import *

from OpenVisus.PyImage import *
from OpenVisus.PyScriptingNode import *

# ////////////////////////////////////////////////////////////////
class PyViewer(Viewer):
	
	# constructor
	def __init__(self):	
		super().__init__()
		
	# editNode
	def editNode(self,node):
		
		if not node:
			node=self.getSelection()
		
		node=node.asPythonObject()
		
		if type(node) is PyScriptingNode: 
			win=PyScriptingNodeView(node)
			win.show()
			return
			
		super().editNode(node)
		
	# run
	def run(self):
		bounds=self.getWorldBox()
		self.getGLCamera().guessPosition(bounds)	
		GuiModule.execApplication()	
		
	# addVolumeRender
	def addVolumeRender(self, data, bounds):
		
		t1=Time.now()
		print("Adding Volume render...")	
		
		Assert(isinstance(data,numpy.ndarray))
		data=Array.fromNumPy(data,TargetDim=3)
		data.bounds=bounds
		
		node=RenderArrayNode()
		node.setName("RenderVolume")
		node.setLightingEnabled(False)
		node.setPaletteEnabled(False)
		node.setData(data)
		self.addNode(self.getRoot(), node)
		
		print("done in ",t1.elapsedMsec(),"msec")
		
		
	# addIsoSurface
	def addIsoSurface(self, field=None, second_field=None, isovalue=0, bounds=None):

		Assert(isinstance(field,numpy.ndarray))
		
		field=Array.fromNumPy(field,TargetDim=3)
		field.bounds=bounds

		print("Extracting isocontour...")
		t1=Time.now()
		mesh=MarchingCube(field,isovalue).run()
		print("done in ",t1.elapsedMsec(),"msec")
		
		if second_field is not None:
			mesh.second_field=Array.fromNumPy(second_field,TargetDim=3)
			mesh.second_field.bounds=bounds
					
		node=IsoContourRenderNode()	
		node.setName("RenderMesh")
		node.setMesh(mesh)
		self.addNode(self.getRoot(),node)
		
		print("Added IsoContourRenderNode")	



	