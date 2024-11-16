## -----------------------------------------------------------------------------
## Copyright(c) 2010 - 2018 ViSUS L.L.C.,
## Scientific Computing and Imaging Institute of the University of Utah
## 
## ViSUS L.L.C., 50 W.Broadway, Ste. 300, 84101 - 2044 Salt Lake City, UT
## University of Utah, 72 S Central Campus Dr, Room 3750, 84112 Salt Lake City, UT
## 
## All rights reserved.
## 
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are met :
## 
## * Redistributions of source code must retain the above copyright notice, this
## list of conditions and the following disclaimer.
## 
## * Redistributions in binary form must reproduce the above copyright notice,
## this list of conditions and the following disclaimer in the documentation
## and/or other materials provided with the distribution.
## 
## * Neither the name of the copyright holder nor the names of its
## contributors may be used to endorse or promote products derived from
## this software without specific prior written permission.
## 
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
## AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
## IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
## DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
## FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
## DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
## SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
## CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
## OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
## OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
## 
## For additional information about this project contact : pascucci@acm.org
## For support : support@visus.net
## -----------------------------------------------------------------------------

import gc
import sys
import math
import unittest
import string 
import unittest
import os

from OpenVisus import *
  
# ////////////////////////////////////////////////////////////////////////
class PyProducer(Node):
	
	# constructor
	def __init__(self): 
		super().__init__()
		self.addOutputPort("output")
		
	# getTypeName
	def getTypeName(self):
		return "PyProducer"

	# processInput (overriding from Node)
	def processInput(self):
		print("PyProducer::processInput")
		msg=DataflowMessage()
		msg.writeString("output","hello visus")
		self.publish(msg)
		return True
    
# ////////////////////////////////////////////////////////////////////////
class PyReceiver(Node):

	# constructor
	def __init__(self): 
		super().__init__()
		self.addInputPort("input")

	# getTypeName
	def getTypeName(self):
		return "PyReceiver"

	# processInput (overriding from Node)
	def processInput(self):
		print("PyReceiver::processInput")
		self.published_value=self.readString("input")
		return True
  

# ////////////////////////////////////////////////////////////////////////
class TestDataflow(unittest.TestCase):
  
	def testDataflow(self): 

		dataflow=Dataflow()

		producer=PyProducer()
		receiver=PyReceiver()

		# the dataflow is the owner of nodes!
		dataflow.addNode(producer)
		dataflow.addNode(receiver)
		dataflow.connectNodes(producer,"output","input",receiver)

		self.assertTrue(dataflow.containsNode(producer))
		self.assertTrue(producer.hasOutputPort("output"))
		self.assertTrue(dataflow.containsNode(receiver))
		self.assertTrue(receiver.hasInputPort ("input" ))

		producer.processInput()

		while dataflow.dispatchPublishedMessages():
			pass

		value=receiver.published_value

		self.assertEqual(value,"hello visus")
		dataflow.disconnectNodes(producer,"output","input",receiver)
		dataflow.removeNode(producer)
		dataflow.removeNode(receiver)  


# ////////////////////////////////////////////////////////
if __name__ == '__main__':
	VISUS_REGISTER_NODE_CLASS("PyProducer", lambda : PyProducer())
	VISUS_REGISTER_NODE_CLASS("PyReceiver", lambda : PyReceiver())
	unittest.main(verbosity=2,exit=True)

