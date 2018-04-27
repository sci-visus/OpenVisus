
import sys, os, sip

from visuspy import *

import PyQt5
from PyQt5.QtCore    import *
from PyQt5.QtWidgets import *
from PyQt5.QtGui     import *

# I 'm using Qt libraries coming from PyQt5 (important to use EXACTLY THE SAME VERSION)
if sys.platform == 'win32':
  addPath(os.path.dirname(PyQt5.__file__) + "/Qt/bin")

from VisusKernelPy   import *
from VisusDataflowPy import *
from VisusGuiPy      import *
from VisusGuiNodesPy import *
from VisusAppKitPy   import *

def convertToQWidget(value):
  return VisusGuiPy.convertToQWidget(sip.unwrapinstance(value))

# ///////////////////////////////////////////////////////////
class MyWidget(QWidget):
    
  # __init__
  def __init__(self):
    QWidget.__init__(self)
    self.checkbox = QCheckBox('Show title', self)
    self.checkbox.move(20, 20)
    self.checkbox.toggle()
    self.setGeometry(300, 300, 250, 150)
    self.setWindowTitle('QCheckBox')
    self.show()
    


# ///////////////////////////////////////////////////////////
class MyPythonNode(PythonNode):

  # __init__
  def __init__(self):
    PythonNode.__init__(self)
    self.setName("MyPythonNode")
    
  # getOsDependentTypeName
  def getOsDependentTypeName(self):
    return "MyPythonNode"
  
  # glRender
  def glRender(self, gl):
    Log.printMessage("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Example of rendering")
    gl.pushFrustum()
    gl.setHud()
    gl.pushDepthTest(False)
    gl.pushBlend(True)
    Color(255,255,0)
    Point3d(50,50)
    GLQuadPoint3d(Point3d(0,0),Point3d(50,50),Color(255,255,0),Color(255,255,255)).glRender(gl)
    gl.popBlend()
    gl.popDepthTest()
    gl.popFrustum()
    
    
  # processInput
  def processInput(self):
    Log.printMessage("Example of processInput")
    return _VisusDataflowPy.Node_processInput(self)

# ///////////////////////////////////////////////////////////
def main(app):

  viewer=Viewer()
  
  viewer.openFile("http://atlantis.sci.utah.edu/mod_visus?dataset=2kbit1") 
  world_box=viewer.getWorldBoundingBox()
  Log.printMessage("world_box is " + world_box.toString())
  
  # example of adding a PyQt5 widget
  mywidget=MyWidget()
  viewer.addDockWidget("MyWidget",convertToQWidget(mywidget))
  
  root=viewer.getRoot()
  
  # example of adding a python node to the dataflow
  pynode=MyPythonNode()
  pynode.glSetRenderQueue(999)
  pynode.setNodeBounds(Position(world_box))
  viewer.addNode(root,pynode)
 
  retcode=app.exec_()
  viewer=None
  
  # try to destroy the viewer here...
  import gc
  gc.collect()
  gc.collect()
  gc.collect()

  return retcode
  
  

  
# ///////////////////////////////////////////////////////////

if __name__ == '__main__':
  QCoreApplication.setAttribute(Qt.AA_ShareOpenGLContexts)
  app = QApplication(sys.argv)
  SetCommandLine("__main__")
  AppKitModule.attach()  
  VISUS_REGISTER_PYTHON_OBJECT_CLASS("MyPythonNode")
  retcode=main(app)
  AppKitModule.detach()
  sys.exit(retcode)
