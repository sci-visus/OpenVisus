import os, sys, glob, subprocess, platform, shutil, sysconfig, re, argparse

this_dir=os.path.dirname(os.path.abspath(__file__))

if len(sys.argv)>=2 and sys.argv[1]=="dirname":
	print(this_dir)
	sys.exit(0)

from OpenVisus import *

# for conda
try:
	import conda.cli 
except:
	pass

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
LINUX=not (WIN32 or APPLE)

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
	print("Executing command", cmd)
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
def Configure(bUserInstall=False):
	
	IS_CONDA = os.path.exists(os.path.join(sys.prefix, 'conda-meta', 'history')) # see https://stackoverflow.com/questions/47608532/how-to-detect-from-within-python-whether-packages-are-managed-with-conda
	IS_CPYTHON = not IS_CONDA
	CONDA_PREFIX=os.environ['CONDA_PREFIX'] if IS_CONDA else ""		
	VISUS_GUI=os.path.isfile("QT_VERSION")
	QT_VERSION=ReadTextFile("QT_VERSION") if VISUS_GUI else ""
	QT_MAJOR_VERSION,QT_MINOR_VERSION=QT_VERSION.split('.')[0:2] if VISUS_GUI else ("","")
	
	print("sys.executable",sys.executable,"VISUS_GUI",VISUS_GUI, "QT_VERSION", QT_VERSION, "IS_CONDA", IS_CONDA, "CONDA_PREFIX",CONDA_PREFIX)

	PACKAGES=['numpy']
	PACKAGES+=["pyqt={}.{}".format(QT_MAJOR_VERSION,QT_MINOR_VERSION)] if VISUS_GUI and IS_CONDA else []
	PACKAGES+=["PyQt5~={}.{}.0".format(QT_MAJOR_VERSION,QT_MINOR_VERSION),"PyQtWebEngine", "PyQt5-sip"] if VISUS_GUI and IS_CPYTHON else []
	PACKAGES+=["libglu"] if VISUS_GUI and IS_CONDA and LINUX else []	

	if IS_CONDA:
		cmd=['conda', 'install', '-y', '-c', 'conda-forge'] + PACKAGES
		conda.cli.main(*cmd)
	else:
		cmd=[sys.executable,"-m", "pip", "install", "--upgrade"]
		if bUserInstall: cmd.append("--user")
		cmd+=PACKAGES
		ExecuteCommand(cmd,check_result=True)
		
	# *** fix rpath ****
	# on windows it's enough to use sys.path (see *.i %pythonbegin section)
	# i don't have any RPATH way of modifying DLLs
	if WIN32:
		return

	# find Qt5 directory
	QT_LIB_DIR=""
	if VISUS_GUI:
		PyQt5_HOME=GetCommandOutput([sys.executable,"-c","import os,PyQt5;print(os.path.dirname(PyQt5.__file__))"]).strip() # this should cover the case where I just installed PyQt5
		print("PyQt5_HOME",PyQt5_HOME)
		Assert(os.path.isdir(PyQt5_HOME))
		QT_LIB_DIR="{}/lib".format(CONDA_PREFIX) if IS_CONDA else os.path.join(PyQt5_HOME,'Qt/lib')
		print("QT_LIB_DIR",QT_LIB_DIR)
		Assert(os.path.isdir(QT_LIB_DIR))

	if APPLE:
		dylibs=glob.glob("bin/*.dylib")
		so=glob.glob("*.so")
		apps=["%s/Contents/MacOS/%s"   % (it,GetFilenameWithoutExtension(it)) for it in glob.glob("bin/*.app")]
		all_bins=so + dylibs + apps
			
		# remove any reference to absolute Qt (it happens with brew which has absolute path), make it relocable with rpath as is in PyQt5
		for filename in all_bins:
			print("# FIXING",filename)
			if QT_VERSION:
				# example .../libQt5*.dylib -> @rpath/libQt5*.dylib
				if IS_CONDA:
					for Old in GetCommandOutput("otool -L %s | grep '.*/libQt5.*\.dylib' | awk '{print $1;}'" % filename, shell=True).splitlines():
						New="@rpath/libQt5" + Old.split("libQt5", 1)[1]
						ExecuteCommand(["install_name_tool","-change", Old, New, filename])
				# eample ../Qt*.framework -> @rpath/Qt*.framework
				else:
					for Old in GetCommandOutput("otool -L %s | grep '.*/Qt.*\.framework' | awk '{print $1;}'" % filename, shell=True).splitlines():
						New="@rpath/Qt" + Old.split("/Qt", 1)[1]
						ExecuteCommand(["install_name_tool","-change", Old, New, filename])	
			rpath=""
			rpath+="@loader_path/"
			rpath+=":@loader_path/../../../" if filename in apps else ":@loader_path/bin"
			rpath+=":" + QT_LIB_DIR if VISUS_GUI else ""
			SetRPath(filename, rpath)
		ShowDeps(all_bins)
		return
				
	if LINUX:
		for filename in glob.glob("*.so") + glob.glob("bin/*.so") + ["bin/visus","bin/visusviewer"]:
			if not os.path.isfile(filename): continue
			rpath=""
			rpath+="$ORIGIN"
			rpath+=":$ORIGIN/bin"
			rpath+=":" + QT_LIB_DIR if VISUS_GUI else ""
			SetRPath(filename,rpath)
		return

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

	src_time=src.getTime() if args.src_time is None else args.src_time
	dst_time=dst.getTime() if args.dst_time is None else args.dst_time   

	src_field=src.getField() if args.src_field is None else args.src_field
	dst_field=dst.getField() if args.dst_field is None else args.dst_field

	Assert(src_field.dtype==dst_field.dtype)

	src_access=src.createAccessForBlockQuery(StringTree.fromString(args.src_access))
	dst_access=dst.createAccessForBlockQuery(StringTree.fromString(args.dst_access))

	Dataset.copyDataset(
		dst, src_access, dst_field, dst_time,
		src, dst_access, src_field, src_time)


# ////////////////////////////////////////////////
def CompressDataset(args):
	parser = argparse.ArgumentParser(description="compress dataset")
	parser.add_argument("--dataset"       , type=str,   help="dataset",     required=True)
	parser.add_argument("--compression"   , type=str,   help="compression", required=True)
	args = parser.parse_args(args)

	db=LoadDataset(args.dataset);Assert(db)
	db.compressDataset([args.compression])


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
	access = idx.createAccess()

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

	if not args:
		return

	action=args[0].lower()
	action_args=args[1:]

	print("-m OpenVisus",action,action_args)

	if action=="configure":
		os.chdir(this_dir)
		Configure(bUserInstall="--user" in action_args)
		print(action,"done")
		print("WARNING", "if you get errors like:  module compiled against API version 0xc but this version of numpy is 0xa, then execute","conda update -y numpy")
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

	# example: -m OpenVisus midx-to-idx --midx "D:\GoogleSci\visus_slam\TaylorGrant\VisusSlamFiles\visus.midx" --field "output=voronoi()" --tile-size "4*1024" --idx "tmp/test.idx"
	if action=="midx-to-idx":
		MidxToIdx(action_args)
		sys.exit(0)

	# deprecated, old visus(convert)
	# example: -m OpenVisus convert create tmp/visus.idx --box "0 511 0 511" --fields "scalar uint8 default_compression(lz4) + vector uint8[3] default_compression(lz4)" 
	if action=="convert":
		VisusConvert().runFromArgs(action_args)
		sys.exit(0)

	# test
	if action=="test":
		os.chdir(this_dir)
		ExecuteCommand([sys.executable, "Samples/python/array.py"],check_result=True) 
		ExecuteCommand([sys.executable, "Samples/python/dataflow/dataflow1.py"],check_result=True) 
		ExecuteCommand([sys.executable, "Samples/python/dataflow/dataflow2.py"],check_result=True) 
		ExecuteCommand([sys.executable, "Samples/python/idx/read.py"],check_result=True) 
		ExecuteCommand([sys.executable, "-m","OpenVisus","server","--dataset","./datasets/cat/rgb.idx","--port","10000","--exit"],check_result=True) 
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
		TestNetworkSpeed(action_args)
		sys.exit(0)

	if action=="viewer":
		from OpenVisus.gui import PyViewer, GuiModule
		from PyQt5.QtWidgets import QApplication
		viewer=PyViewer()
		viewer.configureFromArgs(action_args)

		if "--render-palettes" in action_args:
			viewer.render_palettes=True

		QApplication.exec()
		viewer=None
		print("viewer done")
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

	if action=="jupyter":
		filename=action_args[0]
		ExecuteCommand([sys.executable,"-m","jupyter","nbconvert","--execute", filename])
		# import webbrowser
		# webbrowser.open("file://"+filename.replace(".ipynb",".html"))
		sys.exit(0)

	raise Exception("unknown action",action)



# //////////////////////////////////////////
if __name__ == "__main__":
	Main(sys.argv[1:])
