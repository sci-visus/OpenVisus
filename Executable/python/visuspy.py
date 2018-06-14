import sys,os

# addSysPath
def addSysPath(path): 
  path=os.path.abspath(path)
  print("Adding sys path "+path)
  sys.path.append(path)

# addEnvPath
def addEnvPath(path):
  path=os.path.abspath(path)
  print("Adding env path "+path)
  os.environ['PATH'] = path + ";" + os.environ['PATH']

for it in ["." , "./bin"]:
  dir=os.path.join(os.path.dirname(os.path.abspath(__file__)),it)
  if (not dir in sys.path) and (os.path.exists(dir)):
    addSysPath(dir)
    addEnvPath(dir)
    
from VisusKernelPy import *
