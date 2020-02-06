
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
from OpenVisus.PyScriptingNode import *


# //////////////////////////////////////////////
def Main(argv):
	
	# set PYTHONPATH=D:/projects/OpenVisus/build/RelWithDebInfo
	# c:\Python37\python.exe CMake/PyViewer.py	
	
	CreateQtApplication()
	
	AppKitModule.attach()
	
	VISUS_REGISTER_NODE_CLASS("ScriptingNode", "PyScriptingNode", lambda : PyScriptingNode())
	
	# allow some python code inside scripting node
	viewer=PyViewer()
	viewer.open(r".\datasets\cat\gray.idx")
	
	code="\n".join([
		"import numpy,cv2",
		"print(type(input),input.shape,input.dtype)",
		"output=cv2.Laplacian(input,cv2.CV_64F)"
		])
	
	viewer.setScriptingCode(code)
	viewer.run()
	
	ExecQtApplication()
	viewer=None  
	print("All done")
	sys.exit(0)	
	

# //////////////////////////////////////////////
if __name__ == '__main__':
	Main(sys.argv)

