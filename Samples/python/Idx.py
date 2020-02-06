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

import numpy

from OpenVisus import *

# ////////////////////////////////////////////////////////////////////////
class TestIdx(unittest.TestCase):
  
	def testIdx(self):
		self.filename="temp/TestIdx.idx"
		self.WriteIdx()
		self.ReadIdx()
		self.MergeIdx()
	
	# WriteIdx
	def WriteIdx(self): 
		
		dataset_box=BoxNi(PointNi(0,0,0),PointNi(16,16,16))
		
		idxfile=IdxFile();
		idxfile.logic_box=BoxNi(dataset_box)
		idxfile.fields.push_back(Field("myfield",DType.fromString("uint32")))

		idxfile.save(self.filename)
		dataset=LoadDataset(self.filename)
		access=dataset.createAccess()
		
		sampleid=0
		
		for Z in range(0,16):
			slice_box=dataset.getLogicBox().getZSlab(Z,Z+1)
			
			query=BoxQuery(dataset,dataset.getDefaultField(),dataset.getDefaultTime(),ord('w'))
			query.logic_box=slice_box
			dataset.beginQuery(query)
			self.assertTrue(query.isRunning())
			self.assertEqual(query.getNumberOfSamples().innerProduct(),16*16)
			
			buffer=Array(query.getNumberOfSamples(),query.field.dtype)
			query.buffer=buffer
			
			fill=Array.toNumPy(buffer,bShareMem=True)
			for Y in range(16):
				for X in range(16):
					fill[Y,X]=sampleid
					sampleid+=1

			self.assertTrue(dataset.executeQuery(access,query))

	# ReadIdx
	def ReadIdx(self): 
		
		dataset=LoadDataset(self.filename)
		box=dataset.getLogicBox()
		field=dataset.getDefaultField()
		access=dataset.createAccess()
		
		sampleid=0
		for Z in range(0,16):
			slice_box=box.getZSlab(Z,Z+1)
			
			query=BoxQuery(dataset,dataset.getDefaultField(),dataset.getDefaultTime(),ord('r'))
			query.logic_box=slice_box
			dataset.beginQuery(query)
			self.assertTrue(query.isRunning())
			self.assertEqual(query.getNumberOfSamples().innerProduct(),16*16)
			self.assertTrue(dataset.executeQuery(access,query))
			
			check=Array.toNumPy(query.buffer,bShareMem=True)
			
			for Y in range(16):
				for X in range(16):
					self.assertEqual(check[Y,X],sampleid)
					sampleid+=1

	def MergeIdx(self): 
		
		dataset=LoadDataset(self.filename)
		
		box=dataset.getLogicBox()
		access=dataset.createAccess()
		field=dataset.getDefaultField()
		self.assertEqual(dataset.getMaxResolution(),12) #in the bitmask_pattern "V012012012012" the very last bit of the bitmask is at position MaxH=12 
		
		#I want to read data from first slice Z=0
		slice_box=box.getZSlab(0,1);
		
		#create and read data from VisusFIle up to resolution FinalH=8 (<MaxH)
		query=BoxQuery(dataset,dataset.getDefaultField(),dataset.getDefaultTime(),ord('r'))
		query.logic_box=slice_box
		query.end_resolutions.push_back(8)
		query.end_resolutions.push_back(12)
		
		# end_resolution=8
		dataset.beginQuery(query)
		self.assertTrue(query.isRunning())
		self.assertTrue(query.getNumberOfSamples().innerProduct()>0)
		self.assertTrue(dataset.executeQuery(access,query))
		self.assertEqual(query.getCurrentResolution(),8)
		
		# end_resolution=12
		dataset.nextQuery(query)
		self.assertTrue(query.isRunning())
		self.assertEqual(query.getNumberOfSamples().innerProduct(),16*16)
		self.assertTrue(dataset.executeQuery(access,query))
		self.assertEqual(query.getCurrentResolution(),12)
		
		#verify the data is correct
		check=Array.toNumPy(query.buffer,bShareMem=True)
		sampleid=0
		for Y in range(0,16):
			for X in range(0,16):
				self.assertEqual(check[Y,X],sampleid)
				sampleid+=1 


# ////////////////////////////////////////////////////////
if __name__ == '__main__':
	DbModule.attach()
	unittest.main(exit=True)
