
import sys, os

# Qt5 does not work in debug mode
# important to import before OpenVisus
use_pqyt=True
try:
	import PyQt5
	from PyQt5.QtCore    import *
	from PyQt5.QtWidgets import *
	from PyQt5.QtGui     import *
	import PyQt5.sip as  sip
except ImportError:
	use_pqyt=False

from OpenVisus import *


# ///////////////////////////////////////////////////////////
if use_pqyt:
	class MyWidget(QWidget):
	    
		# __init__
		def __init__(self):
			QWidget.__init__(self)
			self.checkbox = QCheckBox('Show title', self)
			self.checkbox.move(20, 20)
			self.checkbox.toggle()
			self.setGeometry(300, 300, 250, 150)
			self.setWindowTitle('QCheckBox')
			self.show()


# ///////////////////////////////////////////////////////////
class MyPythonNode(PythonNode):

	# __init__
	def __init__(self):
		PythonNode.__init__(self)
		self.setName("MyPythonNode")
		self.addInputPort("data")
    
	# getOsDependentTypeName
	def getOsDependentTypeName(self):
		return "MyPythonNode"

	# glRender
	def glRender(self, gl):
		gl.pushFrustum()
		gl.setHud()
		gl.pushDepthTest(False)
		gl.pushBlend(True)
		
		GL_QUADS=0x0007
		mesh=GLMesh()
		mesh.begin(GL_QUADS);
		mesh.vertex(Point3d( 0,0));
		mesh.vertex(Point3d(50,0));
		mesh.vertex(Point3d(50,50));
		mesh.vertex(Point3d( 0,50));
		mesh.end();

		obj=GLPhongObject()
		obj.color=Color(255,255,0)
		obj.mesh=mesh
		obj.glRender(gl)
		
		gl.popBlend()
		gl.popDepthTest()
		gl.popFrustum()

	# processInput
	def processInput(self):
		return PythonNode.processInput(self)

  
# ///////////////////////////////////////////////////////////
if __name__ == '__main__':

	SetCommandLine("__main__")
	GuiModule.createApplication()
	AppKitModule.attach()  

	viewer=Viewer()
	viewer.openFile("http://atlantis.sci.utah.edu/mod_visus?dataset=2kbit1") 

	# example of adding a PyQt5 widget to C++ Qt
	if use_pqyt:
		mywidget=MyWidget()
		viewer.addDockWidget("MyWidget",ToCppQtWidget(sip.unwrapinstance(mywidget)))

	# example of adding a python node to the dataflow
	add_python_node=True
	
	if add_python_node:
		root=viewer.getRoot()
		world_box=viewer.getWorldBoundingBox()
		
		VISUS_REGISTER_NODE_CLASS("MyPythonNode")
		pynode=MyPythonNode()
		pynode.glSetRenderQueue(999)
		pynode.setNodeBounds(Position(world_box))
		viewer.addNode(root,pynode)

		# pynode will get the data from the query
		query_node=viewer.findNodeByName("Volume 1")
		viewer.connectPorts(query_node,"data","data",pynode)
	 
	GuiModule.execApplication()
	viewer=None  
	AppKitModule.detach()
	print("All done")
	sys.exit(0)
