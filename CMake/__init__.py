import sys,os,platform

OpenVisus_Dir=os.path.dirname(os.path.abspath(__file__))

WIN32=platform.system()=="Windows" or platform.system()=="win32"
VISUS_GUI=os.path.isfile(os.path.join(OpenVisus_Dir,"QT_VERSION"))

# need to allow windows to find DLLs in bin/ directory
# for osx/unix i use the rpath trick
if WIN32:
	for dir in [os.path.join(OpenVisus_Dir,it) for it in (".","bin")]:
		os.environ['PATH'] = dir + os.pathsep + os.environ['PATH']
		sys.path.append(dir)

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



