
import time
import unittest

from OpenVisus import *

# /////////////////////////////////////////////////
class MyReceiverNode(Node):
	
	def __init__(self, name="viewer node"):
		super().__init__()
		self.setName(name)
		self.addInputPort("array")
		self.addInputPort('dataset')
		
	def getTypeName(self):
		return "MyReceiverNode"

	def getOsDependentTypeName(self):
		return "MyReceiverNode"

	def processInput(self):
		print("processInput")
		return super().processInput()

# /////////////////////////////////////////////////
class MyDataflow(Dataflow):
	def __init__(self):
		
		super().__init__()

		dataset = LoadDataset("http://atlantis.sci.utah.edu/mod_visus?dataset=david")
		self.bounds=dataset.getLogicBox()
		self.bounds=self.bounds.scaleAroundCenter(0.01) # 1% of the overall dataset
		print("bounds:", self.bounds.toString(),"width",self.bounds.size()[0],"height",self.bounds.size()[1])

		self.field = None

		self.dataset_node = DatasetNode()
		self.dataset_node.setName('Dataset node')
		self.addNode(self.dataset_node)

		self.query_node = QueryNode()
		self.query_node.setName('query node')
		self.addNode(self.query_node)
		self.connectNodes(self.dataset_node, "dataset", self.query_node)

		self.viewer_node = MyReceiverNode()
		self.viewer_node.setName('viewer node')
		self.addNode(self.viewer_node)
		self.connectNodes(self.query_node, "array", self.viewer_node)
		self.connectNodes(self.dataset_node, "dataset", self.viewer_node)

		self.dataset_node.setDataset(dataset, True)

		self.query_node.setVerbose(True)
		self.query_node.setAccessIndex(0)
		self.query_node.setProgression(QueryGuessProgression)
		self.query_node.setQuality(QueryDefaultQuality)
		self.query_node.setBounds(Position(self.bounds))
		self.query_node.setQueryBounds(Position(self.bounds))
		
		view_dep=True
		self.query_node.setViewDependentEnabled(view_dep)
		if view_dep:
			# this is a frustum which map each pixels in the buffer to a screen pixel
			X1,Y1,X2,Y2=self.bounds.p1[0],self.bounds.p1[1],self.bounds.p2[0],self.bounds.p2[1]
			frustum=Frustum()
			frustum.loadModelview(Matrix.identity(4))
			frustum.loadProjection(Matrix.ortho(X1,X2,Y1,Y2,-1,+1))
			frustum.setViewport(Rectangle2d(0,0,X2-X1,Y2-Y1))
			self.query_node.setNodeToScreen(frustum)

		self.setTime(dataset.getDefaultTime())
		self.setFieldName(dataset.getDefaultField().name)
		
	def setFieldName(self, name):
		self.query_node.getInputPort('fieldname').writeString(name)
		self.needProcessInput(self.query_node)

	def setTime(self, value):
		port = self.query_node.getInputPort('time')
		port.writeDouble(value)
		self.needProcessInput(self.query_node)

	def processInput(self, node):
		print('processInput')
		super().processInput(node)
		
# ////////////////////////////////////////////////////////////////////////
class MyTest(unittest.TestCase):
  
	def test1(self): 
		dataflow = MyDataflow()
		while dataflow.dispatchPublishedMessages():
			print('dispatching messages')
			
		# wait threads to finish processing
		dataflow.joinProcessing()		

if __name__ == '__main__':
	VISUS_REGISTER_NODE_CLASS("MyReceiverNode","MyReceiverNode", lambda : MyReceiverNode())
	unittest.main(verbosity=2,exit=True)
