import sys,os

OpenVisusDir=os.path.dirname(os.path.abspath(__file__))

for it in (".","bin"):
	dir = os.path.join(OpenVisusDir,it)
	if not dir in sys.path and os.path.isdir(dir):
		sys.path.append(dir)
		
VISUS_GUI=os.path.isfile(os.path.join(OpenVisusDir,"QT_VERSION"))

from VisusKernelPy    import *
from VisusDataflowPy  import *
from VisusDbPy        import *
from VisusIdxPy       import *
from VisusXIdxPy      import *
from VisusNodesPy     import *

# automatic import of VISUS_GUI is disabled
if False and VISUS_GUI:
	from VisusGuiPy      import *
	from VisusGuiNodesPy import *
	from VisusAppKitPy   import *
	
# print("OpenVisus imported","VISUS_GUI",VISUS_GUI)

