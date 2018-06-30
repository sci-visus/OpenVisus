
import sys, os, sip

from OpenVisus import *

import PyQt5
from PyQt5.QtCore    import *
from PyQt5.QtWidgets import *
from PyQt5.QtGui     import *

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
    self.addInputPort("data")
    
  # getOsDependentTypeName
  def getOsDependentTypeName(self):
    return "MyPythonNode"
  
  # glRender
  def glRender(self, gl):
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
    return PythonNode.processInput(self)

# ///////////////////////////////////////////////////////////
def main():

  viewer=Viewer()
  viewer.openFile("http://atlantis.sci.utah.edu/mod_visus?dataset=2kbit1") 

  # example of adding a PyQt5 widget
  if True:
    mywidget=MyWidget()
    viewer.addDockWidget("MyWidget",convertToQWidget(mywidget))
  
  # example of adding a python node to the dataflow
  if True:
    root=viewer.getRoot()
    world_box=viewer.getWorldBoundingBox()
    
    pynode=MyPythonNode()
    pynode.glSetRenderQueue(999)
    pynode.setNodeBounds(Position(world_box))
    viewer.addNode(root,pynode)
    
    # pynode will get the data from the query
    query_node=viewer.findNodeByName("Volume 1")
    viewer.connectPorts(query_node,"data","data",pynode)
    
  GuiModule.execApplication()
  
  viewer=None
  
# ///////////////////////////////////////////////////////////
def forceGC():
  # try to destroy the viewer here...
  import gc
  gc.collect()
  gc.collect()
  gc.collect() 
  
# ///////////////////////////////////////////////////////////
if __name__ == '__main__':

  SetCommandLine("__main__")
  GuiModule.createApplication()
  AppKitModule.attach()  
  VISUS_REGISTER_PYTHON_OBJECT_CLASS("MyPythonNode")
  
  main()
  forceGC()
  
  AppKitModule.detach()
  sys.exit(0)
