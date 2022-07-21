---
layout: default
parent: Python OpenViSUS Functions
nav_order: 2
---

# CreateIdx(**args)

Describe function here.

# Function Definition

```python
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

	# am I creating an arco dataset?
	if "arco" in args:
		assert isinstance(args["arco"],int)
		idx.arco=args["arco"]

	idx.save(url)
	db=LoadDataset(url)

	if buffer:
		compression=args["compression"] if "compression" in args else ["zip"]
		db.compressDataset(compression, buffer)
			
	return db
```