import os, sys, glob, subprocess, platform, shutil, sysconfig, re, argparse

this_dir=os.path.dirname(os.path.abspath(__file__))

if len(sys.argv)>=2 and sys.argv[1]=="dirname":
	print(this_dir)
	sys.exit(0)

from OpenVisus import *

"""
Linux:
	readelf -d bin/visus
	QT_DEBUG_PLUGINS=1 LD_DEBUG=libs,files  ./visusviewer.sh
	LD_DEBUG=libs,files ldd bin/visus

OSX:
	otool -L libname.dylib
	otool -l libVisusGui.dylib  | grep -i "rpath"
	DYLD_PRINT_LIBRARIES=1 QT_DEBUG_PLUGINS=1 visusviewer.app/Contents/MacOS/visusviewer
"""

WIN32=platform.system()=="Windows" or platform.system()=="win32"
APPLE=platform.system()=="Darwin"

# /////////////////////////////////////////////////////////////////////////
def ReadTextFile(filename):
	file = open(filename, "r") 
	ret=file.read().strip()
	file.close()
	return ret
	
# /////////////////////////////////////////////////////////////////////////
def GetFilenameWithoutExtension(filename):
	return os.path.splitext(os.path.basename(filename))[0]

# /////////////////////////////////////////////////////////////////////////
def GetCommandOutput(cmd,shell=False):
	print("Executing command", cmd)
	output=subprocess.check_output(cmd, shell = shell, universal_newlines=True)
	return output.strip()

# /////////////////////////////////////////////////////////////////////////
def ExecuteCommand(cmd,shell=False,check_result=False):	
	print("Executing command", " ".join(cmd))
	if check_result:
		return subprocess.check_call(cmd, shell=shell)
	else:
		return subprocess.call(cmd, shell=shell)

# ////////////////////////////////////////////////
def SetRPath(filename,value):
	
	if APPLE:

		def GetRPaths(filename):
			try:
				lines  = GetCommandOutput("otool -l '%s' | grep -A2 LC_RPATH | grep path " % filename, shell=True).splitlines()
			except:
				return []
				
			path_re = re.compile("^\s*path (.*) \(offset \d*\)$")
			return [path_re.search(line).group(1).strip() for line in lines]
			
		for it in GetRPaths(filename):
			ExecuteCommand(["install_name_tool","-delete_rpath",it, filename])
	
		for it in value.split(":"):
			ExecuteCommand(["install_name_tool","-add_rpath", it, filename])
			
		print(filename,GetRPaths(filename))

	else:
		ExecuteCommand(["patchelf","--set-rpath",value, filename])			
				

# ////////////////////////////////////////////////
def ExtractDeps(filename):
	output=GetCommandOutput(['otool', '-L' , filename])
	deps=[line.strip().split(' ', 1)[0].strip() for line in output.split('\n')[1:]]
	deps=[dep for dep in deps if os.path.basename(filename)!=os.path.basename(dep)] # remove any reference to myself
	return deps
	
# ////////////////////////////////////////////////
def ShowDeps(all_bins):
	all_deps={}
	for filename in all_bins:
		for dep in ExtractDeps(filename):
			all_deps[dep]=1
	all_deps=list(all_deps)
	all_deps.sort()	
	
				
	print("\nAll dependencies....")
	for filename in all_deps:
		print(filename)	

# ////////////////////////////////////////////////
def UsePyQt5():
	
	"""
	python -m pip install johnnydep

	johnnydep PyQt5~=5.14.0	-> PyQt5-sip<13,>=12.7
	johnnydep PyQt5~=5.13.0	-> PyQt5_sip<13,>=4.19.19
	johnnydep PyQt5~=5.12.0 -> PyQt5_sip<13,>=4.19.14
	johnnydep PyQt5~=5.11.0 -> empty
	johnnydep PyQt5~=5.10.0 -> empty
	johnnydep PyQt5~=5.9.0  -> empty

	johnnydep PyQtWebEngine~=5.14.0 -> PyQt5>=5.14 
	johnnydep PyQtWebEngine~=5.13.0 -> PyQt5>=5.13
	johnnydep PyQtWebEngine~=5.12.0 -> PyQt5>=5.12
	johnnydep PyQtWebEngine~=5.11.0 -> does not exist
	johnnydep PyQtWebEngine~=5.10.0 -> does not exist
	johnnydep PyQtWebEngine~=5.19.0 -> does not exist

	johnnydep PyQt5-sip~=12.7.0 -> empty
	"""

	QT_VERSION=ReadTextFile("QT_VERSION")
	print("Installing PyQt5...",QT_VERSION)
	major,minor=QT_VERSION.split('.')[0:2]

	try:
		import conda.cli
		bIsConda=True
	except:
		bIsConda=False
		
	if bIsConda:
		
		conda.cli.main('conda', 'install',    '-y', "pyqt={}.{}".format(major,minor))
		# do I need PyQtWebEngine for conda? considers Qt is 5.9 (very old)
		# it has webengine and sip included

	else: 

		cmd=[sys.executable,"-m", "pip", "install", "--progress-bar", "off", "PyQt5~={}.{}.0".format(major,minor)]

		if int(major)==5 and int(minor)>=12:
			cmd+=["PyQtWebEngine~={}.{}.0".format(major,minor)]
			cmd+=["PyQt5-sip<13,>=12.7"] 

		ExecuteCommand(cmd)

	try:
		import PyQt5
		PyQt5_HOME=os.path.dirname(PyQt5.__file__)

	except:
		# this should cover the case where I just installed PyQt5
		PyQt5_HOME=GetCommandOutput([sys.executable,"-c","import os,PyQt5;print(os.path.dirname(PyQt5.__file__))"]).strip()

	print("Linking PyQt5_HOME",PyQt5_HOME)
	
	if not os.path.isdir(PyQt5_HOME):
		print("Error directory does not exists")
		raise Exception("internal error")

	# on windows it's enough to use sys.path (see *.i %pythonbegin section)
	if not WIN32:
		
		if APPLE:		

			dylibs=glob.glob("bin/*.dylib")
			so=glob.glob("*.so")
			apps=["%s/Contents/MacOS/%s"   % (it,GetFilenameWithoutExtension(it)) for it in glob.glob("bin/*.app")]
			all_bins=so + dylibs + apps
			
			# remove any reference to absolute Qt (it happens with brew which has absolute path), make it relocable with rpath as is in PyQt5
			for filename in all_bins:
				lines  = GetCommandOutput("otool -L %s | grep 'lib/Qt.*\.framework' | awk '{print $1;}'" % filename, shell=True).splitlines()
				for Old in lines:
					New="@rpath/Qt" + Old.split("lib/Qt")[1]
					ExecuteCommand(["install_name_tool","-change", Old, New, filename])	
					
			QtLib=os.path.join(PyQt5_HOME,'Qt/lib')

			for filename in so + dylibs:
				SetRPath(filename, "@loader_path/:@loader_path/bin:" + QtLib)
				
			for filename in apps:
				SetRPath(filename, "@loader_path/:@loader_path/../../../:" + QtLib)
						
			ShowDeps(all_bins)
				
		else:
			
			for filename in glob.glob("*.so") + glob.glob("bin/*.so") + ["bin/visus","bin/visusviewer"]:
				SetRPath(filename,"$ORIGIN:$ORIGIN/bin:" + os.path.join(PyQt5_HOME,'Qt/lib'))


# ////////////////////////////////////////////////
def TestWriteSpeed(args):
	parser = argparse.ArgumentParser(description="Test IO write speed")
	parser.add_argument("--filename", type=str, help="Temporary filename", required=False,default="~temp.bin")
	parser.add_argument("--blocksize", type=str, help="Block size", required=False,default="64*1024")
	args = parser.parse_args(args)
	args.blocksize=int(eval(args.blocksize))
	print(action,"filename",args.filename, "blocksize",args.blocksize)

	buffer=Array(args.blocksize, DType.fromString("uint8"))
	if os.path.exists(args.filename):
		os.remove(args.filename)
	file=File()
	Assert(file.createAndOpen(args.filename, "w"))
	t1 = Time.now()
	nwritten=0
	cont=0
	while True:
		Assert(file.write(nwritten, args.blocksize, buffer.c_ptr()))
		nwritten+=args.blocksize
		cont+=1
		if cont> 0 and (cont % 8196) == 0:
			print(int(nwritten / (1024.0 * 1024.0 * t1.elapsedSec())), "MB/sec", int(nwritten/(1024*1024)),"MB")
	file.close()

# ////////////////////////////////////////////////
def TestReadSpeed(args):
	parser = argparse.ArgumentParser(description="Test IO read speed")
	parser.add_argument("--filename", type=str, help="Temporary filename", required=False,default="~temp.bin")
	parser.add_argument("--blocksize", type=str, help="Block size", required=False,default="64*1024")
	args = parser.parse_args(args)
	args.blocksize=int(eval(args.blocksize))
	print(action,"filename",args.filename, "blocksize",args.blocksize)

	file=File()
	Assert(file.open(args.filename, "r"))
	buffer=Array(args.blocksize, DType.fromString("uint8"))
	t1 = Time.now()
	nread,cont=0,0
	while file.read(nread, args.blocksize, buffer.c_ptr()):
		nread += args.blocksize
		cont+=1
		if cont> 0 and (cont % 8196) == 0:
			print(int(nread / (1024.0 * 1024.0 * t1.elapsedSec())), " MB/sec", int(nread/(1024*1024)),"MB")
	file.close()
	print("nread",nread/(1024*1024),"MB")

# ////////////////////////////////////////////////
def TestNetworkSpeed(args):
	parser = argparse.ArgumentParser(description="Test IO read speed")
	parser.add_argument("--nconnections", type=int, help="Number of connections", required=False,default=8)
	parser.add_argument("--nrequests", type=int, help="Number of Request", required=False,default=1000)
	parser.add_argument("--url", action='append', help="Url", required=True)
	args = parser.parse_args(args)

	print("NetService speed test")
	print("nconnections",args.nconnections)
	print("nrequests",args.nrequests)
	print("urls\n","\n\t".join(args.url))
	NetService.testSpeed(args.nconnections,args.nrequests,args.url)

# ////////////////////////////////////////////////
def TestQuerySpeed(args):
	parser = argparse.ArgumentParser(description="Test query read speed")
	parser.add_argument("--dataset", type=str, help="Dataset", required=True)
	parser.add_argument("--query-dim", type=int, help="Query dimension", required=True)
	args = parser.parse_args(args)

	db=LoadDataset(args.dataset)
	db.testQuerySpeed(db.getDefaultTime(),db.getDefaultField(),args.query_dim)

# ////////////////////////////////////////////////
def TestSlabSpeed(args):
	parser = argparse.ArgumentParser(description="Test slab speed")
	parser.add_argument("--dataset", type=str, help="Dataset", required=True)
	parser.add_argument("--dims", type=str, help="Dataset dimension", required=True)
	parser.add_argument("--num-slabs", type=int, help="Number of slabs", required=True)
	parser.add_argument("--dtype", type=str, help="DType", required=True)
	parser.add_argument("--layout", type=str, help="layourt", required=True)
	args = parser.parse_args(args)
	Dataset.testSlabSpeed(args.dataset,PointNi.fromString(args.dims),args.num_slabs,args.dtype,args.layout)

# ////////////////////////////////////////////////
def FixRange(args):
	parser = argparse.ArgumentParser(description="Fix IDX range")
	parser.add_argument("--dataset", type=str, help="Dataset", required=True)
	parser.add_argument("--time", type=float, help="Time", required=False)
	parser.add_argument("--field", type=str, help="Field", required=False)
	args = parser.parse_args(args)

	db=LoadDataset(args.dataset)
	Assert(db)

	if args.time is None:
		args.time=db.getDefaultTime()

	if args.field is None:
		args.field=db.getDefaultField().name

	field=db.getFieldByName(args.field)

	# compute the ranges
	N = field.dtype.ncomponents()
	ranges=[[+sys.float_info.max,-sys.float_info.max]] * N
			
	access = db.createAccessForBlockQuery()
	access.beginRead()
	for block_id in range(db.getTotalNumberOfBlocks()):

		read_block = BlockQuery(db, field, args.time, access.getStartAddress(block_id), access.getEndAddress(block_id), ord('r'), Aborted())

		if not db.executeBlockQueryAndWait(access, read_block):
			continue

		buffer=Array.toNumPy(read_block.buffer,bShareMem=True)

		for C in range(N):
			A=numpy.min(buffer[...,C] if N>1 else buffer)
			B=numpy.max(buffer[...,C] if N>1 else buffer)
			ranges[C][0]=min(ranges[C][0],A)
			ranges[C][1]=max(ranges[C][1],B)

	access.endRead()

	print("Computed ranges", ranges)

	# save the new idx file
	idxfile=IdxFile()
	idxfile.load(args.dataset)
	for F in range(idxfile.fields.size()):
		if idxfile.fields[F].name == field.name:
			for C in range(N):
				A=float(ranges[C][0])
				B=float(ranges[C][1])
				idxfile.fields[F].setDTypeRange(Range(A,B,float(0.0)), C)
			idxfile.save(args.dataset)
			return True
			
	raise Exception("cannot find field")

# ////////////////////////////////////////////////
def RunServer(args):
	parser = argparse.ArgumentParser(description="server command.")
	parser.add_argument("-p", "--port", type=int, help="Server port.", required=False,default=10000)
	parser.add_argument("-d", "--dataset", type=str, help="Idx file", required=False,default="")
	parser.add_argument("-e", "--exit", help="Exit immediately", action="store_true") # for debugging
	args = parser.parse_args(args)

	modvisus = ModVisus()

	# -m OpenVisus server --port 10000 [--dataset D:\projects\OpenVisus\datasets\cat\rgb.idx]
	if args.dataset:
		config=ConfigFile.fromString("<visus><datasets><dataset name='default' url='{}' permissions='public' /></datasets></visus>".format(args.dataset))
	else:
		config=DbModule.getModuleConfig()
			
	modvisus.configureDatasets(config)
	server=NetServer(args.port, modvisus)
	print("Running visus server on port",args.port)

	if args.exit:
		server.signalExit()

	server.runInThisThread()
	print("server done")

# ////////////////////////////////////////////////
def CopyDataset(args):
	parser = argparse.ArgumentParser(description="copy-dataset command.")
	parser.add_argument("--src"       , type=str,   help="src dataset", required=True)
	parser.add_argument("--dst"       , type=str,   help="dst dataset", required=True)
	parser.add_argument("--src-time"  , type=float, help="src time",    required=False,default=None)
	parser.add_argument("--dst-time"  , type=float, help="dst time",    required=False,default=None)
	parser.add_argument("--src-field ", type=str,   help="src field",   required=False,default=None)
	parser.add_argument("--dst-field" , type=str,   help="dst field",   required=False,default=None)
	parser.add_argument("--src-access", type=str,   help="src access",  required=False,default=None)
	parser.add_argument("--dst-access", type=str,   help="dst access",  required=False,default=None)
	args = parser.parse_args(args)

	src=LoadDataset(args.src)
	dst=LoadDataset(args.dst)

	src_time=src.getDefaultTime() if args.src_time is None else args.src_time
	dst_time=dst.getDefaultTime() if args.dst_time is None else args.dst_time   

	src_field=src.getDefaultField() if args.src_field is None else args.src_field
	dst_field=dst.getDefaultField() if args.dst_field is None else args.dst_field

	Assert(src_field.dtype==dst_field.dtype)

	src_access=src.createAccessForBlockQuery(StringTree.fromString(args.src_access))
	dst_access=dst.createAccessForBlockQuery(StringTree.fromString(args.dst_access))

	Dataset.copyDataset(
		dst, src_access, dst_field, dst_time,
		src, dst_access, src_field, src_time)


# ////////////////////////////////////////////////
def CompressDataset(args):
	parser = argparse.ArgumentParser(description="compress dataset")
	parser.add_argument("--dataset"       , type=str,   help="dataset", required=True)
	parser.add_argument("--compression"   , type=str,   help="compresson", required=True)
	args = parser.parse_args(args)

	db=LoadDataset(args.dataset)
	Assert(db)
	Assert(db.compressDataset(args.compression))

# ////////////////////////////////////////////////
def RunViewer(args):
	from VisusGuiPy import GuiModule,Viewer
	viewer=Viewer()
	viewer.configureFromArgs(args)
	GuiModule.execApplication()
	viewer=None
	print("All done")

# ////////////////////////////////////////////////
def MidxToIdx(args):
	parser = argparse.ArgumentParser(description="compress dataset")
	parser.add_argument("--DATASET"   , type=str,   required=True)
	parser.add_argument("--FIELD"     , type=str,   required=True,default="output=voronoi()")
	parser.add_argument("--TILE-SIZE" , type=str,   required=False,default="4*1024")
	parser.add_argument("--dataset"   , type=str,   required=True)
	args = parser.parse_args(args)

	# in case it's an expression
	args.TILE_SIZE=int(eval(args.TILE_SIZE))

	DATASET = LoadIdxDataset(args.DATASET)
	FIELD=DATASET.getFieldByName(args.FIELD)
	TIME=DATASET.getDefaultTime()
	Assert(FIELD.valid())

	# save the new idx file
	idxfile=DATASET.idxfile
	idxfile.filename_template = "" # //force guess
	idxfile.time_template = ""     #force guess
	idxfile.fields.clear()
	idxfile.fields.push_back(Field("DATA", FIELD.dtype, "rowmajor")) # note that compression will is empty in writing (at the end I will compress)
	idxfile.save(args.dataset)

	dataset = LoadIdxDataset(args.dataset)
	Assert(dataset)
	field=dataset.getDefaultField()
	time=dataset.getDefaultTime()
	Assert(field.valid())

	ACCESS = DATASET.createAccess()
	access = dataset.createAccess()

	print("Generating tiles...",args.TILE_SIZE)
	TILES = DATASET.generateTiles(args.TILE_SIZE)
	TOT_TILES=TILES.size()
	T1 = Time.now()
	for TILE_ID in range(TOT_TILES):
		TILE = TILES[TILE_ID]
		t1 = Time.now()
		buffer = DATASET.readFullResolutionData(ACCESS, FIELD, TIME, TILE)
		msec_read = t1.elapsedMsec()
		if not buffer: 
			continue

		t1 = Time.now()
		dataset.writeFullResolutionData(access, field, time, buffer, TILE)
		msec_write = t1.elapsedMsec()

		print("done", TILE_ID, "/", TOT_TILES, "msec_read", msec_read, "msec_write", msec_write)

	dataset.compressDataset("zip")
	print("ALL DONE IN", T1.elapsedMsec())

# ////////////////////////////////////////////////
def Main(args):

	if not args:
		return

	action=args[0].lower()
	action_args=args[1:]

	print("-m OpenVisus",action,action_args)

	# ___________________________________________________________________ openvisus utils

	if action=="configure" or action=="use-pyqt5":
		os.chdir(this_dir)
		UsePyQt5()
		print(action,"done")
		sys.exit(0)


	#example -m OpenVisus fix-range --dataset "D:\GoogleSci\visus_dataset\cat256\visus0.idx" 
	if action=="fix-range":
		FixRange(action_args)
		sys.exit(0)
		
	# example  -m OpenVisus server --port 10000 [--dataset D:\GoogleSci\visus_slam\TaylorGrant\VisusSlamFiles\visus.midx]
	if action=="server":
		RunServer(action_args)
		sys.exit(0)

	if action=="copy-dataset":
		CopyDataset(action_args)
		sys.exit(0)

	# -m OpenVisus compress-dataset --dataset "D:\GoogleSci\visus_dataset\cat256\visus0.idx" --compression zip
	if action=="compress-dataset":
		CompressDataset(action_args)
		sys.exit(0)

	# example: -m OpenVisus midx-to-idx --DATASET "D:\GoogleSci\visus_slam\TaylorGrant\VisusSlamFiles\visus.midx" --FIELD "output=voronoi()" --TILE-SIZE "4*1024" --dataset "tmp/test.idx"
	if action=="midx-to-idx":
		MidxToIdx(action_args)
		sys.exit(0)

	# !!!!!!!!!!!!! DEPRECATED !!!!!!!!!!!!!!!! (give a look to quick_tour)
	# example: -m OpenVisus convert create tmp/visus.idx --box "0 511 0 511" --fields "scalar uint8 default_compression(lz4) + vector uint8[3] default_compression(lz4)" 
	if action=="convert":
		VisusConvert().runFromArgs(action_args)
		sys.exit(0)

	# ___________________________________________________________________ test
	if action=="test":
		os.chdir(this_dir)
		ExecuteCommand([sys.executable, "Samples/python/Array.py"],check_result=True) 
		ExecuteCommand([sys.executable, "Samples/python/Dataflow.py"],check_result=True) 
		ExecuteCommand([sys.executable, "Samples/python/Dataflow2.py"],check_result=True) 
		ExecuteCommand([sys.executable, "Samples/python/Idx.py"],check_result=True) 
		ExecuteCommand([sys.executable, "Samples/python/XIdx.py"],check_result=True) 
		ExecuteCommand([sys.executable, "Samples/python/TestConvert.py"],check_result=True) 
		ExecuteCommand([sys.executable, "Samples/python/MinMax.py"],check_result=True) 
		ExecuteCommand([sys.executable, "-m","OpenVisus","server","--dataset","./datasets/cat/rgb.exit","--port","10000","--exit"],check_result=True) 
		sys.exit(0)

	if action=="test-viewer":
		os.chdir(this_dir)
		ExecuteCommand([sys.executable, "-m","OpenVisus","viewer" ],check_result=True) 
		ExecuteCommand([sys.executable, "-m","OpenVisus","viewer1"],check_result=True) 
		ExecuteCommand([sys.executable, "-m","OpenVisus","viewer2"],check_result=True) 
		sys.exit(0)

	if action=="test-jupyter":
		os.chdir(this_dir)
		ExecuteCommand([sys.executable, "-m","jupyter","nbconvert","--execute","./quick_tour.ipynb"],check_result=True) 
		ExecuteCommand([sys.executable, "-m","jupyter","nbconvert","--execute","./Samples/jupyter/Agricolture.ipynb"],check_result=True) 
		ExecuteCommand([sys.executable, "-m","jupyter","nbconvert","--execute","./Samples/jupyter/Climate.ipynb"],check_result=True) 
		# disabled: dependency on ipyvolume 
		# ExecuteCommand([sys.executable, "-m","jupyter","nbconvert","--execute","./Samples/jupyter/ReadAndView.ipynb"],check_result=True) 
		sys.exit(0)

	if action=="test-idx":
		os.chdir(this_dir)
		SelfTestIdx(300)
		sys.exit(0)
		
	# example python -m OpenVisus test-write-speed --filename "d:/~temp.bin" --blocksize "64*1024"
	if action=="test-write-speed":
		TestWriteSpeed(action_args)
		sys.exit(0)

	if action=="test-read-speed":
		TestReadSpeeds(action_args)
		sys.exit(0)

	# example -m OpenVisus test-network-speed  --nconnections 1 --nrequests 100 --url "http://atlantis.sci.utah.edu/mod_visus?from=0&to=65536&dataset=david_subsampled" 
	if action=="test-network-speed":
		TestNetworkSpeed(action_args)
		sys.exit(0)
		
	# example -m OpenVisus test-query-speed --dataset "D:\GoogleSci\visus_dataset\2kbit1\zip\rowmajor\visus.idx" --query-dim 512
	if action=="test-query-speed":
		TestQuerySpeed(action_args)
		sys.exit(0)
		
	# example -m OpenVisus test-slab-speed --dataset "D:\temp\test\test.idx" --dims "1024 1024 1024" --num-slabs 128 --dtype "int32" --layout ""
	if action=="test-slab-speed":
		TestSlabSpeed(action_args)
		sys.exit(0)
		
	# ___________________________________________________________________ gui

	# example: python -m OpenVisus viewer ....
	if action=="viewer":
		RunViewer(action_args)
		sys.exit(0)

	if action=="viewer1":
		ExecuteCommand([sys.executable,os.path.join(this_dir, "Samples", "python", "TestViewer1.py")]) 
		sys.exit(0)

	if action=="viewer2":
		ExecuteCommand([sys.executable,os.path.join(this_dir, "Samples", "python", "TestViewer2.py")]) 
		sys.exit(0)

	if action=="visible-human":
		ExecuteCommand([sys.executable,os.path.join(this_dir, "Samples", "python", "VisibleHuman.py")]) 
		sys.exit(0)

	raise Exception("unknown action",action)



# //////////////////////////////////////////
if __name__ == "__main__":
	Main(sys.argv[1:])