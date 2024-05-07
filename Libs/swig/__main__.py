import os, sys, glob, subprocess, platform, shutil, sysconfig, re, argparse

this_dir=os.path.dirname(os.path.abspath(__file__))

if len(sys.argv)>=2 and sys.argv[1]=="dirname":
	print(this_dir)
	sys.exit(0)

import logging

from OpenVisus import *

WIN32=platform.system()=="Windows" or platform.system()=="win32"
APPLE=platform.system()=="Darwin"
LINUX=not (WIN32 or APPLE)

# /////////////////////////////////////////////////////////////////////////
def ExecuteCommand(cmd,shell=False,check_result=False, dry_run=False):	
	print(" ".join(cmd))
	if dry_run: return
	if check_result:
		return subprocess.check_call(cmd, shell=shell)
	else:
		return subprocess.call(cmd, shell=shell)


# ////////////////////////////////////////////////
"""
This configure step is the most buggy OpenVisus code.
Since OpenVisus viewer needs C++ Qt, I will try here to `replace` C++ Qt with PyQt5
but this has issue regarding portability on different platforms (Windows,Linux,OSX) and python versions (cpython, conda)
It should work in theory, but combinations are huge and a simple change in python version or PyQt version is a nightmare

To check if the software has been linking right:

Linux:
	readelf -d bin/visus
	QT_DEBUG_PLUGINS=1 LD_DEBUG=libs,files  ./visusviewer.sh
	LD_DEBUG=libs,files ldd bin/visus

OSX:
	otool -L libname.dylib
	otool -l libVisusGui.dylib  | grep -i "rpath"
	DYLD_PRINT_LIBRARIES=1 QT_DEBUG_PLUGINS=1 visusviewer.app/Contents/MacOS/visusviewer
"""

def Configure():

	# for macos arm64
	INSTALL_NAME_TOOL = os.environ.get("INSTALL_NAME_TOOL","install_name_tool")
	CODE_SIGN         = os.environ.get("CODE_SIGN",None)

	# /////////////////////////////////////////////////////////////////////////
	def GetFilenameWithoutExtension(filename):
		return os.path.splitext(os.path.basename(filename))[0]

	# /////////////////////////////////////////////////////////////////////////
	def ReadTextFile(filename):
		file = open(filename, "r") 
		ret=file.read().strip()
		file.close()
		return ret

	# /////////////////////////////////////////////////////////////////////////
	def GetCommandOutput(cmd,shell=False):
		# print("# GetCommandOutput", cmd)
		output=subprocess.check_output(cmd, shell = shell, universal_newlines=True)
		return output.strip()

	# ////////////////////////////////////////////////
	def MacOsExtractDeps(filename):
		output=GetCommandOutput(['otool', '-L' , filename])
		deps=[line.strip().split(' ', 1)[0].strip() for line in output.split('\n')[1:]]
		deps=[dep for dep in deps if os.path.basename(filename)!=os.path.basename(dep)] # remove any reference to myself
		return deps

	# ////////////////////////////////////////////////
	def MacOsShowDeps(all_bins):
		all_deps={}
		for filename in all_bins:
			for dep in MacOsExtractDeps(filename):
				all_deps[dep]=1
		all_deps=list(all_deps)
		all_deps.sort()	

		print("\n")
		print("# Final  dependencies (you shuold see no absolute deps)....")
		for filename in all_deps:
			print(f"#   {filename}")	

	# ////////////////////////////////////////////////
	def MacOsGetRPaths(filename):
		try:
			lines  = GetCommandOutput("otool -l '%s' | grep -A2 LC_RPATH | grep path " % filename, shell=True).splitlines()
		except:
			return []
		
		path_re = re.compile("^\s*path (.*) \(offset \d*\)$")
		return [path_re.search(line).group(1).strip() for line in lines]

	# ////////////////////////////////////////////////
	def MacOsSetRPath(filename, value, dry_run=False):

		for it in MacOsGetRPaths(filename):
			ExecuteCommand([INSTALL_NAME_TOOL,"-delete_rpath",it, filename], dry_run=dry_run)

		for it in value.split(":"):
			ExecuteCommand([INSTALL_NAME_TOOL,"-add_rpath", it, filename], dry_run=dry_run)
		

	user_install="--user"    in sys.argv
	dry_run     ="--dry-run" in sys.argv

	# _____________________________________________________
	VISUS_GUI=os.path.isfile("QT_VERSION")
	if VISUS_GUI:
		QT_VERSION=ReadTextFile("QT_VERSION")
		QT_MAJOR_VERSION,QT_MINOR_VERSION=QT_VERSION.split('.')[0:2]
		print(f"# VISUS_GUI={VISUS_GUI} QT_VERSION={QT_VERSION} QT_MAJOR_VERSION={QT_MAJOR_VERSION} QT_MINOR_VERSION={QT_MINOR_VERSION}")
	
	# _____________________________________________________
	# see https://stackoverflow.com/questions/47608532/how-to-detect-from-within-python-whether-packages-are-managed-with-conda
	IS_CONDA = os.path.exists(os.path.join(sys.prefix, 'conda-meta', 'history'))
	if IS_CONDA:
		CONDA_PREFIX=os.environ['CONDA_PREFIX']
		print(f"# IS_CONDA={IS_CONDA} CONDA_PREFIX={CONDA_PREFIX}")

		# install dependencies
		cmd=['install', '-y', '-c', 'conda-forge', 'numpy']
		cmd+=[f"pyqt={QT_MAJOR_VERSION}.{QT_MINOR_VERSION}"] if VISUS_GUI else ""
		cmd+=["libglu"] if LINUX else ""

		# for conda
		try:# I am specifying the version because any greater version fails on my Docker portable virtual env
			import conda.cli 
		except:
			pass

		# print("# OPENVISUS WARNING", "if you get errors like:  module compiled against API version 0xc but this version of numpy is 0xa, then execute","conda update -y numpy")
		if dry_run:
			ExecuteCommand(["conda"] + cmd, dry_run=dry_run)
		else:
			conda.cli.main(*cmd)

	else:

		# install dependencies
		pip_cmd=[sys.executable,"-m", "pip", "install"] + (["--user"] if user_install else [])

		ExecuteCommand(pip_cmd + ["--upgrade","pip"], check_result=False, dry_run=dry_run)

		# for some environment I need to stick with a specific version
		NUMPY_VERSION=os.environ.get("NUMPY_VERSION",None)
		ExecuteCommand(pip_cmd + [f"numpy=={NUMPY_VERSION}" if NUMPY_VERSION else "numpy"], check_result=False, dry_run=dry_run)

		 # False since it fails a lot !
		if VISUS_GUI:
			ExecuteCommand(pip_cmd + [f"PyQt5~={QT_MAJOR_VERSION}.{QT_MINOR_VERSION}.0", f"PyQtWebEngine~={QT_MAJOR_VERSION}.{QT_MINOR_VERSION}.0", f"PyQt5-sip"], check_result=False, dry_run=dry_run)
		
	# _____________________________________________________
	# on windows it's enough to use sys.path (see *.i %pythonbegin section)
	# i don't have any RPATH way of modifying DLLs
	if WIN32:
		return

	# _____________________________________________________
	if VISUS_GUI:
		
		# find PyQt5
		PyQt5_HOME=GetCommandOutput([sys.executable,"-c","import os,PyQt5;print(os.path.dirname(PyQt5.__file__))"]).strip() # this should cover the case where I just installed PyQt5
		print(f"# Found PyQt5_HOME={PyQt5_HOME}")
		Assert(os.path.isdir(PyQt5_HOME))

		# find Qt5 directory
		QT_LIB_DIR=os.environ.get("QT_LIB_DIR","")

		if not QT_LIB_DIR:

			if IS_CONDA:
				QT_LIB_DIR="{}/lib".format(CONDA_PREFIX)

			elif os.path.isdir(os.path.join(PyQt5_HOME,'Qt5/lib')):
				QT_LIB_DIR=os.path.join(PyQt5_HOME,'Qt5/lib')

			elif os.path.isdir(os.path.join(PyQt5_HOME,'Qt/lib')):
				QT_LIB_DIR=os.path.join(PyQt5_HOME,'Qt/lib')
			else:
				raise Exception("cannot find QT_LIB_DIR")

			print(f"# FOUND QT_LIB_DIR={QT_LIB_DIR}")

		Assert(os.path.isdir(QT_LIB_DIR))

	# _____________________________________________________
	if APPLE:
		dylibs=glob.glob("bin/*.dylib")
		so=glob.glob("*.so")
		apps=["%s/Contents/MacOS/%s"   % (it,GetFilenameWithoutExtension(it)) for it in glob.glob("bin/*.app")]
		all_bins=so + dylibs + apps
			
		# remove any reference to absolute Qt (it happens with brew which has absolute path), make it relocable with rpath as is in PyQt5
		for filename in all_bins:
			print("\n# fixing",filename)
			print(f"# OLD rpaths for {filename} value={MacOsGetRPaths(filename)}")
			
			if VISUS_GUI:

				# example .../libQt5*.dylib -> @rpath/libQt5*.dylib
				if IS_CONDA:
					for Old in GetCommandOutput("otool -L %s | grep '.*/libQt5.*\.dylib' | awk '{print $1;}'" % filename, shell=True).splitlines():
						New="@rpath/libQt5" + Old.split("libQt5", 1)[1]
						if Old!=New:
							ExecuteCommand([INSTALL_NAME_TOOL,"-change", Old, New, filename], dry_run=dry_run)
				
				# eample ../Qt*.framework -> @rpath/Qt*.framework
				else:
					for Old in GetCommandOutput("otool -L %s | grep '.*/Qt.*\.framework' | awk '{print $1;}'" % filename, shell=True).splitlines():
						New="@rpath/Qt" + Old.split("/Qt", 1)[1]
						if Old!=New:
							ExecuteCommand([INSTALL_NAME_TOOL,"-change", Old, New, filename], dry_run=dry_run)	
			rpath=""
			rpath+="@loader_path/"
			rpath+=":@loader_path/../../../" if filename in apps else ":@loader_path/bin"
			rpath+=":" + QT_LIB_DIR if VISUS_GUI else ""
			MacOsSetRPath(filename, rpath, dry_run=dry_run)

			if CODE_SIGN:
				ExecuteCommand([CODE_SIGN,"--force", "-s", "-", filename], dry_run=dry_run)

			print(f"# NEW rpaths for {filename} value={MacOsGetRPaths(filename)}")

		MacOsShowDeps(all_bins)
		return
				
	# _____________________________________________________
	if LINUX:
		for filename in glob.glob("*.so") + glob.glob("bin/*.so") + ["bin/visus","bin/visusviewer"]:
			if not os.path.isfile(filename): continue
			rpath=""
			rpath+="$ORIGIN"
			rpath+=":$ORIGIN/bin"
			rpath+=f":{QT_LIB_DIR}" if VISUS_GUI else ""
			ExecuteCommand(["patchelf","--set-rpath",rpath, filename], dry_run=dry_run)
		return


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
		args.time=db.getTime()

	if args.field is None:
		args.field=db.getField().name

	field=db.getField(args.field)

	# compute the ranges
	N = field.dtype.ncomponents()
	ranges=[[+sys.float_info.max,-sys.float_info.max]] * N
			
	access = db.createAccessForBlockQuery()
	access.beginRead()
	for block_id in range(db.getTotalNumberOfBlocks()):

		read_block = db.createBlockQuery(block_id, field, args.time)

		if not db.executeBlockQueryAndWait(access, read_block):
			continue

		# i don't care about the layout

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
		config=ConfigFile.fromString("<visus><dataset name='default' url='{}' /></visus>".format(args.dataset))
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
def MidxToIdx(args):

	parser = argparse.ArgumentParser(description="compress dataset")
	parser.add_argument("--midx"   ,"--DATASET", type=str,  required=True)
	parser.add_argument("--field"     , type=str,   required=False,default="output=voronoi()")
	parser.add_argument("--tile-size" , type=str,   required=False,default="4*1024")
	parser.add_argument("--idx", "--dataset", type=str, dest="dataset",  required=True)
	args = parser.parse_args(args)

	# in case it's an expression
	args.tile_size=int(eval(args.tile_size))

	midx = LoadDataset(args.midx)
	FIELD=midx.getField(args.field)
	TIME=midx.getTime()
	Assert(FIELD.valid())

	# save the new idx file
	idxfile=midx.idxfile

	# disable compression, at the end I will compress
	idxfile.fields.clear()
	idxfile.fields.push_back(Field("DATA", FIELD.dtype)) 

	# force validation
	idxfile.version=0 
	idxfile.save(args.dataset)

	idx = LoadDataset(args.dataset)
	Assert(idx)
	field=idx.getField()
	time=idx.getTime()
	Assert(field.valid())

	ACCESS = midx.createAccess()
	access = idx.createAccessForBlockQuery()
	access.disableWriteLocks()
	access.disableCompression()

	print("Generating tiles...",args.tile_size)
	pdim=idx.getPointDim()
	tile_size=args.tile_size
	TILES=[]
	if pdim==2:
		(X1,Y1),(X2,Y2)=midx.getLogicBox()
		for Y in range(Y1,Y2,tile_size):
			for X in range(X1,X2,tile_size):
				TILES.append(([X,Y],[min([X2,X+tile_size]),min([Y2,Y+tile_size])]))

	elif pdim==3:
		(X1,Y1,Z1),(X2,Y2,Z2)=midx.getLogicBox()
		for Z in range(Z1,Z2,tile_size):
			for Y in range(Y1,Y2,tile_size):
				for X in range(X1,X2,tile_size):
					TILES.append(([X,Y,Z],[min([X2,X+tile_size]),min([Y2,Y+tile_size]),min([Z2,Z+tile_size])]))
	else:
		raise Exception("not supported")

	T1 = Time.now()
	for I,TILE in enumerate(TILES):

		print("Generating tile",I,"of",len(TILES),TILE,"...")

		t1 = Time.now()
		buffer=None
		try:
			buffer = midx.read(access=ACCESS, field=FIELD, time=TIME, logic_box=TILE) 
		except:
			buffer=None # can fail because that area is empty
			
		msec_read = t1.elapsedMsec()
		if buffer is None: continue

		t1 = Time.now()
		idx.write(buffer, access=access, field=field, time=time, logic_box=TILE)
		msec_write = t1.elapsedMsec()

		print("Generated tile", I, "/", len(TILES), "msec_read", msec_read, "msec_write", msec_write)

	idx.compressDataset(["zip"])
	print("ALL DONE IN", T1.elapsedMsec())


# ////////////////////////////////////////////////
def Main(args):
	import argparse
	from OpenVisus import LoadDataset


	if not args:
		return

	import logging
	from OpenVisus import SetupLogger
	SetupLogger(logging.getLogger("OpenVisus"))

	action=args[0].lower()
	action_args=args[1:]

	print(f"\n# OpenVisus Main")
	print(f"# {sys.executable }-m OpenVisus",action,action_args)

	if action=="configure":
		os.chdir(this_dir)
		Configure()
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

	# DEPRECATED, I think you can do the same with the new convert code
	# -m OpenVisus cpp-compress-dataset --dataset "D:\GoogleSci\visus_dataset\cat256\visus0.idx" --compression zip
	if action=="cpp-compress-dataset":
		parser = argparse.ArgumentParser(description="compress dataset")
		parser.add_argument("--dataset"       , type=str,   help="dataset",     required=True)
		parser.add_argument("--compression"   , type=str,   help="compression", required=True)
		args = parser.parse_args(action_args)
		db=LoadDataset(args.dataset);Assert(db)
		db.compressDataset([args.compression])
		sys.exit(0)

	# DEPRECATED, I think you can do the same with the new convert code
	# example: -m OpenVisus midx-to-idx --midx "D:\GoogleSci\visus_slam\TaylorGrant\VisusSlamFiles\visus.midx" --field "output=voronoi()" --tile-size "4*1024" --idx "tmp/test.idx"
	if action=="midx-to-idx":
		MidxToIdx(action_args)
		sys.exit(0)

	# DEPRECATED, old visus(convert)
	# example: -m OpenVisus convert create tmp/visus.idx --box "0 511 0 511" --fields "scalar uint8 default_compression(lz4) + vector uint8[3] default_compression(lz4)" 
	if action=="convert":
		VisusConvert().runFromArgs(action_args)
		sys.exit(0)

	# DEPRACATED (are we sure?)
	# -m OpenVisus copy-blocks  --src http://atlantis.sci.utah.edu/mod_visus?dataset=2kbit1 --dst D:/tmp/visus.idx   --num-threads 4 --num-read-per-request 256 [--verbose] [--field fieldname]  [--time timestep]
	if action=="copy-blocks":
		parser = argparse.ArgumentParser(description="Copy blocks")
		parser.add_argument("--src","--source", type=str, help="source", required=True,default="")
		parser.add_argument("--dst","--destination", type=str, help="destination", required=True,default="") 
		parser.add_argument("--time", help="time", required=False,default="") 
		parser.add_argument("--field", help="field"  , required=False,default="") 
		parser.add_argument("--num-threads", help="number of threads", required=False,type=int, default=4)
		parser.add_argument("--num-read-per-request", help="number of read block per network request (only for mod_visus)", required=False,type=int, default=16)
		parser.add_argument("--verbose", help="Verbose", required=False,action='store_true') 
		args = parser.parse_args(action_args)


		db=LoadDataset(args.src)
		logger.info(f"copy-blocks time={args.time}, field={args.field}, num_threads={args.num_threads} num_read_per_request={args.num_read_per_request}, verbose={args.verbose}")
		db.copyBlocks(dst=args.dst, time=args.time, field=args.field, num_threads=args.num_threads,num_read_per_request=args.num_read_per_request, verbose=args.verbose)
		sys.exit(0)

	# NEW: convert-image-stack
	if action=="convert-image-stack":
		"""
		# source is a python glob expression
		SRC="/tmp/image-stack/**/*.png" 
		DST=/mnt/d/GoogleSci/visus_dataset/cat/modvisus/visus.idx 
		python3 -m OpenVisus convert-image-stack --arco [ modvisus | 1mb | ...] [--access-config ...] ${SRC} {$DST}
		"""
		parser = argparse.ArgumentParser(description=action)
		parser.add_argument('--arco',type=str,required=False,default="modvisus")
		parser.add_argument('--access-config',type=str,required=False)
		parser.add_argument('src',type=str)
		parser.add_argument('dst',type=str)
		args=parser.parse_args(action_args)
		from OpenVisus import ConvertImageStack
		ConvertImageStack(src=args.src, dst=args.dst, arco=args.arco, access_config=args.access_config)
		sys.exit(0)

	if action=="copy-dataset":
		"""
		python -m OpenVisus copy-dataset --arco 1mb C:\data\visus-datasets\2kbit1\modvisus\visus.idx C:\data\visus-datasets\2kbit1\remove-me\visus.idx
		"""
		import argparse
		parser = argparse.ArgumentParser(description=action)
		parser.add_argument('--arco',type=str, required=False,default="modvisus")
		parser.add_argument('--tile-size',type=int, required=False,default=None)
		parser.add_argument('--timestep',type=str, required=False,default=None)
		parser.add_argument('--field',type=str, required=False,default=None)
		parser.add_argument("src",type=str) 
		parser.add_argument("dst",type=str)
		args=parser.parse_args(action_args)
		db=LoadDataset(args.src)
		db.copyDataset(args.dst, arco=args.arco,tile_size=args.tile_size,timestep=args.timestep,field=args.field)
		sys.exit(0)

	if action=="compress-dataset":
		"""
		# note DST must be a local db because a lot of file seek and write will happen
		IDX=/mnt/d/GoogleSci/visus_dataset/arco/cat/modvisus/visus.idx 
		python3 -m OpenVisus compress-dataset ${IDX}
		"""
		import argparse
		parser = argparse.ArgumentParser(description=action)
		parser.add_argument('--compression',type=str,required=False,default="zip")
		parser.add_argument('--num-threads',type=int,required=False,default=32)
		parser.add_argument('--timestep',type=int,required=False,default=None)
		parser.add_argument('--field',type=str,required=False,default=None)
		parser.add_argument('idx_filename',type=str) 
		args=parser.parse_args(action_args)
		db=LoadDataset(args.idx_filename)
		db.compressDataset(compression=args.compression, num_threads=args.num_threads,timestep=args.timestep,field=args.field)
		return

	if action=="copy-dataset-to-cloud":
		"""
		Example
		SET AWS_PROFILE=wasabi
		python -m OpenVisus copy-dataset-to-cloud ^
				--source C:\data\visus-datasets\2kbit1\modvisus\visus.idx ^
				--local \tmp\visus-datasets\arco\1mb\2kbit1\visus.idx ^
				--remote s3://visus-datasets/arco/1mb/2kbit1/visus.idx ^
				--done   s3://visus-datasets/arco/1mb/2kbit1/visus.idx.done ^
				--arco 1mb ^
				--compression zip
		"""
  
		import argparse
		parser = argparse.ArgumentParser(description="copy-dataset-to-cloud")
		parser.add_argument('--source'      ,type=str, required=True)
		parser.add_argument('--local'       ,type=str, required=True)
		parser.add_argument('--remote'      ,type=str, required=True)
		parser.add_argument('--done'        ,type=str, required=False)
		parser.add_argument("--arco", type=str, default="1mb",required=False)
		parser.add_argument("--compression", type=str, default="zip",required=False)
		parser.add_argument("--timestep", type=int, default=None,required=False)
		parser.add_argument("--field", type=str, default=None,required=False)
		parser.add_argument("--clean-local", action='store_true',required=False)
		args=parser.parse_args(action_args)
		from OpenVisus import LoadDataset
		db=LoadDataset(args.source)
		db.copyDatasetToCloud(local=args.local, remote=args.remote, done=args.done, arco=args.arco, compression=args.compression, timestep=args.timestep, field=args.field, clean_local=args.clean_local)
		logger.info(f"copy-dataset-to-cloud {args.source} DONE")
		sys.exit(0)

	if action=="cloud-du":
		"""
		Example: 
		SET AWS_PROFILE=wasabi     
		python -m OpenVisus cloud-du s3://foam
		"""
		from OpenVisus.s3 import S3
		prefix=action_args[0]
		s3=S3(num_connections=32)
		s3.computeStatistics(prefix)
		sys.exit(0)

	# //////////////////////////////////////////
	if action=="viewer":
		from OpenVisus.gui import PyViewer, GuiModule
		from PyQt5.QtWidgets import QApplication
		viewer=PyViewer()
		viewer.configureFromArgs(action_args)
		viewer.render_palettes="--render-palettes" in action_args
		QApplication.exec()
		viewer=None
		print("viewer done")
		sys.exit(0)

	if action=="jupyter":
		filename=action_args[0]
		ExecuteCommand([sys.executable,"-m","jupyter","nbconvert","--execute", filename]) # ipynb -> html 
		# import webbrowser
		# webbrowser.open("file://"+filename.replace(".ipynb",".html"))
		sys.exit(0)

	# ////////////////////////////////////////// TEST SECTION

	# test
	if action=="test":
		os.chdir(this_dir)
		ExecuteCommand([sys.executable, "Samples/python/array.py"],check_result=True) 
		ExecuteCommand([sys.executable, "Samples/python/dataflow/dataflow1.py"],check_result=True) 
		ExecuteCommand([sys.executable, "Samples/python/dataflow/dataflow2.py"],check_result=True) 
		ExecuteCommand([sys.executable, "Samples/python/idx/read.py"],check_result=True) 
		ExecuteCommand([sys.executable, "-m","OpenVisus","server","--dataset","./datasets/cat/rgb.idx","--port","10000","--exit"],check_result=True) 
		ExecuteCommand([sys.executable, "Samples/python/test_api.py"],check_result=True) 
		sys.exit(0)

	# test-full
	if action=="test-full":
		os.chdir(this_dir)
		ExecuteCommand([sys.executable, "Samples/python/array.py"],check_result=True) 
		ExecuteCommand([sys.executable, "Samples/python/dataflow/dataflow1.py"],check_result=True) 
		ExecuteCommand([sys.executable, "Samples/python/dataflow/dataflow2.py"],check_result=True) 
		ExecuteCommand([sys.executable, "Samples/python/idx/read.py"],check_result=True) 
		ExecuteCommand([sys.executable, "Samples/python/idx/convert.py"],check_result=True)  # this needs pillow (import PIL)
		ExecuteCommand([sys.executable, "Samples/python/wavelets/filters.py"],check_result=True) # this needs pillow (import PIL) 
		ExecuteCommand([sys.executable, "Samples/python/idx/speed.py"],check_result=True) # this is very slow
		ExecuteCommand([sys.executable, "-m","OpenVisus","server","--dataset","./datasets/cat/rgb.idx","--port","10000","--exit"],check_result=True) 
		sys.exit(0)

	if action=="test-idx":
		os.chdir(this_dir)
		SelfTestIdx()
		sys.exit(0)

	# this is just to test run-time linking of shared libraries
	if action=="test-gui":
		if os.path.isfile(os.path.join(this_dir,"QT_VERSION")): 
			from OpenVisus.VisusGuiPy import GuiModule
			print("test-gui ok")
		else:
			print("Skipping test-gui since it is a non-gui distribution")
		sys.exit(0)
		
	# example -m OpenVisus test-network-speed  --nconnections 1 --nrequests 100 --url "http://atlantis.sci.utah.edu/mod_visus?from=0&to=65536&dataset=david_subsampled" 
	if action=="test-network-speed":
		parser = argparse.ArgumentParser(description="Test IO read speed")
		parser.add_argument("--nconnections", type=int, help="Number of connections", required=False,default=8)
		parser.add_argument("--nrequests", type=int, help="Number of Request", required=False,default=1000)
		parser.add_argument("--url", action='append', help="Url", required=True)
		args = parser.parse_args(action_args)
		print("NetService speed test")
		print("nconnections",args.nconnections)
		print("nrequests",args.nrequests)
		print("urls\n","\n\t".join(args.url))
		NetService.testSpeed(args.nconnections,args.nrequests,args.url)
		sys.exit(0)

	if action=="test-viewer":
		os.chdir(this_dir)
		from OpenVisus.gui import PyViewer, GuiModule
		from PyQt5.QtWidgets import QApplication
		viewer=PyViewer()
		viewer.configureFromArgs(action_args)
		QApplication.exec()
		viewer=None
		sys.exit(0)

	if action=="test-viewer1":
		os.chdir(this_dir)
		ExecuteCommand([sys.executable,os.path.join(this_dir, "Samples/python/viewer/viewer1.py")]) 
		sys.exit(0)

	if action=="test-viewer2":
		os.chdir(this_dir)
		ExecuteCommand([sys.executable,os.path.join(this_dir, "Samples/python/viewer/viewer2.py")]) 
		sys.exit(0)

	if action=="test-two-viewers":
		os.chdir(this_dir)
		ExecuteCommand([sys.executable,os.path.join(this_dir, "Samples/python/viewer/two_viewers.py")]) 
		sys.exit(0)

	raise Exception("unknown action",action)


# //////////////////////////////////////////
if __name__ == "__main__":
	Main(sys.argv[1:])
