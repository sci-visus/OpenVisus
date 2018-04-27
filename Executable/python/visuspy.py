import sys,os

this_dir=os.path.dirname(os.path.abspath(__file__))

for it in ["."]:
  dir=os.path.join(this_dir,it)
  if (not dir in sys.path) and (os.path.exists(dir)):
    sys.path.append(dir)
    
# fixPathSeparators
def fixPathSeparators(path):
  return path.replace("\\", "/") if sys.platform=="win32" else path.replace("/", "\\") 

# addSysPath
def addSysPath(path): 
  sys.path.append(fixPathSeparators(path))

# addPath
def addPath(path):
  os.environ['PATH'] = fixPathSeparators(path) + ";" + os.environ['PATH']

from VisusKernelPy import *
