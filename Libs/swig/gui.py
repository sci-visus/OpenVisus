from OpenVisus.VisusGuiPy import *

GuiModule.attach()

from OpenVisus.scripting_node  import *
from OpenVisus.viewer          import *

def VISUS_REGISTER_NODE_CLASS(TypeName, PyTypeName, creator):
	
	print("Registering Python class",TypeName,PyTypeName)
	
	from OpenVisus.VisusDataflowPy import NodeCreator,NodeFactory
	
	class PyNodeCreator(NodeCreator):
		
		def __init__(self,creator):
			print("PyNodeCreator","constructor")
			super().__init__()
			self.creator=creator
			
		def createInstance(self):
			print("PyNodeCreator","createInstance")
			return self.creator()
			
	NodeFactory.getSingleton().registerClass(TypeName, PyTypeName , PyNodeCreator(creator))

VISUS_REGISTER_NODE_CLASS("ScriptingNode", "PyScriptingNode", lambda : PyScriptingNode())