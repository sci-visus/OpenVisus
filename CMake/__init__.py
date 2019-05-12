import sys,os

OpenVisus_Dir=os.path.dirname(os.path.abspath(__file__))

for it in (".","bin"):
	dir = os.path.join(OpenVisus_Dir,it)
	if not dir in sys.path and os.path.isdir(dir):
		sys.path.append(dir)
		
VISUS_GUI=os.path.isfile(os.path.join(OpenVisus_Dir,"QT_VERSION"))

from VisusKernelPy    import *
print("VisusKernelPy imported")

try:
	from VisusXIdxPy import *
	print("VisusXIdxPy imported")
except:
	print("VisusXIdxPy missing or failed")
	
try:
	from VisusDbPy import *
	print("VisusDbPy imported")
except:
	print("VisusDbPy missing or failed")
	
try:
	from VisusIdxPy import *
	print("VisusIdxPy imported")
except:
	print("VisusIdxPy missing or failed")

try:
	from VisusDataflowPy  import *
	print("VisusDataflowPy imported")
except:
	print("VisusDataflowPy missing or failed")

try:
	from VisusNodesPy import *
	print("VisusNodesPy imported")
except:
	print("VisusNodesPy missing or failed")

# automatic import of VISUS_GUI is disabled
if False :
	try:
		from VisusGuiPy import *
		print("VisusGuiPy imported")
	except:
		print("VisusGuiPy missing or failed")
		
	try:
		from VisusGuiNodesPy import *
		print("VisusGuiNodesPy imported")
	except:
		print("VisusGuiNodesPy missing or failed")
		
	try:
		from VisusAppKitPy import *
		print("VisusAppKitPy imported")
	except:
		print("VisusAppKitPy missing or failed")	
	



