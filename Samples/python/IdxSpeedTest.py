
# this example test the speed of IDX 
import os,sys,math, numpy as np
from OpenVisus import *

# ////////////////////////////////////////////////////////////////
def BSize(value):
	return StringUtils.getStringFromByteSize(int(value))

# ////////////////////////////////////////////////////////////////
def GenerateRandomData(dims, dtype):
	"""
	half data will be zero, half will be random
	in case you want to test with compression having a 50% compression ratio
	"""
	
	W,H,D=dims
	ret=np.zeros((D, H, W),dtype=convert_dtype(dtype.get(0).toString()))

	if ret.dtype==numpy.float32 or ret.dtype==numpy.float64:
		ret[0:int(D/2),:,]=np.random.rand(int(D/2), H, W,dtype=ret.dtype)
	else:
		ret[0:int(D/2),:,]=np.random.randint(0, 65535, (int(D/2), H, W),dtype=ret.dtype)
	return ret	

# ////////////////////////////////////////////////////////////////
def CreateIdxDataset(url, DIMS, dtype, blocksize=0, default_layout="rowmajor",default_compression=""):

	totvoxels=DIMS[0]*DIMS[1]*DIMS[2]
	samplessize=dtype.getByteSize()
	samplesperblock=int(blocksize/samplessize)
	bitsperblock=int(math.log(samplesperblock,2))
	nblocks=int(totvoxels/samplesperblock)
	blocksperfile=nblocks # I want only one file containing all blocks (to avoid fopen/fclose slowness)
	
	print("blocksize",BSize(blocksize))
	print("samplesperblock",samplesperblock)
	print("bitsperblock",bitsperblock)
	print("nblocks",nblocks)
	print("blocksperfile",blocksperfile)

	field=Field("data")
	field.dtype=dtype
	field.default_layout=default_layout
	field.default_compression=default_compression
	CreateIdx(url=url,dims=DIMS,fields=[field], bitsperblock=bitsperblock, blocksperfile=blocksperfile, filename_template="./" + os.path.basename(url)+".bin")

	# fill with fake data
	db=LoadDataset(url)
	field=db.getDefaultField()
	time=db.getDefaultTime()
	access = IdxDiskAccess.create(db)
	access.disableAsync()
	access.disableWriteLock()
	access.beginWrite()
	nblocks=db.getTotalNumberOfBlocks()
	for block_id in range(nblocks):
		write_block = BlockQuery(db, field, time, access.getStartAddress(block_id), access.getEndAddress(block_id), ord('w'), Aborted())
		nsamples=write_block.getNumberOfSamples().toVector()
		buffer=GenerateRandomData(nsamples,dtype)
		write_block.buffer=Array.fromNumPy(buffer, bShareMem=True)
		db.executeBlockQueryAndWait(access, write_block)
		Assert(write_block.ok())
		if block_id % 300 ==0:
			print(block_id,"{}%".format(int(100*block_id/nblocks)))
	access.endWrite()	

# /////////////////////////////////////////////////////
def CreateFullRes(url, DIMS, dims, dtype):
	if os.path.exists(url): os.remove(url)
	file=File()
	Assert(file.createAndOpen(url, "w"))
	t1 = Time.now()
	cursor=0
	nx=DIMS[0]/dims[0]
	ny=DIMS[1]/dims[1]
	nz=DIMS[2]/dims[2]
	nblocks=nx*ny*nz
	for block_id in range(nblocks):
		buffer=GenerateRandomData(dims, dtype)
		array=Array.fromNumPy(buffer, bShareMem=True)
		cursor+=array.c_size()
		print(block_id,nblocks)
	file.close()


# /////////////////////////////////////////////////////
def ReadFullRes(url,DIMS, dims,dtype):
	file=File()
	Assert(file.open(url, "r"))
	nx=DIMS[0]/dims[0]
	ny=DIMS[1]/dims[1]
	nz=DIMS[2]/dims[2]
	nblocks=nx*ny*nz
	samplesperblock=dims[0]*dims[1]*dims[2]
	samplesize=dtype.getByteSize()
	blocksize=samplesperblock*samplesize
	while True:
		array=Array(PointNi(dims[0],dims[1],dims[2]), dtype)
		blockid=np.random.randint(0,nblocks)
		file.read(blockid*blocksize,array.c_size(),array.c_ptr())
		yield (array,array.c_size())
	file.close()

# ////////////////////////////////////////////////////////////////
def ReadIdx(url, DIMS, dims, dtype):
	db=PyDataset(url)
	access = IdxDiskAccess.create(db)
	access.disableAsync()
	access.beginRead()
	samplesize=dtype.getByteSize()
	while True:
		x=np.random.randint(0,DIMS[0]/dims[0])*dims[0]
		y=np.random.randint(0,DIMS[1]/dims[1])*dims[1]
		z=np.random.randint(0,DIMS[2]/dims[2])*dims[2]
		BlockQuery.global_stats().resetStats()
		data=next(db.read(logic_box=[(x,y,z),(x+dims[0],y+dims[1],z+dims[2])],access=access))
		Assert(data.nbytes==dims[0]*dims[1]*dims[2]*samplesize)
		N=BlockQuery.global_stats().getNumRead()
		# print(N)
		yield (data,data.nbytes)
	access.endRead()


# ////////////////////////////////////////////////////////////////
def TimeIt(gen):
	np.random.seed()

	# skip any headers (open/close file)
	data, good=next(gen)

	T1=Time.now()
	GOOD, DISK,DONE,t1=0,0,0,Time.now()

	for I in range (1024*1024):
		File.global_stats().resetStats()
		data, good=next(gen)
		disk=File.global_stats().getReadBytes()

		GOOD, DISK, DONE=GOOD+good, DISK+disk, DONE+1
		sec,SEC=t1.elapsedSec(),T1.elapsedSec()
		if sec>5:
			print("GOOD({}kb/sec)".format(int(GOOD/(1024*SEC))),"DISK({}/sec)".format(BSize(DISK/SEC)),"GOOD",GOOD,"DISK",DISK,"%{}".format(int(100*GOOD/DISK)),"{}x".format(int(DISK/GOOD)))
			t1=Time.now()

# ////////////////////////////////////////////////////////////////
def Main():

	fullres_url="D:/tmp/test_speed/fullres.bin"
	dtype=DType.fromString("uint16")
	DIMS=(4096,4096,4096)
	dims=(32,32,32) 

	totvoxels=DIMS[0]*DIMS[1]*DIMS[2]
	samplesize=dtype.getByteSize()
	print("DIMS",DIMS)
	print("dims",dims)
	print("totvoxels",BSize(totvoxels))
	print("database size",BSize(totvoxels*samplesize))
	print("samplesize",samplesize)

	# CreateFullRes(fullres_url,DIMS, dims, dtype)
	# CreateIdxDataset("D:/tmp/test_speed/64k.idx",DIMS, dtype, blocksize=int((dims[0]*dims[1]*dims[2] * samplesize)/1))
	# CreateIdxDataset("D:/tmp/test_speed/32k.idx",DIMS, dtype, blocksize=int((dims[0]*dims[1]*dims[2] * samplesize)/2))
	# CreateIdxDataset("D:/tmp/test_speed/16k.idx",DIMS, dtype, blocksize=int((dims[0]*dims[1]*dims[2] * samplesize)/4))
	CreateIdxDataset("D:/tmp/test_speed/8k.idx",DIMS, dtype, blocksize=int((dims[0]*dims[1]*dims[2] * samplesize)/8))
	
	# TimeIt(ReadFullRes(fullres_url, DIMS, dims, dtype))
	# TimeIt(ReadIdx("D:/tmp/test_speed/64k.idx", DIMS, dims, dtype))
	# TimeIt(ReadIdx("D:/tmp/test_speed/32k.idx", DIMS, dims, dtype))
	# TimeIt(ReadIdx("D:/tmp/test_speed/16k.idx", DIMS, dims, dtype))
	# TimeIt(ReadIdx("D:/tmp/test_speed/8k.idx", DIMS, dims, dtype))

	print("all done")
	sys.exit(0)


# ////////////////////////////////////////////////////////////////
if __name__=="__main__":
	Main()



