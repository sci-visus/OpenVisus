
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
def CreateIdxDataset(url, DIMS=None, dtype=None, blocksize=0, default_layout="rowmajor",default_compression=""):

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
def CreateFullRes(url, DIMS=None, dims=None, dtype=None):
	samplesize=dtype.getByteSize()
	blocksize=dims[0]*dims[1]*dims[2]*samplesize
	if os.path.exists(url): os.remove(url)
	file=File()
	Assert(file.createAndOpen(url, "w"))
	t1 = Time.now()
	cursor=0
	nx=int(DIMS[0]/dims[0])
	ny=int(DIMS[1]/dims[1])
	nz=int(DIMS[2]/dims[2])
	nblocks=nx*ny*nz
	for block_id in range(nblocks):
		buffer=GenerateRandomData(dims, dtype)
		array=Array.fromNumPy(buffer, bShareMem=True)
		file.write(cursor, blocksize, array.c_ptr())
		cursor+=array.c_size()
		#print(block_id,nblocks)
	file.close()


# /////////////////////////////////////////////////////
def ReadFullRes(url,DIMS, dims,dtype):
	file=File()
	Assert(file.open(url, "r"))
	nx=int(DIMS[0]/dims[0])
	ny=int(DIMS[1]/dims[1])
	nz=int(DIMS[2]/dims[2])
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
def ReadIdxBlocks(url):
	db=PyDataset(url)
	access = IdxDiskAccess.create(db)
	access.disableAsync()
	access.beginRead()
	nblocks=db.getTotalNumberOfBlocks()
	while True:
		block_id=np.random.randint(0,nblocks)
		data=db.readBlock(block_id, access=access)
		Assert(data is not None)
		yield (data,data.nbytes)
	access.endRead()

# ////////////////////////////////////////////////////////////////
def ReadIdxFullRes(url, dims):
	db=PyDataset(url)
	DIMS=db.getLogicSize()
	access = IdxDiskAccess.create(db)
	access.disableAsync()
	access.beginRead()
	samplesize=db.getDefaultField().dtype.getByteSize()
	while True:
		x=np.random.randint(0,DIMS[0]/dims[0])*dims[0]
		y=np.random.randint(0,DIMS[1]/dims[1])*dims[1]
		z=np.random.randint(0,DIMS[2]/dims[2])*dims[2]
		BlockQuery.global_stats().resetStats()
		data=db.read(logic_box=[(x,y,z),(x+dims[0],y+dims[1],z+dims[2])],access=access)
		Assert(data.nbytes==dims[0]*dims[1]*dims[2]*samplesize)
		N=BlockQuery.global_stats().getNumRead()
		# print(N)
		yield (data,data.nbytes)
	access.endRead()


# ////////////////////////////////////////////////////////////////
def TimeIt(name, gen, max_seconds=60):
	print("Starting",name,"...")
	np.random.seed()
	data, needed=next(gen) # skip any headers (open/close file)
	NEEDED, DISK, DONE, T1=0,0,0,Time.now()
	while T1.elapsedSec()<max_seconds:
		File.global_stats().resetStats()
		data, needed=next(gen)
		NEEDED+=needed
		DISK+=File.global_stats().getReadBytes()
		DONE+=1
	SEC=T1.elapsedSec()
	print(name,"done","Needed KB/sec","{:0.2f}".format(NEEDED/(SEC*1024)),"Disk MB/sec","{:0.2f}/sec".format(DISK/(SEC*1024*1024)),"Disk/Needed","{:0.2f}".format(DISK/NEEDED))

# ////////////////////////////////////////////////////////////////
def Main():

	dtype=DType.fromString("uint16")
	DIMS=(4096,4096,4096)

	samplesize=dtype.getByteSize()
	totvoxels=DIMS[0]*DIMS[1]*DIMS[2]
	print("DIMS",DIMS)
	print("totvoxels",BSize(totvoxels))
	print("database size",BSize(totvoxels*samplesize))

	if False:
		CreateFullRes("D:/tmp/test_speed/fullres_008k.bin",DIMS=DIMS, dims=(16,16,16), dtype=dtype)
		CreateFullRes("D:/tmp/test_speed/fullres_064k.bin",DIMS=DIMS, dims=(32,32,32), dtype=dtype)

		CreateIdxDataset("D:/tmp/test_speed/128k.idx", DIMS=DIMS, dtype=dtype, blocksize=128*1024)
		CreateIdxDataset("D:/tmp/test_speed/064k.idx", DIMS=DIMS, dtype=dtype, blocksize= 64*1024)
		CreateIdxDataset("D:/tmp/test_speed/032k.idx", DIMS=DIMS, dtype=dtype, blocksize= 32*1024)
		CreateIdxDataset("D:/tmp/test_speed/016k.idx", DIMS=DIMS, dtype=dtype, blocksize= 16*1024)
		CreateIdxDataset("D:/tmp/test_speed/008k.idx", DIMS=DIMS, dtype=dtype, blocksize=  8*1024)

	if True:
		#TimeIt("fullres-008k",      ReadFullRes("D:/tmp/test_speed/fullres_008k.bin", DIMS, (16,16,16) , dtype))
		#TimeIt("fullres-064k",      ReadFullRes("D:/tmp/test_speed/fullres_064k.bin", DIMS, (32,32,32) , dtype))

		TimeIt("idx-blocks-128k",   ReadIdxBlocks("D:/tmp/test_speed/128K.idx"))
		TimeIt("idx-blocks-064k",   ReadIdxBlocks("D:/tmp/test_speed/064k.idx"))
		TimeIt("idx-blocks-032k",   ReadIdxBlocks("D:/tmp/test_speed/032k.idx"))
		TimeIt("idx-blocks-016k",   ReadIdxBlocks("D:/tmp/test_speed/016k.idx"))
		TimeIt("idx-blocks-008k",   ReadIdxBlocks("D:/tmp/test_speed/008k.idx"))

		TimeIt("idx-query-128k-16", ReadIdxFullRes("D:/tmp/test_speed/128K.idx", (16,16,16)))
		TimeIt("idx-query-064k-16", ReadIdxFullRes("D:/tmp/test_speed/064k.idx", (16,16,16)))
		TimeIt("idx-query-032k-16", ReadIdxFullRes("D:/tmp/test_speed/032k.idx", (16,16,16)))
		TimeIt("idx-query-016k-16", ReadIdxFullRes("D:/tmp/test_speed/016k.idx", (16,16,16)))
		TimeIt("idx-query-008k-16", ReadIdxFullRes("D:/tmp/test_speed/008k.idx", (16,16,16)))

		TimeIt("idx-query-128k-32", ReadIdxFullRes("D:/tmp/test_speed/128K.idx", (32,32,32)))
		TimeIt("idx-query-064k-32", ReadIdxFullRes("D:/tmp/test_speed/064k.idx", (32,32,32)))
		TimeIt("idx-query-032k-32", ReadIdxFullRes("D:/tmp/test_speed/032k.idx", (32,32,32)))
		TimeIt("idx-query-016k-32", ReadIdxFullRes("D:/tmp/test_speed/016k.idx", (32,32,32)))
		TimeIt("idx-query-008k-32", ReadIdxFullRes("D:/tmp/test_speed/008k.idx", (32,32,32)))

	print("all done")
	sys.exit(0)


# ////////////////////////////////////////////////////////////////
if __name__=="__main__":
	Main()



