
import sys, os

from OpenVisus       import *

# IMPORTANT for WIndows
# Mixing C++ Qt5 and PyQt5 won't work in Windows/DEBUG mode
# because forcing the use of PyQt5 means to use only release libraries (example: Qt5Core.dll)
# but I'm in need of the missing debug version (example: Qt5Cored.dll)
# as you know, python (release) does not work with debugging versions, unless you recompile all from scratch

# on windows rememeber to INSTALL and CONFIGURE

from OpenVisus.VisusGuiPy      import *
from OpenVisus.VisusGuiNodesPy import *
from OpenVisus.VisusAppKitPy   import *

from OpenVisus.PyViewer        import *

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
	# c:\Python37\python.exe CMake/PyViewer.py	
	
	SetCommandLine("__main__")
	GuiModule.createApplication()
	AppKitModule.attach()  		

	viewer=PyViewer()
	viewer.open(r".\datasets\cat\gray.idx")
	# ... with some little python scripting
	viewer.setScriptingCode("""
import cv2,numpy
pdim=input.dims.getPointDim()
img=Array.toNumPy(input,bShareMem=True)
img=cv2.Laplacian(img,cv2.CV_64F)
output=Array.fromNumPy(img,TargetDim=pdim)
""".strip())	
	viewer.run()
	
	GuiModule.execApplication()
	viewer=None  
	AppKitModule.detach()
	print("All done")
	sys.exit(0)		


# //////////////////////////////////////////////
if __name__ == '__main__':
	Main(sys.argv)

