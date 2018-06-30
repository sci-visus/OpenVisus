import sys,os

WIN32=True if sys.platform == 'win32'  else False
APPLE=True if sys.platform == "darwin" else False
THIS_DIR=os.path.dirname(os.path.abspath(__file__))

# add to PYTHONPATH something I need
for it in (".","bin"):
  dir=os.path.abspath(os.path.join(THIS_DIR,it))
  if not dir in sys.path and os.path.isdir(dir):
    sys.path.append(dir)

# /////////////////////////////////////////////////////////
def check():

  try:
    import VisusKernelPy
    import VisusDataflowPy
    print("import non-gui OpenVisus libraries","OK")
  except:
    print("import non-gui OpenVisus libraries","ERROR")
    print("Try to:")
    
    if WIN32:
      print("set PATH="+ ";".join(["%PATH%",THIS_DIR,THIS_DIR+"\\bin"]))  
      
    elif APPLE:
      print("export DYLD_LIBRARY_PATH="+":".join(["$DYLD_LIBRARY_PATH",THIS_DIR + "/bin"]))
      
    else:
      print("export LD_LIBRARY_PATH="+":".join(["$LD_LIBRARY_PATH",THIS_DIR + "/bin"]))
      
    raise
  
  try:
    import PyQt5
    PYQT5_FOUND=True
    PYQT5_DIR=os.path.dirname(PyQt5.__file__)
  except:
    PYQT5_FOUND=False
    PYQT5_DIR=""    
  
  try:
    import VisusKernelPy
    import VisusGuiPy
    print("import gui OpenVisus libraries","OK")
  except:
    print("import gui OpenVisus libraries","ERROR")
    print("You can set point to your Qt directory. For example:")
    print("")
    if WIN32:
      print("set QT5_DIR="+"C:\\Qt\\Qt5.9.2\\5.9.2\\msvc2017_64\\bin")
      print("set PATH="+";".join(["%PATH%","%QT5_DIR"]))
      
    elif APPLE:
      print("export DYLD_FRAMEWORK_PATH="+":".join(["$DYLD_FRAMEWORK_PATH", "..." + "/Qt/lib"]))
      print("export QT_PLUGIN_PATH="     +":".join(["$QT_PLUGIN_PATH"     , "..." + "/Qt/plugins"])) 
      
    else:
      pass # nothing to do? qt5 libs go into /usr/lib... so it will be a default location
    
    print("")
    if PYQT5_FOUND:
      print("Or you can use the existing PyQt5. For example:\n")
      if WIN32:
        print("set PATH="+";".join(["%PATH%",PYQT5_DIR + "\\Qt\\bin"]))
      elif APPLE:
        print("export DYLD_FRAMEWORK_PATH="+":".join(["$DYLD_FRAMEWORK_PATH",PYQT5_DIR + "/Qt/lib"]))
        print("export QT_PLUGIN_PATH="     +":".join(["$QT_PLUGIN_PATH"     ,PYQT5_DIR + "/Qt/plugins"]))
      else:
        print("export LD_LIBRARY_PATH="+":".join(["$LD_LIBRARY_PATH",PYQT5_DIR + ""]))
        print("export QT_PLUGIN_PATH=" +":".join(["$QT_PLUGIN_PATH" ,PYQT5_DIR + "/Qt/plugins"]))

    raise 
    
try:
  from VisusKernelPy import *  
except:
  check()
