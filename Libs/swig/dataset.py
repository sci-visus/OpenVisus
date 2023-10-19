import os,sys
import inspect
import tempfile
import shutil

from OpenVisus import *

from threading import Thread

#in configure step I dont have numpy and skimage yet
try:
	import numpy
except:
	pass

# ////////////////////////////////////////////////////////////////////////////////////////
def NormalizeArcoArg(value):
	
	# Example: modvisus | 0 | 1mb | 512kb

	if value is None or value=="modvisus":
		return 0
	
	if isinstance(value,str):
		return StringUtils.getByteSizeFromString(value)  
	
	assert(isinstance(value,int))
	return int(value)

# //////////////////////////////////////////////////////////
def CreateIdx(**args):

	if not "url" in args:
		raise Exception("url not specified")

	url=args["url"]

	idx=IdxFile()
		
	buffer=None

	if "data" in args:
		data=args["data"]
		dim=int(args["dim"]);Assert(dim>=2) # you must specify the point dim since it could be that data has multiple components
		buffer=Array.fromNumPy(data,TargetDim=dim, bShareMem=True)
		dims=PointNi(buffer.dims)

	elif "dims" in args:
		dims=PointNi(args["dims"])
		
	else:
		raise Exception("please specify dimensions or source data")

	idx.logic_box=BoxNi(PointNi.zero(dims.getPointDim()),dims)

	# add fields
	if "fields" in args:
		for field in  args["fields"]:
			idx.fields.push_back(field)
	elif buffer:
		idx.fields.push_back(Field.fromString("DATA {} default_layout(row_major)".format(buffer.dtype.toString())))
	else:
		raise Exception("no field")

	# bitsperblock
	if "bitsperblock" in args:
		idx.bitsperblock=int(args["bitsperblock"])

	if "bitmask" in args:
		idx.bitmask=DatasetBitmask.fromString(args["bitmask"])

	# compute db overall size
	TOT=0
	for field in idx.fields:
		TOT+=field.dtype.getByteSize(idx.logic_box.size())

	# blocks per file
	if "blocksperfile" in args:
		idx.blocksperfile=int(args["blocksperfile"])
		
	elif "data" in args or TOT<2*(1024*1024*1024):
		idx.blocksperfile=-1 # all blocks in one file
		
	else:
		idx.blocksperfile==0 # openvisus will guess (probably using multiple files)
	
	# is the user specifying filters?
	if "filters" in args and args["filters"]:
		filters=args["filters"]
		for I in range(idx.fields.size()):
			idx.fields[I].filter=filters[I]

	if "time" in args:
		A,B,time_template=args["time"]
		idx.timesteps=DatasetTimesteps(A,B,1.0)
		idx.time_template=time_template

	if "filename_template" in args:
		idx.filename_template=args["filename_template"]

	if "bounds" in args:
		idx.bounds=args["bounds"]

	if "physic_box" in args:
		idx.bounds=Position(args["physic_box"])

	if "axis" in args:
		idx.axis=args["axis"]

	# am I creating an arco dataset?
	if "arco" in args:
		arco=NormalizeArcoArg(args["arco"])
		idx.arco=arco
	
	idx.save(url)
	db=LoadDataset(url)

	if buffer:
		# shortcut for the C++ method
		compression=args["compression"] if "compression" in args else ["zip"]
		assert not db.db.idxfile.arco
		db.db.compressDataset(compression, buffer)
			
	return db

# //////////////////////////////////////////////
class PyDataset(object):
	
	# constructor
	def __init__(self,db):
		self.db = db
		self.shape = tuple(reversed(self.getLogicSize()))
		self.max_resolution = self.getMaxResolution()
		self.field_names = self.getFields()

	# __getattr__
	def __getattr__(self,attr):
		return getattr(self.db, attr)	

	# getPointDim
	def getPointDim(self):
		return self.db.getPointDim()

	# getMaxResolution
	def getMaxResolution(self):
		return self.db.getMaxResolution()

	# getLogicBox
	def getLogicBox(self,x=None,y=None,z=None):
		pdim=self.getPointDim()
		lbox=self.db.getLogicBox()
		A=[lbox.p1[I] for I in range(pdim)]
		B=[lbox.p2[I] for I in range(pdim)]
		p1,p2=[0]*pdim,[0]*pdim
		for I in range(pdim):
			r=(x,y,z)[I]
			if r is None: r=[A[I],B[I]]
			p1[I] = int( A[I]+r[0]*(B[I]-A[I]) if isinstance(r[0],float) else r[0])
			p2[I] = int( A[I]+r[1]*(B[I]-A[I]) if isinstance(r[1],float) else r[1])
		return (p1,p2)
		
	# getSliceLogicBox
	def getSliceLogicBox(self,axis,offset):
		p1,p2=self.getLogicBox()
		p1[axis]=offset+0
		p2[axis]=offset+1
		return (p1,p2)
		
	# getBounds
	def getBounds(self, logic_box):
		
		if isinstance(logic_box,(tuple,list)):
			logic_box=BoxNi(PointNi(logic_box[0]),PointNi(logic_box[1]))
			
		return Position(self.logicToPhysic(),Position(BoxNi(logic_box)))

	# getLogicSize
	def getLogicSize(self):
		p1,p2=self.getLogicBox()
		return numpy.subtract(p2,p1)

	# getTimesteps
	def getTimesteps(self):
		return [int(it) for it in self.db.getTimesteps().asVector()]

	# getFields
	def getFields(self):
		return [field.name for field in self.db.getFields()]
		
	# getField
	def getField(self,value=None):
		
		if value is None:
			return self.db.getField()

		if isinstance(value,str):
			return self.db.getField(value)
			
		return value
		
	# getExtendedInfo
	def getExtendedInfo(self):
		pdim=self.getPointDim()
		p1=self.getLogicBox()[0]
		p2=self.getLogicBox()[1]
		center=[(p1[I]+p2[I])//2 for I in range(pdim)]
		dims=[(p2[I]-p1[I]) for I in range(pdim)]
		fields=[self.getField(it) for it in self.getFields()]
		timesteps=[int(it) for it in self.getTimesteps().asVector()]

		import datetime

		files=[]
		for filename in list(self.getFilenames()):
			size=FileUtils.getFileSize(Path(filename))
			if size>0:
				files.append({
					"filename" : filename, 
					"size" : size,
					"modification_time" : datetime.datetime.fromtimestamp(os.path.getmtime(filename)).strftime('%Y-%m-%d %H:%M:%S'),
					"creation_time" : datetime.datetime.fromtimestamp(os.path.getctime(filename)).strftime('%Y-%m-%d %H:%M:%S'),
				})

		ret={
			"url": self.getUrl(),
			"dimension": pdim,
			"logic_box" : self.getLogicBox(),
			"dims" : dims,
			"timesteps": timesteps,
			"fields" : [],
			"num_files" : len(files),
			"total_file_size" : sum([it["size"] for it in files]),
			"total_field_size": sum([field.dtype.getByteSize(PointNi(dims)) for field in fields]),
			"files": files,
		}

		for field in fields:
			dtype=field.dtype
			ranges=[dtype.getDTypeRange(I) for I in range(dtype.ncomponents())]
			ranges=[(r.From,r.To) if r.delta()>0 else (0,0) for r in ranges]

			ret["fields"].append({
				"name":field.name,
				"dtype":  dtype.toString(),
				"default_compression":field.default_compression,
				"default_layout":field.default_layout,
				"default_value":field.default_value,
				"filter":field.filter,
				"dtype_ranges":ranges,
				"total_field_size" : dtype.getByteSize(PointNi(dims)),
			})

		return ret

	# createAccessForBlockQuery
	def createAccessForBlockQuery(self,config=StringTree()):
		return self.db.createAccessForBlockQuery(config)

	# createAccess
	def createAccess(self,config=StringTree()):
		return self.db.createAccess(config)

	# readBlock
	def readBlock(self, block_id, time=None, field=None, access=None, aborted=Aborted()):
		Assert(access)
		field=self.getField() if field is None else self.getField(field)	
		time = self.getTime() if time is None else time
		read_block = self.db.createBlockQuery(block_id, field, time, ord('r'), aborted)
		self.executeBlockQueryAndWait(access, read_block)
		if not read_block.ok(): return None
		self.db.convertBlockQueryToRowMajor(read_block) # default is to change the layout to rowmajor
		return Array.toNumPy(read_block.buffer, bShareMem=False)

	# writeBlock
	def writeBlock(self, block_id, time=None, field=None, access=None, data=None, aborted=Aborted()):
		Assert(access is not None)
		Assert(isinstance(data, numpy.ndarray))
		field=self.getField() if field is None else self.getField(field)	
		time = self.getTime() if time is None else time
		write_block = self.db.createBlockQuery(block_id, field, time, ord('w'), aborted)
		write_block.buffer=Array.fromNumPy(data,TargetDim=self.getPointDim(), bShareMem=True)
		# note write_block.buffer.layout is empty (i.e. rowmajor)
		self.executeBlockQueryAndWait(access, write_block)
		return write_block.ok()


	# read
	def read(self, logic_box=None, x=None, y=None, z=None, time=None, field=None, field_name=None, num_refinements=1, quality=0, max_resolution=None, disable_filters=False, access=None):
		"""
		Reads a box in voxel or unit coordinates.

		Examples:
		dataset = load_dataset(url)

		# example of reading a single slice with z coordinate 512
		data = dataset.read(z=[512,513]) 
		
		# example of reading a box in normalized coordinates (i.e., [0,1])
		data = dataset.read(x=[0,0.1], y=[0.1,0.2], z=[0,0.1])
		
		# example of reading a single slice with 3 refinements
		for data in dataset.read(z=[512,513], num_refinements=3):
			print(data)
		"""
		if x is not None and not isinstance(x[0], float):
			if not x[0] < x[1]:
				raise IndexError(f"The first index in x needs to be lower than the second index")
			if not (0 <= x[0] < self.shape[-1] and 0 < x[1] <= self.shape[-1]):
				raise IndexError(f"The bounds specified in the argument x are outside the dataset's bounds [0,{self.shape[-1]}]")
		if y is not None and not isinstance(y[0], float):
			if not y[0] < y[1]:
				raise IndexError(f"The first index in y needs to be lower than the second index")
			if not (0 <= y[0] < self.shape[-2] and 0 < y[1] <= self.shape[-2]):
				raise IndexError(f"The bounds specified in the argument y are outside the dataset's bounds [0,{self.shape[-2]}]")
		# TODO(12/10/2022): What if the dataset is 2D?
		if z is not None and not isinstance(z[0], float):
			if not z[0] < z[1]:
				raise IndexError(f"The first index in z needs to be lower than the second index")
			if not (0 <= z[0] < self.shape[-3] and 0 < z[1] <= self.shape[-3]):
				raise IndexError(f"The bounds specified in the argument z are outside the dataset's bounds [0,{self.shape[-3]}]")

		if max_resolution is not None and not (0 <= max_resolution <= self.max_resolution):
			raise ValueError(f"The valid range for max_resolution is from 0 to {self.max_resolution}")

		if field_name is not None and field_name not in self.field_names:
			raise ValueError(f"The field name '{field_name}' is not in field names {self.field_names}")

		pdim=self.getPointDim()

		field=self.getField() if field is None else self.getField(field)	
			
		if time is None:
			time = self.getTime()			

		if logic_box is None:
			logic_box=self.getLogicBox(x,y,z)

		if isinstance(logic_box,(tuple,list)):
			logic_box=BoxNi(PointNi(logic_box[0]),PointNi(logic_box[1]))

		query = self.db.createBoxQuery(BoxNi(logic_box), field , time, ord('r'))
		
		if disable_filters:
			query.disableFilters()
		else:
			query.enableFilters()
		
		if max_resolution is None:
			max_resolution=self.getMaxResolution()
		
		# example quality -3 means not full resolution
		Assert(quality<=0)
		max_resolution=max_resolution+quality 
		
		for I in reversed(range(num_refinements)):
			res=max_resolution-(pdim*I)
			if res>=0:
				query.end_resolutions.push_back(res)
		
		self.db.beginBoxQuery(query)
		
		if not query.isRunning():
			raise Exception("begin query failed {0}".format(query.errormsg))
			
		if not access:
			access=self.db.createAccess()
			
		def NoGenerator():
			if not self.db.executeBoxQuery(access, query):
				raise Exception("query error {0}".format(query.errormsg))
			# i cannot be sure how the numpy will be used outside or when the query will dealllocate the buffer
			data=Array.toNumPy(query.buffer, bShareMem=False) 
			return data
			
		def WithGenerator():
			while query.isRunning():

				if not self.db.executeBoxQuery(access, query):
					raise Exception("query error {0}".format(query.errormsg))

				# i cannot be sure how the numpy will be used outside or when the query will dealllocate the buffer
				data=Array.toNumPy(query.buffer, bShareMem=False) 
				yield data
				self.db.nextBoxQuery(query)	

		return NoGenerator() if query.end_resolutions.size()==1 else WithGenerator()
			


	# write
	# IMPORTANT: usually db.write happens without write lock and synchronously (at least in Python)
	def write(self, data, x=0, y=0, z=0,logic_box=None, time=None, field=None, access=None):

		"""
		import OpenVisus as ov

		db=ov.open_dataset(url)
		width,height,depth=db.getLogicSize()

		# write single slice
		data=numpy.zeros([height,width,3],dtype.uint8)
		db.write(data,z=[512,513]) 

		# write several slices in one-shot
		nslices=10
		data=numpy.zeros([nslices,height,width,10,3],dtype.uint8)
		db.write(data,z=[512,512+nslices])

		# write several slices with a generator
		nslices=10
		def gen():
			for I in range(nslices):
				yield=p.zeros([height,width,3],dtype.uint8)
		db.write(gen,z=512)
		"""
		
		pdim=self.getPointDim()
		
		field=self.getField(field)
		
		if time is None:
			time = self.getTime()


		dims=list(data.shape)
		
		# remove last components
		if field.dtype.ncomponents()>1:
			dims=dims[:-1]
		
			# could be I'm writing a slice, I need to increment the "dimension"
		while len(dims)<pdim: 
			dims=[1] + dims	
		
		dims=list(reversed(dims))	

		if logic_box is None:
			p1=PointNi([x,y,z][0:pdim])
			logic_box=BoxNi(p1,p1+PointNi(dims))

		if isinstance(logic_box,(tuple,list)):
			logic_box=BoxNi(PointNi(logic_box[0]),PointNi(logic_box[1]))

		query = self.db.createBoxQuery(logic_box, field , time , ord('w'))
		query.end_resolutions.push_back(self.getMaxResolution())
		
		self.db.beginBoxQuery(query)
		
		if not query.isRunning():
			raise Exception("begin query failed {0}".format(query.errormsg))
			
		if not access:
			access=self.createAccessForBlockQuery()
			access.disableWriteLocks()
			access.disableCompression()
		
		# I need to change the shape of the buffer, since the last component is the channel (like RGB for example)
		buffer=Array.fromNumPy(data,bShareMem=True)
		Assert(buffer.c_size()==data.nbytes)
		buffer.resize(PointNi(dims),query.field.dtype,__file__,0)
		
		query.buffer=buffer
		
		if not self.db.executeBoxQuery(access, query):
			raise Exception("query error {0}".format(query.errormsg))
			
	# writeSlabs
	def writeSlabs(self,slices, x=0, y=0, z=0, time=None, field=None, max_memsize=4*1024*1024*1024, access=None):
		
		os.environ["VISUS_DISABLE_WRITE_LOCK"]="1"
		
		slab=[]
		memsize=0
		
		for slice in slices:
			slab.append(slice)
			memsize+=slice.nbytes
			
			# flush
			if memsize>=max_memsize: 
				data=numpy.stack(slab,axis=0)
				self.write(data , x=x, y=y, z=z,field=field,time=time, access=access)
				z+=len(slab)
				slab=[]
				memsize=0

		# flush
		if slab: 
			data=numpy.stack(slab,axis=0)
			self.write(data , x=x, y=y, z=z,field=field,time=time, access=access)

	#getXSlice (get a slice orthogonal to the X axis)
	def getXSlice(self, position=None, resolution=-1,resample_output=True): 
		"""
		Get a slice orthogonal to the X axis.
		resample_output=True	(resample to the full   resolutiuon) 
		resample_output=False (resample to the lower resolutiuon) 
		resample_output= (x,y) (resample to a (x,y) grid) 
		"""

		myLogicBox = self.getLogicBox()
		x_dim = myLogicBox[1][0]
		y_dim = myLogicBox[1][1]
		z_dim = myLogicBox[1][2]
		normalizationFactor = 2**(-resolution)
		
		if position==None:
			position = x_dim//2
		
		# adjust to slices that exist at this level of resolution
		position = (position//normalizationFactor) *normalizationFactor 
		
		# One slice is a volume
		data = self.read(x=[position,position+1],  y=[0,y_dim], z=[0,z_dim],quality=resolution*3)
		data = data[:,:,0]

		from skimage.transform import resize

		if resample_output==True:
			data = resize(data, (z_dim,y_dim), preserve_range=True).astype(data.dtype)
			
		elif	type(resample_output) is tuple:
			data = resize(data, resample_output, preserve_range=True).astype(data.dtype)
			
		return data
		
		
	#getXYSlice (get a slice orthogonal to the X axis)
	def getXYSlice(self, position=None, XY_MinMax=None, resolution=-1, resample_output=True, time=None, field=None):
		"""
		Get a slice orthogonal to the X axis.
		resample_output=True	(resample to the full   resolutiuon) 
		resample_output=False (resample to the lower resolutiuon) 
		resample_output= (x,y) (resample to a (x,y) grid) 
		"""
		def normalize_position(position,axis,myLogicBox,resolution=-1):
			normalizationFactor = 2 ** (-resolution)
			axis_dim = myLogicBox[1][axis]
			if position == None:  # if None pick middle slice
				position = axis_dim // 2
			elif isinstance(position, float):
				if position > 1:  # if out of range bring back to the limit
					position = int(axis_dim - 1)
				elif position < 0:
					position = int(0)
				else:  # Convert the ratio to an integer
					position = int(position * (axis_dim - 1))

			# adjust to slices that exist at this level of resolution
			position = (position // normalizationFactor) * normalizationFactor
			return position

		#xMin = normalize_position(XY_MinMax[0][0],0 myLogicBox,resolution=resolution)

		myLogicBox = self.getLogicBox()
		spaceDim = len(myLogicBox[1])
		x_dim = myLogicBox[1][0]
		y_dim = myLogicBox[1][1]
		if spaceDim == 2:
			z_dim = position = None
		else:
			z_dim = myLogicBox[1][2]
			position  = normalize_position(position,2,myLogicBox,resolution=resolution)

		if XY_MinMax==None:
			x_min, x_max, y_min, y_max = 0, x_dim, 0, y_dim
		else:
			x_min = normalize_position(XY_MinMax[0],0,myLogicBox,resolution=resolution)
			x_max = normalize_position(XY_MinMax[1],0,myLogicBox,resolution=resolution)
			y_min = normalize_position(XY_MinMax[2],1,myLogicBox,resolution=resolution)
			y_max = normalize_position(XY_MinMax[3],1,myLogicBox,resolution=resolution)
		#print("XY_MinMax=",x_min, x_max, y_min, y_max)
		x_dim, y_dim = x_max-x_min, y_max-y_min
		# One slice is a volume
		if spaceDim == 2:
			data = self.read(x=[x_min,x_max], y=[y_min,y_max],  quality=resolution*2, time=time, field=field)
		else:
			data = self.read(x=[x_min,x_max], y=[y_min,y_max], z=[position,position+1],quality=resolution*3, time=time, field=field)
		#print("data.shape=",data.shape)
		if spaceDim > 2:
			data = data[0,:,:]

		from skimage.transform import resize

		if resample_output==True:
			data = resize(data, (z_dim,y_dim), preserve_range=True).astype(data.dtype)
			
		elif	type(resample_output) is tuple:
			data = resize(data, resample_output, preserve_range=True).astype(data.dtype)

			
		return data

	# copyDataset
	def copyDataset(self, dst:str, arco="modvisus", tile_size:int=None, timestep:int=None, field:str=None,num_attempts:int=3):
		from .convert import CopyDataset
		return CopyDataset(self.db, dst, arco, tile_size, timestep, field, num_attempts)

	# compressDataset
	def compress_dataset(self):
		self.compressDataset()

	def compressDataset(self, compression="zip", num_threads=32, timestep=None,field=None):

		# TODO: enable different compressions for different levels
		if isinstance(compression, (list, tuple)):
			assert len(compression)==1 
			compression=compression[0]		

		# no compression specified
		if compression=="" or compression=="raw":
			return

		if self.db.idxfile.arco:
			from .convert import CompressArcoDataset
			CompressArcoDataset(self.db,compression=compression,num_threads=num_threads,timestep=timestep,field=field)
		else:
			from .convert import CompressModVisusDataset
			CompressModVisusDataset(self.db,compression=compression,num_threads=num_threads,timestep=timestep,field=field)

		# override the information for fields
		fields=[it for it in self.db.idxfile.fields]
		self.db.idxfile.fields.clear()
		for field in fields:
			field.default_compression=compression
			self.db.idxfile.fields.push_back(field)

		url=self.getUrl()
		self.db.idxfile.save(url)
		self.db=LoadDatasetCpp(url)
		
	# copyBlocks
	def copyBlocks(self, dst, time=None, field=None, num_read_per_request=16, num_threads=4, verbose=False):

		if isinstance(dst,str):

			# create the dataset
			if not os.path.isfile(dst):
				idx=IdxFile.clone(self.db.idxfile)
				idx.version= 0             # re-validate 
				idx.filename_template = "" # re-guess (since the path can be different)
				idx.save(dst)

			dst=LoadDataset(dst)


		from .convert import CopyBlocks
		copier=CopyBlocks(self.db, dst,time=time,field=field,num_read_per_request=num_read_per_request, verbose=verbose)
		tot_blocks=self.db.getTotalNumberOfBlocks()
		num_per_thread=tot_blocks//num_threads
		logger.info(f"tot_blocks={tot_blocks} num_threads={num_threads} num_per_thread={num_per_thread}")

		if num_threads<=1:
			copier.doCopy(0,tot_blocks)
		else:
			threads=[]
			for I in range(num_threads):
				A=(I+0)*num_per_thread
				B=(I+1)*num_per_thread if I<(num_threads-1) else tot_blocks
				threads.append(Thread(target=CopyBlocks.doCopy, args=(copier,A,B)))
			for t in threads: t.start()
			for t in threads: t.join()

	# copyDatasetToCloud
	def copyDatasetToCloud(
		self,            
		local:str,              # local ARCO, you need enough space for doing the local conversion
		remote:str,             # s3://bucket/whatever/visus.idx i.e. the remote location
		done:str=None,          # keep track on s3://.. if the conversion was already done or not
		arco:str="1mb",         # anything from 1mb to 8mb should work
		compression:str="zip",  # what kind of compression to apply
		clean_local:bool=True,  # clean local dataset at the end
		timestep:int=None,      # specify timestep or None for all datasets
		field:str=None,         # specify field or None for all datasets
		):  
		from .convert import CopyDatasetToCloud
		CopyDatasetToCloud(self,local=local,remote=remote,done=done,arco=arco,compression=compression,clean_local=clean_local,timestep=timestep,field=field)


def create_idx(*, url, shape, fields):
	"""
	Creates a local .idx file at the url location storing the metadata of the shape and fields.
	The data is written using the write function.

	Example:
	dataset = create_idx(url="test.idx", shape=(10, 20, 30), fields=[Field("data", "float32")])
	dataset.write(data)
	dataset.compress_dataset()
	"""
	return CreateIdx(url=url, dims=list(reversed(shape)), fields=fields, arco="1mb")




############### stable OpenVisus API #####################
def load_dataset(url, cache_dir=""):
	"""
	Loads dataset locally or over http.

	Examples:
	dataset = load_dataset("test.idx")
	dataset = load_dataset("http://domain.com/test.idx")
	dataset = load_dataset("http://domain.com/test.idx", cache_dir='.')
	"""
	# NOTE(12/5/2022): cache_dir exception is the same as url not existing (SystemError), so we check
	#	the directory here
	if os.path.isfile(cache_dir):
		raise NotADirectoryError(f"The cache_dir {cache_dir} is a file and not a directory")
	try:
		dataset = Dataset1(LoadDatasetCpp(url, cache_dir))
	except SystemError as e:
		dataset = None
	if dataset is None:
		raise FileNotFoundError()
	return dataset


class Dataset1(object):
	def __init__(self,db):
		self.db = db
		self.max_resolution = self.db.getMaxResolution()
		self.field_names = [field.name for field in self.db.getFields()]

		low, high = self.getLogicBox()
		assert len(low) == len(high)
		self.x = low[0], high[0] if len(low) > 0 else None
		self.y = low[1], high[1] if len(low) > 1 else None
		self.z = low[2], high[2] if len(low) > 2 else None


	def getLogicBox(self,x=None,y=None,z=None):
		pdim=self.db.getPointDim()
		lbox=self.db.getLogicBox()
		A=[lbox.p1[I] for I in range(pdim)]
		B=[lbox.p2[I] for I in range(pdim)]
		p1,p2=[0]*pdim,[0]*pdim
		for I in range(pdim):
			r=(x,y,z)[I]
			if r is None: r=A[I],B[I]
			if type(r) is tuple:
				p1[I] = r[0]
				p2[I] = r[1]
			else:
				p1[I] = r
				p2[I] = r + 1
		return p1,p2
		

	def read(self, logic_box=None, x=None, y=None, z=None, field_name=None, resolution=None):
		"""
		Reads a box in voxel or unit coordinates.

		Examples:
		dataset = load_dataset(url)

		# example of reading a single slice with z coordinate 512
		data = dataset.read(z=512) 

		# reading box with y coordinate automatically set, first coordinate is inclusive, second is exclusive
		data = dataset.read(x=(10,20), z=(5,30))
		"""
		if x is None:
			x = self.x
		if y is None:
			y = self.y
		if z is None:
			z = self.z
		if resolution is None:
			resolution = self.max_resolution

		if x is not None:
			if type(x) is tuple:
				if not x[0] < x[1]:
					raise IndexError(f"The first index in x needs to be lower than the second index")
				if not (self.x[0] <= x[0] < self.x[1] and self.x[0] < x[1] <= self.x[1]):
					raise IndexError(f"The bounds specified in the argument x are outside the dataset's bounds [{self.x[0]},{self.x[1]}]")
			elif not (self.x[0] <= x < self.x[1]):
				raise IndexError(f"The bounds specified in the argument x are outside the dataset's bounds [{self.x[0]},{self.x[1]}]")
		if y is not None:
			if type(y) is tuple:
				if not y[0] < y[1]:
					raise IndexError(f"The first index in y needs to be lower than the second index")
				if not (self.y[0] <= y[0] < self.y[1] and self.y[0] < y[1] <= self.y[1]):
					raise IndexError(f"The bounds specified in the argument y are outside the dataset's bounds [{self.y[0]},{self.y[1]}]")
			elif not (self.y[0] <= y < self.y[1]):
				raise IndexError(f"The bounds specified in the argument y are outside the dataset's bounds [{self.y[0]},{self.y[1]}]")
		if z is not None:
			if type(z) is tuple:
				if not z[0] < z[1]:
					raise IndexError(f"The first index in z needs to be lower than the second index")
				if not (self.z[0] <= z[0] < self.z[1] and self.z[0] < z[1] <= self.z[1]):
					raise IndexError(f"The bounds specified in the argument z are outside the dataset's bounds [{self.z[0]},{self.z[1]}]")
			elif not (self.z[0] <= z < self.z[1]):
				raise IndexError(f"The bounds specified in the argument z are outside the dataset's bounds [{self.z[0]},{self.z[1]}]")

		if resolution is not None and not (0 <= resolution <= self.max_resolution):
			raise ValueError(f"The valid range for max_resolution is [0, {self.max_resolution}]")

		if field_name is not None and field_name not in self.field_names:
			raise ValueError(f"The field name '{field_name}' is not in field names {self.field_names}")

		if field_name is None:
			field = self.db.getField()
		else:
			field = self.db.getField(field_name)

		pdim = self.db.getPointDim()
			
		time = self.db.getTime()			
		logic_box = self.getLogicBox(x,y,z)

		if isinstance(logic_box, tuple):
			logic_box = BoxNi(PointNi(logic_box[0]), PointNi(logic_box[1]))

		query = self.db.createBoxQuery(BoxNi(logic_box), field , time, ord('r'))
		query.disableFilters()
		query.end_resolutions.push_back(resolution)
		
		self.db.beginBoxQuery(query)
		
		if not query.isRunning():
			raise Exception(f"begin query failed {query.errormsg}")
			
		access = self.db.createAccess(StringTree())
		#access.disableWriteLock()
			
		if not self.db.executeBoxQuery(access, query):
			raise Exception(f"query error {query.errormsg}")
		# I cannot be sure how the numpy will be used outside or when the query will dealllocate the buffer
		data = Array.toNumPy(query.buffer, bShareMem=False)
		if pdim != 3:
			return data

		# extract slice if needed
		if type(x) is not tuple:
			data = data[:,:,0]
		elif type(y) is not tuple:
			data = data[:,0,:]
		elif type(z) is not tuple:
			data = data[0,:,:]
		return data
