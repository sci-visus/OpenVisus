import sys,os

OpenVisus_Dir=os.path.dirname(os.path.abspath(__file__))

for it in (".","bin"):
	dir = os.path.join(OpenVisus_Dir,it)
	if not dir in sys.path and os.path.isdir(dir):
		sys.path.append(dir)
		
VISUS_GUI=os.path.isfile(os.path.join(OpenVisus_Dir,"QT_VERSION"))

if os.path.isfile(os.path.join(OpenVisus_Dir,"VisusKernelPy.py")) :
	from VisusKernelPy import *

if os.path.isfile(os.path.join(OpenVisus_Dir,"VisusXIdxPy.py")) :
	from VisusXIdxPy import *
	
if os.path.isfile(os.path.join(OpenVisus_Dir,"VisusDbPy.py")) :
	from VisusDbPy import *

if os.path.isfile(os.path.join(OpenVisus_Dir,"VisusIdxPy.py")) :
	from VisusIdxPy import *

if os.path.isfile(os.path.join(OpenVisus_Dir,"VisusDataflowPy.py")) :
	from VisusDataflowPy import *

if os.path.isfile(os.path.join(OpenVisus_Dir,"VisusNodesPy.py")) :
	from VisusNodesPy import *

# automatic import of VISUS_GUI is disabled
if False:

	if os.path.isfile(os.path.join(OpenVisus_Dir,"VisusGuiPy.py")) :
		from VisusGuiPy import *

	if os.path.isfile(os.path.join(OpenVisus_Dir,"VisusGuiNodesPy.py")) :
		from VisusGuiNodesPy import *
	
	if os.path.isfile(os.path.join(OpenVisus_Dir,"VisusAppKitPy.py")) :
		from VisusAppKitPy import *



