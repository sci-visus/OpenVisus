from OpenVisus.VisusGuiPy import *

GuiModule.attach()

from OpenVisus.scripting_node  import *
from OpenVisus.viewer          import *

def VISUS_REGISTER_NODE_CLASS(TypeName, creator):
	# print("Registering Python class",TypeName)
	from OpenVisus.VisusDataflowPy import NodeCreator,NodeFactory
	class PyNodeCreator(NodeCreator):
		def __init__(self,creator):
			super().__init__()
			self.creator=creator
		def createInstance(self):
			return self.creator()
	NodeFactory.getSingleton().registerClass(TypeName, PyNodeCreator(creator))

VISUS_REGISTER_NODE_CLASS("ScriptingNode",   lambda : PyScriptingNode())
VISUS_REGISTER_NODE_CLASS("PyScriptingNode", lambda : PyScriptingNode())