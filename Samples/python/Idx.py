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

from VisusKernelPy import *
from VisusDbPy import *
from VisusIdxPy import *

# ////////////////////////////////////////////////////////////////////////
class TextIdx(unittest.TestCase):
  
  def testIdx(self):
    self.filename="temp/tutorial_1.idx"
    self.WriteIdx()
    self.ReadIdx()
    self.MergeIdx()
  
  # WriteIdx
  def WriteIdx(self): 
    
    dataset_box=NdBox(NdPoint(0,0,0),NdPoint.one(16,16,16))
    
    idxfile=IdxFile();
    idxfile.box=NdBox(dataset_box)
    idxfile.fields.push_back(Field("myfield",DType.fromString("uint32")))

    bSaved=idxfile.save(self.filename)
    self.assertTrue(bSaved)
    
    dataset=Dataset.loadDataset(self.filename)
    self.assertIsNotNone(dataset)
    access=dataset.get().createAccess()
    
    sampleid=0
    
    for Z in range(0,16):
      slice_box=dataset.get().getBox().getZSlab(Z,Z+1)
      
      query=QueryPtr(Query(dataset.get(),ord('w')))
      query.get().position=Position(slice_box)
      
      self.assertTrue(dataset.get().beginQuery(query))
      self.assertEqual(query.get().nsamples.innerProduct(),16*16)
      
      buffer=Array(query.get().nsamples,query.get().field.dtype)
      query.get().buffer=buffer
      
      fill=buffer.toNumPy()
      for Y in range(16):
        for X in range(16):
          fill[Y,X]=sampleid
          sampleid+=1

      self.assertTrue(dataset.get().executeQuery(access,query))

  # ReadIdx
  def ReadIdx(self): 
    
    dataset=Dataset_loadDataset(self.filename)
    self.assertIsNotNone(dataset)
    box=dataset.get().getBox()
    field=dataset.get().getDefaultField()
    access=dataset.get().createAccess()
    
    sampleid=0
    for Z in range(0,16):
      slice_box=box.getZSlab(Z,Z+1)
      
      query=QueryPtr(Query(dataset.get(),ord('r')))
      query.get().position=Position(slice_box)
      
      self.assertTrue(dataset.get().beginQuery(query))
      self.assertEqual(query.get().nsamples.innerProduct(),16*16)
      self.assertTrue(dataset.get().executeQuery(access,query))
      
      check=query.get().buffer.toNumPy()
      
      for Y in range(16):
        for X in range(16):
          self.assertEqual(check[Y,X],sampleid)
          sampleid+=1

  def MergeIdx(self): 
    
    dataset=Dataset_loadDataset(self.filename)
    self.assertIsNotNone(dataset)
    
    box=dataset.get().getBox()
    access=dataset.get().createAccess()
    field=dataset.get().getDefaultField()
    MaxH=dataset.get().getBitmask().getMaxResolution()
    self.assertEqual(MaxH,12) #in the bitmask_pattern "V012012012012" the very last bit of the bitmask is at position MaxH=12 
    
    #I want to read data from first slice Z=0
    slice_box=box.getZSlab(0,1);
    
    #create and read data from VisusFIle up to resolution FinalH=8 (<MaxH)
    query=QueryPtr(Query(dataset.get(),ord('r')))
    query.get().position=Position(slice_box)
    query.get().end_resolutions.push_back(8)
    query.get().end_resolutions.push_back(12)
    query.get().merge_mode=Query.InsertSamples
    
    # end_resolution=8
    
    self.assertTrue(dataset.get().beginQuery(query))
    self.assertTrue(query.get().nsamples.innerProduct()>0)
    self.assertTrue(dataset.get().executeQuery(access,query))
    self.assertEqual(query.get().cur_resolution,8)
    
    # end_resolution=12
    self.assertTrue(dataset.get().nextQuery(query))
    self.assertEqual(query.get().nsamples.innerProduct(),16*16)
    self.assertTrue(dataset.get().executeQuery(access,query))
    self.assertEqual(query.get().cur_resolution,12)
    
    #verify the data is correct
    check=query.get().buffer.toNumPy()
    sampleid=0
    for Y in range(0,16):
      for X in range(0,16):
        self.assertEqual(check[Y,X],sampleid)
        sampleid+=1 
        
    # finished
    
    self.assertFalse(dataset.get().nextQuery(query)) 


# ////////////////////////////////////////////////////////
if __name__ == '__main__':
  SetCommandLine("__main__")
  IdxModule.attach()
  unittest.main(exit=False)
  IdxModule.detach()

