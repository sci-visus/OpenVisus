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

import sys
import os
import unittest
import numpy

from OpenVisus import *

# ////////////////////////////////////////////////////////////////////
class MyTestCase(unittest.TestCase):

	# test1: convert numpy->Array with no memory sharing
	def test1(self):
		width,height,ncomponents=5,4,3
		A=numpy.zeros((height,width,ncomponents),dtype=numpy.float32)
		
		B=Array.fromNumPy(A,bShareMem=False)
		self.assertEqual(B.dims,PointNi(ncomponents,width,height))
		self.assertEqual(B.dtype.toString(),"float32")
		self.assertNotEqual(str(A.__array_interface__["data"][0]),B.c_address())
		
		# use TargetDim if you want the dtype to have multiple ncompgnts
		B=Array.fromNumPy(A,TargetDim=2,bShareMem=False)
		self.assertEqual(B.dims,PointNi(width,height))
		self.assertEqual(B.dtype.toString(),"float32[3]")
		self.assertNotEqual(str(A.__array_interface__["data"][0]),B.c_address())
		

	# test2: convert Array->numpy with no memory sharing
	def test2(self):
		width,height,ncomponents=5,4,3
		A=Array(width,height,DType.fromString("float32[{}]".format(ncomponents)))
		B=Array.toNumPy(A,bShareMem=False)
		self.assertEqual(B.shape,(4,5,3))
		self.assertEqual(B.dtype,numpy.float32)
		self.assertNotEqual(str(B.__array_interface__["data"][0]),A.c_address())

	# test3 : convert numpy->Array with memory sharing, be careful to keep the numpy array alive
	def test3(self):
		width,height,ncomponents=5,4,3
		A=numpy.zeros((height,width,ncomponents),dtype=numpy.float32)
		
		B=Array.fromNumPy(A,bShareMem=True)
		self.assertEqual(B.dims,PointNi(ncomponents,width,height))
		self.assertEqual(B.dtype.toString(),"float32")
		self.assertEqual(str(A.__array_interface__["data"][0]),B.c_address())
		
		# use TargetDim if you want the dtype to have multiple ncomponents
		B=Array.fromNumPy(A,TargetDim=2,bShareMem=True)
		self.assertEqual(B.dims,PointNi(width,height))
		self.assertEqual(B.dtype.toString(),"float32[3]")
		self.assertEqual(str(A.__array_interface__["data"][0]),B.c_address())

	# test4: convert Array->numpy with memory sharing, be careful to keep the OpenVisus array alive
	def test4(self):
		width,height,ncomponents=5,4,3
		A=Array(width,height,DType.fromString("float32[{}]".format(ncomponents)))
		B=Array.toNumPy(A,bShareMem=True)
		self.assertEqual(B.shape,(4,5,3))
		self.assertEqual(B.dtype,numpy.float32)
		self.assertEqual(str(B.__array_interface__["data"][0]),A.c_address())
		
		

# ////////////////////////////////////////////////////////
if __name__ == '__main__':
	unittest.main(verbosity=2,exit=True)


