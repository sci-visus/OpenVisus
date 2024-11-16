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
import shutil

import numpy as np

from OpenVisus import *

# ////////////////////////////////////////////////////////////////////////
class TestIdx(unittest.TestCase):
  
	# testIdx
	def testIdx(self):
		data=np.random.randint(0, np.iinfo(np.uint32).max, (16, 16, 16), dtype=np.uint32)
		shutil.rmtree('tmp/test_idx', ignore_errors=True)
		db=CreateIdx(url='tmp/test_idx/visus.idx', dim=3, dims=(16,16,16),fields=[Field("myfield","uint32")],data=data, compression=["zip"])
		data_check=db.read()
		Assert((data==data_check).all())

		# example of doing a progressive query 
		for data in db.read(z=[0,1],num_refinements=3):
			print(data.shape,data.dtype)

# ////////////////////////////////////////////////////////
if __name__ == '__main__':
	unittest.main(verbosity=2,exit=True)
