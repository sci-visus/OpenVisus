import sys,os,platform

this_dir=os.path.dirname(os.path.abspath(__file__))

WIN32=platform.system()=="Windows" or platform.system()=="win32"
VISUS_GUI=os.path.isfile(os.path.join(this_dir,"QT_VERSION"))


# need to allow windows to find DLLs in bin/ directory
# for osx/unix i use the rpath trick
for dir in [os.path.join(this_dir,it) for it in (".","bin")]:
	os.environ['PATH'] = dir + os.pathsep + os.environ['PATH']
	sys.path.append(dir)

if os.path.isfile(os.path.join(this_dir,"VisusKernelPy.py")) :
	from VisusKernelPy import *

if os.path.isfile(os.path.join(this_dir,"VisusXIdxPy.py")) :
	from VisusXIdxPy import *
	
if os.path.isfile(os.path.join(this_dir,"VisusDbPy.py")) :
	from VisusDbPy import *

if os.path.isfile(os.path.join(this_dir,"VisusDataflowPy.py")) :
	from VisusDataflowPy import *

if os.path.isfile(os.path.join(this_dir,"VisusNodesPy.py")) :
	from VisusNodesPy import *

