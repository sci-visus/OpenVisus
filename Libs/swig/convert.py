import os,time,glob,math,sys,shutil,shlex,subprocess,datetime,argparse,zlib,glob,struct,argparse,logging
from multiprocessing.pool import ThreadPool
from threading import Thread

from OpenVisus import *

logger = logging.getLogger(__name__)




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

		waccess=dst.createAccessForBlockQuery()
		waccess.setWritingMode()

		# for mod_visus there is the problem of aggregation (Ii.e. I need to call endRead to force the newtork request)
		if "mod_visus" in src.getUrl():
			num_read_per_request=self.num_read_per_request 
			# mov_visus access will not call itself the flush batch, but will wait for endRead()
			rconfig="<access type='ModVisusAccess' chmod='r'  compression='zip' nconnections='1'  num_queries_per_request='1073741824' />"
			raccess=src.createAccessForBlockQuery(StringTree.fromString(rconfig))
		else:
			num_read_per_request=1
			raccess=src.createAccessForBlockQuery()

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


# ////////////////////////////////////////////////////////////////////////////////////////
def SafeReplaceFile(filename, new_content):

	assert os.path.isfile(filename)

	temp_filename=filename + ".temp"
	
	# remove the old file
	if os.path.isfile(temp_filename): 
		os.remove(temp_filename)
	
	# rename the old file to tmp
	os.rename(filename,temp_filename)

	try:
		if callable(new_content):
			new_content()
		else:
			with open(filename,"wb") as f: 
				f.write(new_content)

		os.remove(temp_filename)

	except:
		os.rename(temp_filename,filename) # put back the old file
		raise # let outside known anyway


# ////////////////////////////////////////////////////////////////////////////////////////
def CompressArcoBlock(compression,dims,dtype,filename, use_python_zlib=False):
		
	# use the python version (working only for zip files)
	if compression=="zip" and use_python_zlib :
		with open(filename,"rb") as f: raw=f.read()	
		try:
			zlib.decompress(raw)
			logger.info(f"{filename} already compressed")
			return
		except:
			pass
		encoded=zlib.compress(raw,level=-1)
		SafeReplaceFile(filename,encoded)
		logger.info(f"CompressArcoBlock done {filename}")
	else:
		# use OpenVisus encoders
		decoded=LoadBinaryDocument(filename)

		# already encoded, do nothing
		if decoded.c_size()!=dtype.getByteSize(dims):
			logger.info(f"{filename} already compressed, skipping")
			return

		encoded = Encode(compression, dims,dtype, decoded)

		# otherwise in rehentrant code I can call twice the Encode
		assert encoded 
		assert encoded.c_size()!=decoded.c_size() 

		SafeReplaceFile(filename, lambda: SaveBinaryDocument(filename,encoded))
		logger.info(f"CompressArcoBlock done {filename}")

# ////////////////////////////////////////////////////////////////////////////////////////
def CompressArcoDataset(db, compression="zip", num_threads=32, timestep=None,field=None):

	T1=time.time()
	timesteps=[int(it) for it in db.getTimesteps().asVector()] if timestep is None else [int(timestep)]
	fields   = db.getFields() if field is None else [str(field)]

	logger.info(f'Compressing ARCO dataset compression={compression} num_threads={num_threads} ...')

	# i use this only to generate filenames
	access=db.createAccessForBlockQuery() 

	logger.info('Collecting blocks to compress...')
	ARGS=[]
	for timestep in timesteps:
		for field in fields:
			for blockid in range(db.getTotalNumberOfBlocks()):
				dtype=field.dtype
				dims=db.getBlockQuerySamples(blockid).nsamples
				filename=access.getFilename(field,float(timestep),blockid)
				args=(compression, dims, dtype, filename)
				logger.info(f"compression={compression} dims=[{dims.toString()}] dtype={dtype.toString()} filename={filename}")
				ARGS.append(args)
	logger.info('Starting the compression...')
	p=ThreadPool(num_threads)
	p.map(lambda args: CompressArcoBlock(*args), ARGS)
	SEC=time.time()-T1
	logger.info(f"CompressArcoDataset done in {SEC} seconds")
	# TODO: do I need to save the new idx with field.default_compression? Essentially I need to check DiskAcess and CloudAccess that assume zip as defaults

# ////////////////////////////////////////////////////////////////////////////////////////
def CompressModVisusDataset(db, compression="zip", num_threads=32, timestep=None,field=None):

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

	assert not db.idxfile.arco

	# TODO: the C++ version is deprecated but it is the only one to handle with other compression schemes (lz4, zfp, etc)
	#       in the future we need to modify this function to handle with these cases
	#       for now just consider the non-zip version is slower and needs improvement
	if compression!="zip":
		return db.compressDataset([compression])
	
	T1=time.time()
	UNCOMPRESSED_SIZE,COMPRESSED_SIZE=0,0
	
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
		field if field is not None else "")

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
			elif flags_compression==0:
				decompressed=block
			else:
				assert(False) # TODO OTHER CASE

			assert len(decompressed)==blocksize
			nonlocal full_size
			full_size+=blocksize

			if compression=="zip":
				compressed=zlib.compress(decompressed,level=-1) # https://docs.python.org/3/library/zlib.html#zlib.compress
			else:
				assert(False) # TODO OTHER CASE

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

		SafeReplaceFile(filename, mem)
		del mem

		ratio=int(100*compressed_size/full_size)
		UNCOMPRESSED_SIZE+=full_size
		COMPRESSED_SIZE+=compressed_size
		logger.info(f"Compressed {filename}  in {time.time()-t1} ratio({ratio}%)")

	RATIO=int(100*COMPRESSED_SIZE/UNCOMPRESSED_SIZE) if UNCOMPRESSED_SIZE else 0.0
	logger.info(f"CompressModVisusDataset done in {time.time()-T1} seconds {RATIO}%")
	# NOTE: I don't need to save the new idx since it will be the block header telling the real compression


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
	access=db.createAccessForBlockQuery()
	access.setWritingMode()
	db.writeSlabs(generator, access=access)
	logger.info(f"ConvertImageStack DONE in {time.time()-T1} seconds")


# ////////////////////////////////////////////////////////////////////////////
def CopyDataset(SRC, dst:str, arco="modvisus", tile_size:int=None, timestep:int=None, field:str=None,num_attempts:int=3):

	arco=NormalizeArcoArg(arco)

	src=SRC.getUrl()

	logger.info(f"CopyDataset {src} {dst} arco={arco}")
	T1=time.time()
	SRC=LoadIdxDataset(src)
	
	dims=[int(it) for it in SRC.getLogicSize()]
	all_fields=SRC.getFields()
	all_timesteps=[int(it) for it in SRC.getTimesteps().asVector()]

	timesteps_to_convert=all_timesteps if timestep is None else [int(timestep)]
	fields_to_convert   =all_fields    if field    is None else [str(field)]

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
		arco=arco)
	
	Saccess=SRC.createAccessForBlockQuery() 
	Daccess=DST.createAccessForBlockQuery();Daccess.setWritingMode()

	pdim=SRC.getPointDim()
	assert pdim==2 or pdim==3 # TODO other cases

	# copy
	if tile_size is None:
		tile_size=8192 if pdim==2 else 512

	piece=[tile_size, tile_size,1 if pdim==2 else tile_size] 
	W,H,D=[dims[0]  , dims[1]  ,1 if pdim==2 else dims[2]  ]

	pieces=[]
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
						pieces.append((timestep,fieldname, logic_box))

	done_filename=dst+".done"
	try:
		DONE=int(ReadTextFile(done_filename).strip())
	except:
		DONE=0
	logger.info(f"Read {done_filename} DONE={DONE}")

	N=len([it for it in pieces])
	Saccess.beginRead()
	Daccess.beginWrite()
	logger.info(f"Number of pieces {N}")
	for I,(timestep,fieldname,logic_box) in enumerate(pieces):

		if I<DONE:
			continue

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
				logger.info(f"Wrote src={src} {I}/{N} {logic_box.toString()} timestep({timestep}) field({fieldname}) sec({read_sec:.2f}/{write_sec:.2f}/{read_sec+write_sec}) ETA({ETA:.0f}s {ETA/60:.0f}m {ETA/3600:.0}h {ETA/86400:.0f}d)")
				break
			except:
				if K==(num_attempts-1): raise
				logger.info(f"Writing of src={src} {I}/{N} failed, retrying...")
				time.sleep(1.0)

		try:
			DONE=I
			WriteTextFile(done_filename,str(DONE))
		except:
			pass

	Saccess.endRead()
	Daccess.endWrite()

	logger.info(f"CopyDataset src={src} dst={dst} done in {time.time()-T1:.2f} seconds")
	return DST


 