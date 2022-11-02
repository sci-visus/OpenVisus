import os,time,glob,math,sys,shutil,shlex,subprocess,datetime,argparse,zlib,glob,struct,argparse,logging
from multiprocessing.pool import ThreadPool
from threading import Thread

from OpenVisus import *

logger = logging.getLogger(__name__)



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
			logger.info(f"# *** Progress {perc:.2f}% {self.ndone}/{self.tot}")
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
			logger.info(f"{'ERROR' if not write_ok else 'OK'} writeBlock time({read_block.time}) field({read_block.field.name}) blockid({read_block.blockid}) write_ok({write_ok})")

	# doCopy
	def doCopy(self, A, B):

		self.tot+=(B-A)
		src,dst=self.src,self.dst
		logger.info(f"Copying blocks time({self.time}) field({self.field.name}) A({A}) B({B}) ...")

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
		logger.info(f"CopyBlocks args={args}")
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
		logger.info(f"tot_blocks={tot_blocks} nthreads={nthreads} num_per_thread={num_per_thread}")

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
def CompressArcoDataset(idx_filename:str=None, compression="zip", num_threads=32, level=-1,timestep=None,field=None):

	# ________________________________________________________________________
	def Decompress(mem):
		try:
			return zlib.decompress(mem) 
		except:
			return None

	# ________________________________________________________________________
	def CompressArcoBlock(filename):
		with open(filename,"rb") as f: raw=f.read()	
		if Decompress(raw):  
			logger.info(f"{filename} already compressed")
		else:
			encoded=zlib.compress(raw,level=level) # https://docs.python.org/3/library/zlib.html#zlib.compress
			ReplaceFile(filename, encoded)
			logger.info(f"CompressArcoBlock done {filename}")

	# ________________________________________________________________________
	def UncompressArcoBlock(filename):
		with open(filename,"rb") as f: encoded=f.read()	
		raw=Decompress(encoded)
		if not raw: 
			logger.info(f"{filename} already uncompressed")
		else:
			ReplaceFile(filename, raw)
			logger.info(f"UncompressArcoBlock done {filename}")

	# NOTE: this code is making some assumptions (like bin files should be inside the same diretory of *.idx and should have a certain patter)
	# for arco I am assuming always the same pattern 
	# /prefix/to/named/db/<basename>.idx
	# /prefix/to/named/db/<basename>/<time>/<field>/0000/.... (see DiskAccess.cpp and CloudStorageAccess.cpp)
	pattern=os.path.splitext(idx_filename)[0]
	
	if timestep is not None:
		pattern=os.path.join(pattern,str(timestep))

	if field  is not None:
		pattern=os.path.join(pattern,str(field))
	
	pattern=os.path.join(pattern,"**/*.bin")
	filenames=glob.glob(pattern,recursive=True)

	assert LoadIdxDataset(idx_filename).idxfile.arco
	T1=time.time()
	p=ThreadPool(num_threads)
	logger.info('Compressing ARCO dataset...' if compression else "Uncompressing ARCO dataset...")
	p.map(CompressArcoBlock if compression else UncompressArcoBlock, filenames)
	logger.info(f"CompressDataset done in {time.time()-T1} seconds")

# ////////////////////////////////////////////////////////////////////////////////////////
def CompressModVisusDataset(idx_filename:str=None, compression="zip", num_threads=32, level=-1,timestep=None,field=None):

	T1=time.time()
	UNCOMPRESSED_SIZE,COMPRESSED_SIZE=0,0

	db=LoadIdxDataset(idx_filename)
	assert not db.idxfile.arco
	
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
	filenames=db.getFilenames(
		timestep if timestep is not None else -1, # any negative number will work for all timesteps
		field if field is not None else ""
	)

	for filename in filenames:

		if not os.path.isfile(filename):
			continue

		t1=time.time()
		with open(filename,"rb") as f: mem=f.read()

		# read the header
		v=[struct.unpack('>I',mem[cur:cur+4])[0] for cur in range(0,header_size,4)]; assert(len(v)% 10==0)
		v=[v[I:I+10] for I in range(0,len(v),10)]
		file_header  =v[0]
		block_headers=v[1:]
		assert file_header==[0]*10
		full_size=header_size

		# ________________________________________________________________________
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

			# note: it automatically detect if it's already compressed or not, so the code should be robust
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
		logger.info(f"Compressed {filename}  in {time.time()-t1} ratio({ratio}%)")

	RATIO=int(100*COMPRESSED_SIZE/UNCOMPRESSED_SIZE)
	logger.info(f"CompressDataset done in {time.time()-T1} seconds {RATIO}%")

# ////////////////////////////////////////////////////////////////////////////////////////
def CompressDataset(idx_filename:str=None, compression="zip", num_threads=32, level=-1,timestep=None,field=None):
	db=LoadIdxDataset(idx_filename)
	if db.idxfile.arco:
		CompressArcoDataset(idx_filename,compression=compression,num_threads=num_threads,level=level,timestep=timestep,field=field)
	else:
		CompressModVisusDataset(idx_filename,compression=compression,num_threads=num_threads,level=level,timestep=timestep,field=field)

# ////////////////////////////////////////////////////////////////////////
def ConvertImageStack(src:str, dst:str, arco="modvisus"):

	T1=time.time()

	# dst must be an IDX file
	assert os.path.splitext(dst)[1]==".idx" 

	arco=NormalizeArcoArg(arco)

	logger.info(f"ConvertImageStack src={src} dst={dst} arco={arco}")

	# get filenames
	if True:
		objects=sorted(glob.glob(src,recursive=True))
		tot_slices=len(objects)
		logger.info(f"ConvertLocalImageStack tot_slices {tot_slices} src={src}...")
		if tot_slices==0:
			error_msg=f"Cannot find images inside {src}"
			logger.info(error_msg)
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
			logger.info(f"{I}/{tot_slices} Reading image file {filename}")
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

	logger.info(f"ConvertImageStack DONE in {time.time()-T1} seconds")


# ////////////////////////////////////////////////////////////////////////////
def CopyDataset(src:str, dst:str, arco="modvisus", tile_size:int=None, timestep:int=None, field:str=None,num_attempts:int=3):

	arco=NormalizeArcoArg(arco)

	logger.info(f"CopyDataset {src} {dst} arco={arco}")
	T1=time.time()
	SRC=LoadIdxDataset(src)
	
	dims=[int(it) for it in SRC.getLogicSize()]
	max_h=SRC.getMaxResolution()
	all_fields=SRC.getFields()
	all_timesteps=[int(it) for it in SRC.getTimesteps().asVector()]
	max_fieldsize=max([SRC.getField(it).dtype.getByteSize() for it in all_fields])

	timesteps_to_convert=all_timesteps if timestep is None else [int(timestep)]
	fields_to_convert   =all_fields    if field    is None else [str(field)]

	# guess bitsperblock
	bitsperblock = int(math.log2(arco // max_fieldsize)) if arco else 16
	if bitsperblock>max_h:
		bitsperblock=max_h
	assert(bitsperblock<=max_h)

	# adjust arco if needed
	arco=(2**bitsperblock)*max_fieldsize if arco else 0

	Dfields=[]
	for fieldname in all_fields:
		Sfield=SRC.getField(fieldname)
		dtype=Sfield.dtype.toString()
		Dfields.append(Field(fieldname, dtype, 'row_major'))  # force row major here

	# NOTE: I am creating a new idx with all timesteps and all fields
	DST=CreateIdx(
		url=dst, 
		dims=dims,
		time=[all_timesteps[0],all_timesteps[-1],"%00000d/"],
		fields=Dfields,
		bitsperblock=bitsperblock, 
		arco=arco
	)
	
	Saccess=CreateAccess(SRC, for_writing=False) 
	Daccess=CreateAccess(DST, for_writing=True) 

	pdim=SRC.getPointDim()
	assert pdim==2 or pdim==3 # TODO other cases


	# copy
	if tile_size is None:
		tile_size=8192 if pdim==2 else 512

	piece=[tile_size, tile_size,1 if pdim==2 else tile_size] 
	W,H,D=[dims[0]  , dims[1]  ,1 if pdim==2 else dims[2]  ]

	def pieces():
		for timestep in timesteps_to_convert:
			for fieldname in fields_to_convert:
				for z1 in range(0,D,piece[2]):
					for y1 in range(0,H,piece[1]):
						for x1 in range(0,W,piece[0]):
							x2=min(x1+piece[0],W)
							y2=min(y1+piece[1],H)
							z2=min(z1+piece[2],D) 
							logic_box=BoxNi(PointNi(x1,y1,z1),PointNi(x2,y2,z2))
							logic_box.setPointDim(pdim)
							yield timestep,fieldname, logic_box

	N=len([it for it in pieces()])
	Saccess.beginRead()
	Daccess.beginWrite()
	logger.info(f"Number of pieces {N}")
	for I,(timestep,fieldname,logic_box) in enumerate(pieces()):
		for K in range(num_attempts):
			try:
  				# read from source
				t1=time.time()
				data=SRC.read( logic_box=logic_box,time=timestep,field=SRC.getField(fieldname),access=Saccess)
				read_sec=time.time()-t1

				# write
				t1=time.time()
				DST.write(data,logic_box=logic_box,time=timestep,field=DST.getField(fieldname)	,access=Daccess)
				write_sec=time.time()-t1

				# statistics
				ETA=N*((time.time()-T1)/(I+1)) 
				logger.info(f"Wrote src={src} {I}/{N} {logic_box.toString()} timestep({timestep}) field({fieldname}) sec({read_sec:.2f}/{write_sec:.2f}/{read_sec+write_sec}) ETA_MIN({ETA/60.0:.0f})")
				break
			except:
				if K==(num_attempts-1): raise
				logger.info(f"Writing of src={src} {I}/{N} failed, retrying...")
				time.sleep(1.0)

	Saccess.endRead()
	Daccess.endWrite()

	logger.info(f"CopyDataset src={src} dst={dst} done in {time.time()-T1:.2f} seconds")


 