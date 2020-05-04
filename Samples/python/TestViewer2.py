
import sys, os

from OpenVisus import *

# IMPORTANT for WIndows
# Mixing C++ Qt5 and PyQt5 won't work in Windows/DEBUG mode
# because forcing the use of PyQt5 means to use only release libraries (example: Qt5Core.dll)
# but I'm in need of the missing debug version (example: Qt5Cored.dll)
# as you know, python (release) does not work with debugging versions, unless you recompile all from scratch

# on windows rememeber to INSTALL and CONFIGURE

from OpenVisusGui import *

from OpenVisus.PyViewer import *

import PyQt5
from   PyQt5.QtCore    import *
from   PyQt5.QtWidgets import *
from   PyQt5.QtGui     import *
import PyQt5.sip

# ///////////////////////////////////////////////////////////
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
		self.addInputPort("array")
    
	# getTypeName
	def getTypeName(self):    
		return "MyPythonNode"
    
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
		mesh.vertex( 0, 0)
		mesh.vertex(50, 0)
		mesh.vertex(50,50)
		mesh.vertex( 0,50)
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

		
# //////////////////////////////////////////////
def Main(argv):		
	
	"""
	allow some python code inside scripting node
	"""
	
	# set PYTHONPATH=D:/projects/OpenVisus/build/RelWithDebInfo
	# c:\Python37\python.exe Libs/Gui/PyViewer.py	
	SetCommandLine("__main__")
	GuiModule.createApplication()
	GuiModule.attach()  
	
	VISUS_REGISTER_NODE_CLASS("MyPythonNode", "MyPythonNode", lambda : MyPythonNode())

	viewer=Viewer()
	viewer.open("http://atlantis.sci.utah.edu/mod_visus?dataset=2kbit1") 

	# example of adding a PyQt5 widget to C++ Qt
	mywidget=MyWidget()
	viewer.addDockWidget("MyWidget",ToCppQtWidget(PyQt5.sip.unwrapinstance(mywidget)))

	# example of adding a python node to the dataflow
	root=viewer.getRoot()
	world_box=viewer.getWorldBox()

	pynode=MyPythonNode()
	pynode.glSetRenderQueue(999)
	pynode.setBounds(Position(world_box))
	viewer.addNode(root,pynode)

	# pynode will get the data from the query
	query_node=viewer.findNodeByUUID("volume")
	viewer.connectNodes(query_node, pynode)
	 
	GuiModule.execApplication()
	viewer=None  
	GuiModule.detach()
	print("All done")
	sys.exit(0)


# //////////////////////////////////////////////
if __name__ == '__main__':
	Main(sys.argv)

