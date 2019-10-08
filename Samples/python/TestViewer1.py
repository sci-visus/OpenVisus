
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


# //////////////////////////////////////////////
def Main(argv):
	
	# set PYTHONPATH=D:/projects/OpenVisus/build/RelWithDebInfo
	# c:\Python37\python.exe CMake/PyViewer.py	
	
	SetCommandLine("__main__")
	GuiModule.createApplication()
	AppKitModule.attach()  	
	
	"""
	allow some python code inside scripting node
	"""

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

