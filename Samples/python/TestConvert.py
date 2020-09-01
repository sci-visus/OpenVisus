
import os,sys
import unittest
from PIL import Image
import random

from OpenVisus import *

# ////////////////////////////////////////////////////////////////////////////////////////////////
def ComposeImages(images, bHorizontal=True):
	# https://stackoverflow.com/questions/30227466/combine-several-images-horizontally-with-python
	images=[Image.fromarray(it) for it in images]
	widths, heights = zip(*(i.size for i in images))
	W=sum(widths)  if bHorizontal else max(widths)
	H=max(heights) if bHorizontal else sum(heights)
	ret = Image.new('RGB', (W ,H), color=(220, 140, 60))
	offset = 0
	for img in images:
		ret.paste(img, (offset, 0) if bHorizontal else (0,offset))
		offset += img.size[0] if bHorizontal else img.size[1]
	return numpy.asarray(ret)

# ////////////////////////////////////////////////////////////////////
def SaveImage(url, image):
	Image.fromarray(image).save(url)

# ////////////////////////////////////////////////////////////////////
class MyTestCase(unittest.TestCase):
	
	def testCreateEmpty3dDataset(self):
		dtype="uint8[3]"
		width,height,depth=1025,512,256
		field=Field("data",dtype,"row_major")
		db=CreateIdx(url='tmp/test_convert/visus.idx', rmtree=True, dims=[width,height,depth],fields=[field])

	def testCreate2dDatasetFromNumPy(self):
		data=numpy.asarray(Image.open('datasets/cat/rgb.png'))
		db=CreateIdx(url='tmp/test_convert/visus.idx', rmtree=True, dim=2,data=data)
		
	def testCreate3dDatasetFromNumPy(self):
		data=numpy.zeros((100,100,100,3),dtype=numpy.float32) # depth,height,width,nchannels
		db=CreateIdx(url='tmp/test_convert/visus.idx', rmtree=True, dim=3, data=data)
		
	def testCreate3dDatasetFromSlices(self):
		width, height,depth=256,256,10
		fields=[Field("data","uint8[3]","row_major")]
		db=CreateIdx(url='tmp/test_convert/visus.idx',rmtree=True, dims=[width,height,depth],fields=fields)

		rgb=numpy.asarray(Image.open('datasets/cat/rgb.png'))

		def generateSlices():
			for I in range(depth): 
				yield rgb

		db.writeSlabs(generateSlices(),z=0)
			
		# read a slice in the middle
		middle=int(depth/2)
		check=db.read(z=[middle, middle+1])
		Assert((rgb==check).all())
	
	def testCreate3dDatasetAndReadStuff(self):
		
		width,height,depth=256,256,100
		
		field=Field("data","uint8[3]","row_major")
		db=CreateIdx(url='tmp/test_convert/visus.idx', rmtree=True, dims=[width,height,depth], fields=[field])
		# print(db.getDatasetBody().toString())
			
		# write first slice at offset z=0
		rgb=numpy.asarray(Image.open('datasets/cat/rgb.png'))
		db.write(rgb,z=0)
		
		# read back slice at z=0
		check=db.read(z=[0,1])
		Assert((rgb==check).all())
		
		# read slice at z=0 with 3 refinements (coarse to fine)
		for I,data in enumerate(db.read(z=[0,1],num_refinements=3)):
			level=data[0,:,:]
			Assert(I<2 or (rgb==level).all())
			
	def testCompression(self):
		dims=(128,128,128)
		db=CreateIdx(url="tmp/test_convert/visus.idx",rmtree=True, blocksperfile=4, dims=(dims),fields=[Field("data","uint8")])
		data=numpy.zeros(dims,numpy.uint8)
		# add some random sample
		for z in [random.randint(0, dims[2]-1) for I in range(int(dims[2]/2))]:
			for y in [random.randint(0, dims[1]-1) for I in range(int(dims[1]/2))]:
				for x in [random.randint(0, dims[0]-1) for I in range(int(dims[0]/2))]:
					data[z,y,x]=random.randint(0, 255)
		db.write(data)
		db.compressDataset(["zip"])
		data_check=db.read()
		Assert((data==data_check).all())


# ///////////////////////////////////////////////////////////
if __name__ == '__main__':
	unittest.main(verbosity=2,exit=True)