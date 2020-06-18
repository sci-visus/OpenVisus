from OpenVisus import *
from PIL import Image
import unittest

# example: ((rrr...),(ggg...),(bbb...)) -> (rgb rgb rgb ...)
def InterleaveChannels(channels):
	nchannels=len(channels)
	first=channels[0]
	ret=numpy.zeros(list(first.shape) + [nchannels],first.dtype)
	for I, channel in enumerate(channels):
		ret[:,:,I]=channel
	return ret
	
# example (ffff) -> (f0 f0 f0...)
def AddChannel(value,num=1):
	extra=numpy.zeros(value.shape,value.dtype)
	return InterleaveChannels([value] + [extra]*num)
	
# OpenImage
def OpenImage(url):
	return numpy.asarray(Image.open(url))	
	
# save image
def SaveImage(url,data, bSeparateChannels=False):
	
	if bSeparateChannels:
		N=data.shape[2]
		for I in range(N):
			base,ext=os.path.splitext(url)
			SaveImage(base+".{0}".format(I)+ext,data[:,:,I])
		
	else:
		# print("saving image",url)
		Image.fromarray(data).save(url)

# NormalizeData
def NormalizeData(data):
	data.astype(numpy.float32)
	return (data - numpy.min(data))/numpy.ptp(data)


# //////////////////////////////////////////////////////////
def TestFilters(img_filename, filter):

	# this is the original image
	noise=OpenImage(img_filename)

	temp_dir="tmp/test_minmax"

	# create source dataset for comparison purpouse
	src_db=CreateIdx(url=temp_dir + '/src_db/visus.idx', rmtree=True, dim=2, data=noise)
	SaveImage(temp_dir + "/src.full.tif", src_db.read())

	# read data coarse to fine
	num_refinements=5
	src_levels=[it for it in src_db.read(num_refinements=num_refinements)]
	for I,src in enumerate(src_levels):
		SaveImage(temp_dir + "/src.{0}.{1}.tif".format(filter,I),src)

	# create filter_level dataset
	# in order to compute filters I need to have 2 channels (i.e. A B C D -> A0 B0 C0 D0 )
	filter_db=CreateIdx(url=temp_dir + '/filter_db/visus.idx', rmtree=True, dim=src_db.getPointDim(), data=AddChannel(noise), filters=[filter])
	
	# compute the filter fine to coarse
	filter_db.computeFilter(filter_db.getField(),4096)
	
	SaveImage(temp_dir + "/{0}.full.tif".format(filter), filter_db.read()[:,:,0])

	# read filtered data 
	filter_levels=list(filter_db.read(num_refinements=num_refinements))
	m0,M0=None,None
	for I,(src_level,filter_level) in enumerate(zip(src_levels,filter_levels)):
		
		# drop second channel (used for coefficients computation)
		filter_level=filter_level[:,:,0]
		SaveImage(temp_dir + "/reconstructed.{0}.{1}.tif".format(filter,I),filter_level)
		
		# check that the image is reconstructed just fine
		if filter=="identity" or I==num_refinements-1:
			Assert((src_level==filter_level).any())	
		
		# compute range
		m,M=[numpy.min(filter_level),numpy.max(filter_level)]
		
		if I>0:
			
			# min should decrease, max should increase
			if filter=="identity":
				Assert(m<=m0 and M>=M0)		
				
			# min should decrease from coarse to fine, max should be constant from coarse to fine
			if filter=="max":
				Assert(m<=m0 and M==M0)	
				
			# min should be constant from coarse to fine, max should increase from coarse to fine
			if filter=="min":
				Assert(m==m0 and M>=M0)
			
		m0,M0=(m,M)

	# this is the data without the filter applied (i.e. how it is stored on disk)
	filter_channels=filter_db.read(disable_filters=True)
	SaveImage(temp_dir + "/disk.{0}.tif".format(filter),filter_channels, bSeparateChannels=True)	
	


# ////////////////////////////////////////////////////////////////////
class MyTestCase(unittest.TestCase):
	
	def test1(argv):
		for filter in ["identity","min","max", "wavelet"]:
			TestFilters("datasets/minmax/minmax_0.png", filter)
			
	def test2(argv):
		for filter in ["identity","min","max", "wavelet"]:			
			TestFilters("datasets/minmax/minmax_1.png", filter)
			

	def test3(argv):
		for filter in ["identity","min","max", "wavelet"]:			
			TestFilters("datasets/minmax/minmax_2.png", filter)
			
	def test4(argv):
		for filter in ["identity","min","max", "wavelet"]:
			TestFilters("datasets/minmax/noise_uint16.tif", filter)
			
	def test5(argv):
		for filter in ["identity","min","max", "wavelet"]:
			TestFilters("datasets/minmax/noise_float32.tif", filter)


# /////////////////////////////////////////////////////
if __name__=="__main__":
	unittest.main(verbosity=2,exit=True)
	TestFilters("datasets/minmax/noise.tif", "wavelet")	