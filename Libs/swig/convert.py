import os,time,glob,math,sys,shutil,shlex,subprocess,datetime,argparse,zlib,glob,struct,argparse
from multiprocessing.pool import ThreadPool
from threading import Thread

from OpenVisus import *

# ////////////////////////////////////////////////////////////////////////////////////////
"""

  IDX File format

  FileHeader == Uint32[10]== all zeros (not used)

  class BlockHeader
  {
    Uint32  prefix_0    = 0; //not used
    Uint32  prefix_1    = 0; 
    Uint32  offset_low  = 0;
    Uint32  offset_high = 0;
    Uint32  size        = 0;
    Uint32  flags       = 0;
    Uint32  suffix_0    = 0; //not used
    Uint32  suffix_1    = 0; //not used
    Uint32  suffix_2    = 0; //not used
    Uint32  suffix_3    = 0; //not used
	};

  enum
  {
    NoCompression = 0,
    ZipCompression = 0x03,
    JpgCompression = 0x04,
    //ExrCompression =0x05,
    PngCompression = 0x06,
    Lz4Compression = 0x07,
    ZfpCompression = 0x08,    
    CompressionMask = 0x0f
  };

  enum
  {
    FormatRowMajor = 0x10
  };
"""




# ////////////////////////////////////////////////////////////////////////////////////////
def NormalizeArcoArg(value):
	
	"""
	Example: modvisus | 0 | 1mb | 512kb
	"""

	if value is None or value=="modvisus":
		return 0
	
	if isinstance(value,str):
		return StringUtils.getByteSizeFromString(value)  
	
	assert(isinstance(value,int))
	return int(value)


# ////////////////////////////////////////////////////////////////////////////////////////
def CreateAccess(db,for_writing=False):

	"""
	this function will create a local access if the *.idx file is local (e.g. IdxDiskAccess or DiskAccess)
	if the *.idx is remote will guess if to create a ModVisusAccess or a CloudStorageAccess

	see Dataset::createAccess
	"""
	ret=db.createAccessForBlockQuery()

	# important for writing to disable compression otherwise I will spend most of the time to compress/uncompress
	# blocks during the writing (remember that convert may need to write the same block several times to interleave samples from different levels)
	if for_writing:
		ret.disableWriteLock()
		ret.disableCompression()
	return ret


# /////////////////////////////////////////////////////////////////////////////
class CopyBlocks:

	# constructor
	def __init__(self,src,dst, time=None, field=None, num_read_per_request=1, verbose=False):
		self.src=src
		self.dst=dst
		self.time =time   if time is not None and time !="" else src.getTime()
		self.field=field if field is not None and field!="" else src.getField()
		self.verbose=verbose

		# for mod_visus aggregation (do N block requests in one network request)
		self.num_read_per_request=num_read_per_request 

		# statistics
		self.t1=Time.now()
		self.ndone=0
		self.tot=0

	# advance
	def advance(self,num):
		self.ndone+=num
		sec=self.t1.elapsedSec()
		if sec>1:
			perc=100.0*self.ndone/float(self.tot)
			print("# *** Progress {:.2f}% {}/{}".format(perc,self.ndone,self.tot))
			self.t1=Time.now()

	# writeBlocks
	def writeBlocks(self, waccess, read_blocks):

		src,dst=self.src,self.dst

		for read_block in read_blocks:
			read_block.waitDone()

			# could be that the block is not stored
			if not read_block.ok(): 
				continue

			# default is to change the layout to rowmajor
			src.convertBlockQueryToRowMajor(read_block)

			buffer= Array.toNumPy(read_block.buffer, bShareMem=False)

			# I don't care if it fails????
			write_ok=dst.writeBlock(read_block.blockid, field=read_block.field.name, time=read_block.time,access=waccess, data=buffer)
			
			if self.verbose or not write_ok:
				if not write_ok: print("ERROR",end=' ')
				print(f"writeBlock time({read_block.time}) field({read_block.field.name}) blockid({read_block.blockid}) write_ok({write_ok})")

	# doCopy
	def doCopy(self, A, B):

		self.tot+=(B-A)
		src,dst=self.src,self.dst
		print(f"Copying blocks time({self.time}) field({self.field.name}) A({A}) B({B}) ...")

		waccess=CreateAccess(dst, for_writing=True)

		# for mod_visus there is the problem of aggregation (Ii.e. I need to call endRead to force the newtork request)
		if "mod_visus" in src.getUrl():
			num_read_per_request=self.num_read_per_request 
			# mov_visus access will not call itself the flush batch, but will wait for endRead()
			raccess=src.createAccessForBlockQuery(StringTree.fromString("<access type='ModVisusAccess' chmod='r'  compression='zip' nconnections='1'  num_queries_per_request='1073741824' />"))
		else:
			num_read_per_request=1
			raccess=CreateAccess(src,for_writing=False)

		read_blocks=[]
		aborted=Aborted()
		raccess.beginRead()
		waccess.beginWrite()
		for blockid in range(A,B):

			read_block = src.createBlockQuery(blockid, self.field, self.time, ord('r'),aborted)
			src.executeBlockQuery(raccess, read_block)
			read_blocks.append(read_block)

			if len(read_blocks)>=num_read_per_request or blockid==(B-1):
				raccess.endRead() 
				self.writeBlocks(waccess, read_blocks)
				raccess.beginRead()
				self.advance(len(read_blocks))
				read_blocks=[]
			
		Assert(len(read_blocks)==0)
		raccess.endRead()
		waccess.endWrite()

	@staticmethod
	def Main(args):
		print("CopyBlocks", "Got args",args)
		parser = argparse.ArgumentParser(description="Copy blocks")
		parser.add_argument("--src","--source", type=str, help="source", required=True,default="")
		parser.add_argument("--dst","--destination", type=str, help="destination", required=True,default="") 
		parser.add_argument("--field", help="field"  , required=False,default="") 
		parser.add_argument("--time", help="time", required=False,default="") 
		parser.add_argument("--num-threads", help="number of threads", required=False,type=int, default=4)
		parser.add_argument("--num-read-per-request", help="number of read block per network request (only for mod_visus)", required=False,type=int, default=256)
		parser.add_argument("--verbose", help="Verbose", required=False,action='store_true') 
		args = parser.parse_args(args)

		# load src and destination dataset
		src=LoadDataset(args.src); Assert(src)

		# can in which destination  *.idx does not exist
		if not os.path.isfile(args.dst):
			src.db.idxfile.createNewOne(args.dst)
		dst=LoadDataset(args.dst)

		copier=CopyBlocks(src,dst,time=args.time,field=args.field,num_read_per_request=args.num_read_per_request, verbose=args.verbose)

		tot_blocks=src.getTotalNumberOfBlocks()

		nthreads=args.num_threads
		num_per_thread=tot_blocks//nthreads
		print("tot_blocks",tot_blocks,"nthreads",nthreads,"num_per_thread",num_per_thread)

		if nthreads<=1:
			copier.doCopy(0,tot_blocks)
		else:
			threads=[]
			for I in range(nthreads):
				A=(I+0)*num_per_thread
				B=(I+1)*num_per_thread if I<(nthreads-1) else tot_blocks
				threads.append(Thread(target=CopyBlocks.doCopy, args=(copier,A,B)))
			for t in threads: t.start()
			for t in threads: t.join()




# ////////////////////////////////////////////////////////////////////////////////////////
def CompressDataset(idx_filename:str=None, compression="zip", num_threads=32, level=-1):

	T1=time.time()
	UNCOMPRESSED_SIZE,COMPRESSED_SIZE=0,0

	db=LoadIdxDataset(idx_filename)
	if db.idxfile.arco:

		def CompressArcoBlock(filename):
			nonlocal UNCOMPRESSED_SIZE,COMPRESSED_SIZE
			with open(filename,"rb") as f: mem=f.read()	
			if compression:
				UNCOMPRESSED_SIZE+=len(mem)
				mem=zlib.compress(mem,level=level) # https://docs.python.org/3/library/zlib.html#zlib.compress
				COMPRESSED_SIZE+=len(mem)
			else:
				COMPRESSED_SIZE+=len(mem)
				mem=zlib.decompress(mem) 
				UNCOMPRESSED_SIZE+=len(mem)

			ReplaceFile(filename, mem)
			print(f"CompressArcoBlock done {filename}")
			del mem
	
		# assuming *.bin files are in the dir of the *.idx (please keep it in mind!)
		filename_pattern=os.path.join(os.path.dirname(idx_filename),"**/*.bin")
		p=ThreadPool(num_threads)
		p.map(CompressArcoBlock, glob.glob(filename_pattern,recursive=True))

	else:

		# modvisus compress or decompress
		CompressionMask, ZipMask=0x0f,0x03
		idx=db.idxfile
		blocks_per_file=idx.blocksperfile
		num_fields=idx.fields.size()
		file_header_size=10*4
		block_header_size=10*4
		tot_blocks=blocks_per_file*num_fields
		header_size=file_header_size+block_header_size*tot_blocks

		# doing compression at file level
		for filename in glob.glob(filename_pattern,recursive=True):

			t1=time.time()
			with open(filename,"rb") as f:
				mem=f.read()

			# read the header
			v=[struct.unpack('>I',mem[cur:cur+4])[0] for cur in range(0,header_size,4)]; assert(len(v)% 10==0)
			v=[v[I:I+10] for I in range(0,len(v),10)]
			file_header  =v[0]
			block_headers=v[1:]
			assert file_header==[0]*10
			full_size=header_size

			def CompressModVisusBlock(B):
				block_header=block_headers[B]
				field_index=B // blocks_per_file
				blocksize=idx.fields[field_index].dtype.getByteSize(2**idx.bitsperblock)
				assert block_header[0]==0 and block_header[1]==0 and block_header[6]==0 and block_header[7]==0 and block_header[8]==0 and block_header[9]==0
				offset=(block_header[2]<<0)+(block_header[3]<<32)
				size  =block_header[4]
				flags =block_header[5]
				block=mem[offset:offset+size]
				flags_compression=flags & CompressionMask
				if flags_compression == ZipMask:
					decompressed=zlib.decompress(block)
				else:
					assert(flags_compression==0)
					decompressed=block
				assert len(decompressed)==blocksize
				nonlocal full_size
				full_size+=blocksize

				if compression=="zip":
					compressed=zlib.compress(decompressed,level=level) # https://docs.python.org/3/library/zlib.html#zlib.compress
				else:
					compressed=decompressed # uncompress

				return (B,compressed)

			# compress blocks in parallel
			p=ThreadPool(num_threads)
			COMPRESSED=p.map(CompressModVisusBlock, range(tot_blocks))
			COMPRESSED=sorted(COMPRESSED, key=lambda tup: tup[0])

			compressed_size=header_size+sum([len(it[1]) for it in COMPRESSED])
			mem=bytearray(mem[0:compressed_size])

			# write file header
			mem[0:40]=struct.pack('>IIIIIIIIII',0,0,0,0,0,0,0,0,0,0)

			# fill out new mem
			offset=header_size
			for I in range(tot_blocks):
				compressed=COMPRESSED[I][1]
				size=len(compressed)
				# block header
				header_offset=file_header_size+I*40
				mem[header_offset:header_offset+40]=struct.pack('>IIIIIIIIII',0,0,(offset & 0xffffffff),(offset>>32),size,(block_headers[I][5] & ~CompressionMask) | ZipMask,0,0,0,0)
				mem[offset:offset+size]=compressed; offset+=size
			assert offset==compressed_size

			ReplaceFile(filename, mem)
			del mem

			ratio=int(100*compressed_size/full_size)
			UNCOMPRESSED_SIZE+=full_size
			COMPRESSED_SIZE+=compressed_size
			print(f"Compressed {filename}  in {time.time()-t1} ratio({ratio}%)")

	RATIO=int(100*COMPRESSED_SIZE/UNCOMPRESSED_SIZE)
	print(f"CompressDataset done in {time.time()-T1} seconds {RATIO}%")


# ////////////////////////////////////////////////////////////////////////
def ConvertImageStack(src:str, dst:str, arco="modvisus"):

	T1=time.time()

	# dst must be an IDX file
	assert os.path.splitext(dst)[1]==".idx" 

	arco=NormalizeArcoArg(arco)

	print(f"ConvertImageStack src={src} dst={dst} arco={arco}")

	# get filenames
	if True:
		objects=sorted(glob.glob(src,recursive=True))
		tot_slices=len(objects)
		print(f"ConvertLocalImageStack tot_slices {tot_slices} src={src}...")
		if tot_slices==0:
			error_msg=f"Cannot find images inside {src}"
			print(error_msg)
			raise Exception(error_msg)	

	# guess 3d volume size and dtype
	if True:
		import imageio
		first=imageio.imread(objects[0]) # the first image it's needed for the shape and dtype
		depth=tot_slices
		height=first.shape[0]
		width=first.shape[1]
		nchannels=first.shape[2] if len(first.shape)>=3 else 1
		dtype=f'{first.dtype}[{nchannels}]'
	
	# this is the field
	field=Field('data',dtype,'row_major')

	# one-file==one-block (NOTE: arco is the maximum blocksize, it will be less for power-of-two block aligment, for example in case of RGB data)
	# for example arco=1mb, samplesperblock=256k blocksize=256*3=756kb<1mb
	if arco:
		bitsperblock = int(math.log2(arco // field.dtype.getByteSize())) 
	else:
		bitsperblock=16

	samples_per_block=2**bitsperblock
	blocksize=field.dtype.getByteSize(samples_per_block)

	def Generator():
		import imageio
		for I,filename in enumerate(objects): 
			print(f"{I}/{tot_slices} Reading image file {filename}")
			yield imageio.imread(filename)

	generator=Generator()

	db=CreateIdx(
		url=dst, 
		dims=[width,height,depth],
		fields=[field],
		bitsperblock=bitsperblock,
		arco=arco)

	assert(db.getMaxResolution()>=bitsperblock)
	access=CreateAccess(db, for_writing=True)
	db.writeSlabs(generator, access=access)

	print(f"ConvertImageStack DONE in {time.time()-T1} seconds")


# ////////////////////////////////////////////////////////////////////////////
def CopyDataset(src:str,dst:str,arco="modvisus", tile_size=None):

	arco=NormalizeArcoArg(arco)

	print(f"CopyDataset {src} {dst} arco={arco}")
	T1=time.time()
	SRC=LoadIdxDataset(src)
	
	Dfields=[]
	for Sfield in SRC.getFields():
		Sfield=SRC.getField(Sfield)
		Dfields.append(Field(Sfield.name,Sfield.dtype,'row_major'))

	# todo: multiple fields, but I have the problem of fields with different dtype will lead to different bitsperblock, how to solve?
	if arco:
		assert len(SRC.getFields())==1
		Dbitsperblock = int(math.log2(arco // Dfields[0].dtype.getByteSize())) 
	else:
		Dbitsperblock=16

	# todo other cases
	timesteps=[it for it in SRC.getTimesteps().asVector()]
	dims=[int(it) for it in SRC.getLogicSize()]

	DST=CreateIdx(
		url=dst, 
		dims=dims,
		time=[timesteps[0],timesteps[-1],"%00000d/"],
		fields=Dfields,
		bitsperblock=Dbitsperblock,
		arco=arco)

	assert(DST.getMaxResolution()>=Dbitsperblock)

	Saccess=CreateAccess(SRC, for_writing=False) 
	Daccess=CreateAccess(DST, for_writing=True) 

	pdim=SRC.getPointDim()
	assert pdim==2 or pdim==3 # TODO other cases

	# copy
	if tile_size is None:
		tile_size=1024 if pdim==2 else 512

	piece=[tile_size, tile_size,1 if pdim==2 else tile_size] 
	W,H,D=[dims[0]  , dims[1]  ,1 if pdim==2 else dims[2]  ]

	def pieces():
		for timestep in timesteps:
			for fieldname in SRC.getFields():
				for z1 in range(0,D,piece[2]):
					for y1 in range(0,H,piece[1]):
						for x1 in range(0,W,piece[0]):
							x2=min(x1+piece[0],W)
							y2=min(y1+piece[1],H)
							z2=min(z1+piece[2],D) 
							logic_box=BoxNi(PointNi(x1,y1,z1),PointNi(x2,y2,z2))
							logic_box.setPointDim(pdim)
							yield timestep,fieldname, logic_box

	N=len([logic_box for logic_box in pieces()])
	Saccess.beginRead()
	Daccess.beginWrite()
	for I,(timestep,fieldname,logic_box) in enumerate(pieces()):
		t1=time.time()
	
		data=SRC.read( logic_box=logic_box,time=timestep,field=SRC.getField(fieldname),access=Saccess)
		DST.write(data,logic_box=logic_box,time=timestep,field=DST.getField(fieldname)	,access=Daccess)
		sec=time.time()-t1
		print(f"Wrote {I}/{N} {logic_box.toString()} time({timestep}) field({Sfield.name}) in {sec:.2f} seconds")
	Saccess.endRead()
	Daccess.endWrite()

	print(f"CopyDataset src={src} dst={dst} done in {time.time()-T1:.2f} seconds")
