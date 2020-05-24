
import os,sys
import unittest
from PIL import Image

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
		url="tmp/empty3d.idx"
		dtype="uint8[3]"
		width,height,depth=1025,512,256
		field=Field("data",dtype,"row_major")
		PyDataset.Create(url=url,dims=[width,height,depth],fields=[field])

	def testCreate2dDatasetFromNumPy(self):
		url="tmp/data2d_from_numpy.idx"
		data=numpy.asarray(Image.open('datasets/cat/rgb.png'))
		PyDataset.Create(url=url, dim=2,data=data)
		
	def testCreate3dDatasetFromNumPy(self):
		url="tmp/data3d_from_numpy.idx"
		data=numpy.zeros((100,100,100,3),dtype=numpy.float32) # depth,height,width,nchannels
		PyDataset.Create(url=url, dim=3, data=data)
		
	def testCreate3dDatasetFromSlices(self):
			url="tmp/data3d_from_slices.idx"
			width, height,depth=256,256,10
			fields=[Field("data","uint8[3]","row_major")]
			PyDataset.Create(url=url,dims=[width,height,depth],fields=fields)
			db=PyDataset(url)

			cat=numpy.asarray(Image.open('datasets/cat/rgb.png'))

			def generateSlices():
				for I in range(depth): 
					yield cat

			db.writeSlabs(generateSlices(),z=0)
			
			# read a slice in the middle
			middle=int(depth/2)
			check=next(db.read(z=[middle, middle+1]))
			Assert((cat==check).all())
	
	def testCreate3dDatasetAndReadStuff(self):
		
		width,height,depth=256,256,100
		
		url="tmp/create3d_and_read_stuff.idx"
		field=Field("data","uint8[3]","row_major")
		PyDataset.Create(url=url,dims=[width,height,depth],fields=[field])
		
		db=PyDataset(url)
		print(db.getDatasetBody().toString())
			
		# write first slice at offset z=0
		cat=numpy.asarray(Image.open('datasets/cat/rgb.png'))
		db.write(cat,z=0)
		
		# read back slice at z=0
		check=next(db.read(z=[0,1]))
		Assert((cat==check).all())
		
		# read slice at z=0 with 3 refinements (coarse to fine)
		for I,data in enumerate(db.read(z=[0,1],num_refinements=3)):
			level=data[0,:,:]
			Assert(I<2 or (cat==level).all())


# ///////////////////////////////////////////////////////////
if __name__ == '__main__':
	unittest.main(verbosity=2,exit=True)