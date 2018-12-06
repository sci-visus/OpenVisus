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

from VisusKernelPy import *
from VisusDataflowPy import *
  
# ////////////////////////////////////////////////////////////////////////
class PyProducer(Node):
	
  # constructor
  def __init__(self): 
    Node.__init__(self)
    self.addOutputPort("output")
   
  # getOsDependentTypeName
  def getOsDependentTypeName(self):
    return "Producer"
  
  # processInput (overriding from Node)
  def processInput(self):
    print("PyProducer::processInput")
    msg=DataflowMessagePtr(DataflowMessage())
    msg.get().writeContent("output",ObjectPtr(StringObject("hello visus")))
    self.publish(msg)
    return True
    
# ////////////////////////////////////////////////////////////////////////
class PyReceiver(Node):
	
  # constructor
  def __init__(self): 
    Node.__init__(self)
    self.addInputPort("input")
    
  # getOsDependentTypeName
  def getOsDependentTypeName(self):
    return "Receiver"    
    
  # processInput (overriding from Node)
  def processInput(self):
    print("PyReceiver::processInput")
    self.published_value=cstring(self.readInput("input"))
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
    dataflow.connectPorts(producer,"output","input",receiver)

    self.assertTrue(dataflow.containsNode(producer))
    self.assertTrue(producer.hasOutputPort("output"))
    self.assertTrue(dataflow.containsNode(receiver))
    self.assertTrue(receiver.hasInputPort ("input" ))
    
    producer.processInput()
    
    while dataflow.dispatchPublishedMessages():
      pass
      
    value=receiver.published_value

    self.assertEqual(value,"hello visus")
    dataflow.disconnectPorts(producer,"output","input",receiver)
    dataflow.removeNode(producer)
    dataflow.removeNode(receiver)  
    

# ////////////////////////////////////////////////////////
if __name__ == '__main__':
    SetCommandLine("__main__")
    DataflowModule.attach()
    VISUS_REGISTER_PYTHON_OBJECT_CLASS("PyProducer")
    VISUS_REGISTER_PYTHON_OBJECT_CLASS("PyReceiver")
    
    unittest.main(exit=False)
    DataflowModule.detach()


