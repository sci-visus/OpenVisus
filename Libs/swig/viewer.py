import os,sys

def ImportQt5Sip():
	try:
		 from PyQt5 import sip as  sip
	except ImportError:
		 import sip

import PyQt5
from   PyQt5.QtCore	import *
from   PyQt5.QtWidgets import *
from   PyQt5.QtGui	 import *

ImportQt5Sip()

from OpenVisus	    import *
from OpenVisus.gui import *

# ////////////////////////////////////////////////////////////////
class PyViewer(Viewer):
	
	# constructor
	def __init__(self,title="PyViewer"):	
		super().__init__(title)
		self.render_palettes=False
	
	# run
	def run(self):
		bounds=self.getWorldBox()
		self.getGLCamera().guessPosition(bounds)	
		QApplication.exec()	
		
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

	# findQueryNodes
	def findQueryNodes(self):
		dataflow=self.getDataflow()
		return [QueryNode.castFrom(it) for it in dataflow.getNodesAsVector() if QueryNode.castFrom(it)]

	# findPaletteNodes
	def findPaletteNodes(self):
		dataflow=self.getDataflow()
		return [PaletteNode.castFrom(it) for it in dataflow.getNodesAsVector() if PaletteNode.castFrom(it)]

	# glRenderTexturedQuad
	def glRenderTexturedQuad(self, gl, texture=None, bounds=None):
		x1,y1,w,h=bounds
		x2,y2=x1+w,y1+h

		# draw a 2d quad
		GL_QUADS=0x0007
		mesh=GLMesh()
		mesh.begin(GL_QUADS);
		mesh.texcoord2(0,0);mesh.vertex(x1,y1)
		mesh.texcoord2(0,1);mesh.vertex(x2,y1)
		mesh.texcoord2(1,1);mesh.vertex(x2,y2)
		mesh.texcoord2(1,0);mesh.vertex(x1,y2)
		mesh.end()

		obj=GLPhongObject()
		obj.color=Color(255,255,0)
		obj.texture=GLTexture.createFromArray(Array.fromNumPy(texture,TargetDim=1))
		obj.mesh=mesh
		obj.glRender(gl)

	# pushHUD
	def pushHUD(self,gl):
		gl.pushFrustum()
		gl.setHud()
		gl.pushDepthTest(False)
		gl.pushBlend(True)

	# popHUD
	def popHUD(self,gl):
		gl.popBlend()
		gl.popDepthTest()
		gl.popFrustum()

	# glRenderPalette
	def glRenderPalette(self,gl, texture=None, stats=None,bounds=None, slot=0, default_w=32, default_h=600, default_border=30, disable_transparency=True):
		
		# automatic displacemente basing on slot
		if bounds is None:
			W=gl.getViewport().width
			H=gl.getViewport().height
			w,h,border=default_w,default_h,default_border
			bounds=[W-(slot+1)*(w+border),H-h-border,w,h]
			
		if disable_transparency:
			texture[:,3]=255 

		font_height=10
		x1,y1=bounds[0],bounds[1]
		x2,y2=x1+bounds[2],y1+bounds[3]
		self.glRenderTexturedQuad(gl, texture=texture, bounds=bounds)
		gl.glRenderScreenText(x1,y1-font_height,"{}".format(stats.computed_range.From if stats else 0.0),Color(255,255,255))
		gl.glRenderScreenText(x1,y2            ,"{}".format(stats.computed_range.To   if stats else 0.0),Color(255,255,255))

	# glRenderPalettes
	def glRenderPalettes(self,gl):
		for I, node in enumerate(self.findPaletteNodes()):
			node.setStatisticsEnabled(True) # I am assuming I want to see statistics
			stats=node.getLastStatistics()
			texture=Array.toNumPy(node.getPalette().toArray())
			self.glRenderPalette(gl,
								texture=texture, 
								stats=stats.components[0] if len(stats.components) else None, 
								slot=I)
		
	# glRenderNodes (overriding from Viewer)
	def glRenderNodes(self, gl):

		Viewer.glRenderNodes(self,gl)

		if self.render_palettes:
			self.pushHUD(gl)
			self.glRenderPalettes(gl)
			self.popHUD(gl)