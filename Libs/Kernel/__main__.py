import os, sys, glob, subprocess, platform, shutil, sysconfig, re, argparse

from OpenVisus import *

# *** NOTE: this file must be self-contained ***

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
def Main(args):

	action=args[1] if len(args)>=2 else ""

	# no arguments
	if action=="":
		sys.exit(0)

	this_dir=os.path.dirname(os.path.abspath(__file__))

	# _____________________________________________
	if action=="dirname":
		print(this_dir)
		sys.exit(0)

	print("args",args)
	print("this_dir",this_dir)

	# _____________________________________________
	if action=="configure" or action=="use-pyqt5":
		os.chdir(this_dir)
		UsePyQt5()
		print(action,"done")
		sys.exit(0)

	# _____________________________________________
	if action=="test":
		
		os.chdir(this_dir)

		parser = argparse.ArgumentParser(description="test command.")
		parser.add_argument("--type",type=str, help="Type of  tests to enable", default="default") 
		args = parser.parse_args(args[2:])

		def RunTest(cmd):
			return ExecuteCommand([sys.executable] + cmd,check_result=True) 

		def RunJupyterTest(filename):
			return RunTest(["-m","jupyter","nbconvert","--execute",filename])

		if "all" in args.type or "default" in args.type:
			RunTest(["Samples/python/Array.py"])
			RunTest(["Samples/python/Dataflow.py"])
			RunTest(["Samples/python/Dataflow2.py"])
			RunTest(["Samples/python/Idx.py"])
			RunTest(["Samples/python/XIdx.py"])
			RunTest(["Samples/python/TestConvert.py"])
			RunTest(["Samples/python/MinMax.py"])
			RunTest(["-m","OpenVisus","server","--dataset","./datasets/cat/rgb.exit","--port","10000","--exit"])

		if "all" in args.type or "viewer" in args.type:
			RunTest(["-m","OpenVisus","viewer"])
			RunTest(["-m","OpenVisus","viewer1"])
			RunTest(["-m","OpenVisus","viewer2"])

		if "all" in args.type or "jupyter" in args.type:
			RunJupyterTest("./quick_tour.ipynb")
			RunJupyterTest("./Samples/jupyter/Agricolture.ipynb")
			RunJupyterTest("./Samples/jupyter/Climate.ipynb")

			# disabled: dependency on ipyvolume 
			# RunJupyterTest("./Samples/jupyter/ReadAndView.ipynb")

		sys.exit(0)

	# _____________________________________________
	if action=="test-idx":
		os.chdir(this_dir)
		SelfTestIdx(300)
		sys.exit(0)


	# _____________________________________________
	if action=="test-write-speed":
		# example python -m OpenVisus test-write-speed --filename "d:/~temp.bin" --blocksize "64*1024"
		os.chdir(this_dir)
		parser = argparse.ArgumentParser(description="Test IO write speed")
		parser.add_argument("--filename", type=str, help="Temporary filename", required=False,default="~temp.bin")
		parser.add_argument("--blocksize", type=str, help="Block size", required=False,default="64*1024")
		args = parser.parse_args(args[2:])
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
		sys.exit(0)

	# _____________________________________________
	if action=="test-read-speed":
		os.chdir(this_dir)
		parser = argparse.ArgumentParser(description="Test IO read speed")
		parser.add_argument("--filename", type=str, help="Temporary filename", required=False,default="~temp.bin")
		parser.add_argument("--blocksize", type=str, help="Block size", required=False,default="64*1024")
		args = parser.parse_args(args[2:])
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
		sys.exit(0)

	# _____________________________________________
	if action=="test-network-speed":
		# example -m OpenVisus test-network-speed  --nconnections 1 --nrequests 100 --url "http://atlantis.sci.utah.edu/mod_visus?from=0&to=65536&dataset=david_subsampled" 
		os.chdir(this_dir)
		parser = argparse.ArgumentParser(description="Test IO read speed")
		parser.add_argument("--nconnections", type=int, help="Number of connections", required=False,default=8)
		parser.add_argument("--nrequests", type=int, help="Number of Request", required=False,default=1000)
		parser.add_argument("--url", action='append', help="Url", required=True)
		args = parser.parse_args(args[2:])

		print("NetService speed test")
		print("nconnections",args.nconnections)
		print("nrequests",args.nrequests)
		print("urls\n","\n\t".join(args.url))
		NetService.testSpeed(args.nconnections,args.nrequests,args.url)

	# _____________________________________________
	if action=="convert":
		convert=VisusConvert()
		convert.runFromArgs(args[2:])
		sys.exit(0)

	# _____________________________________________
	if action=="server":

		# example
		# -m OpenVisus server --port 10000

		os.chdir(this_dir)

		parser = argparse.ArgumentParser(description="server command.")
		parser.add_argument("-p", "--port", type=int, help="Server port.", required=False,default=10000)
		parser.add_argument("-d", "--dataset", type=str, help="Idx file", required=False,default="")
		parser.add_argument("-e", "--exit", help="Exit immediately", action="store_true") # for debugging
		args = parser.parse_args(args[2:])

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
		sys.exit(0)

	if action=="copy-dataset":

		parser = argparse.ArgumentParser(description="copy-dataset command.")
		parser.add_argument("--src"       , type=str,   help="src dataset", required=True)
		parser.add_argument("--dst"       , type=str,   help="dst dataset", required=True)
		parser.add_argument("--src-time"  , type=float, help="src time",    required=False,default=None)
		parser.add_argument("--dst-time"  , type=float, help="dst time",    required=False,default=None)
		parser.add_argument("--src-field ", type=str,   help="src field",   required=False,default=None)
		parser.add_argument("--dst-field" , type=str,   help="dst field",   required=False,default=None)
		parser.add_argument("--src-access", type=str,   help="src access",  required=False,default=None)
		parser.add_argument("--dst-access", type=str,   help="dst access",  required=False,default=None)
		args = parser.parse_args(args[2:])

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


	# _____________________________________________
	if action=="viewer":

		# example: python -m OpenVisus viewer ....
		from VisusGuiPy import GuiModule,Viewer
		os.chdir(this_dir)
		viewer=Viewer()
		viewer.configureFromArgs(args[2:])
		GuiModule.execApplication()
		viewer=None
		print("All done")
		sys.exit(0)

	# _____________________________________________
	if action=="viewer1":
		ExecuteCommand([sys.executable,os.path.join(this_dir, "Samples", "python", "TestViewer1.py")]) 
		sys.exit(0)

	# _____________________________________________
	if action=="viewer2":
		ExecuteCommand([sys.executable,os.path.join(this_dir, "Samples", "python", "TestViewer2.py")]) 
		sys.exit(0)

	# _____________________________________________
	if action=="visible-human":
		ExecuteCommand([sys.executable,os.path.join(this_dir, "Samples", "python", "VisibleHuman.py")]) 
		sys.exit(0)

	# _____________________________________________
	if True:
		convert=VisusConvert()
		convert.runFromArgs(args[1:])
		sys.exit(0)


  

# //////////////////////////////////////////
if __name__ == "__main__":
	Main(sys.argv)




