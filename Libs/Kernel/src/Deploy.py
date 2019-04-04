import os
import sys
import subprocess
import glob
import shutil
import platform
import errno
import fnmatch
import os
import sysconfig
import re

bVerbose=False

WIN32=platform.system()=="Windows" or platform.system()=="win32"
APPLE=platform.system()=="Darwin"

# /////////////////////////////////////////////////
class DeployUtils:
	
	# ExecuteCommand
	@staticmethod
	def ExecuteCommand(cmd):	
		"""
		note: shell=False does not support wildcard but better to use this version
		because quoting the argument is not easy
		"""
		print("# Executing command: ",cmd)
		subprocess.call(cmd, shell=False)

	# GetCommandOutput
	@staticmethod
	def GetCommandOutput(cmd):
		output=subprocess.check_output(cmd)
		if sys.version_info >= (3, 0): output=output.decode("utf-8")
		return output.strip()

	# CreateDirectory
	@staticmethod
	def CreateDirectory(value):
		try: 
			os.makedirs(value)
		except OSError:
			if not os.path.isdir(value):
				raise
		
	# GetFilenameWithoutExtension
	@staticmethod
	def GetFilenameWithoutExtension(filename):
		return os.path.splitext(os.path.basename(filename))[0]

	# CopyFile
	@staticmethod
	def CopyFile(src,dst):
		
		src=os.path.realpath(src) 
		dst=os.path.realpath(dst)		
		
		if src==dst or not os.path.isfile(src):
			return		

		DeployUtils.CreateDirectory(os.path.dirname(dst))
		shutil.copyfile(src, dst)	
		
	# CopyDirectory
	@staticmethod
	def CopyDirectory(src,dst):
		
		src=os.path.realpath(src)
		
		if not os.path.isdir(src):
			return
		
		DeployUtils.CreateDirectory(dst)
		
		# problems with symbolic links so using shutil	
		dst=dst+"/" + os.path.basename(src)
		
		if os.path.isdir(dst):
			shutil.rmtree(dst,ignore_errors=True)
			
		shutil.copytree(src, dst, symlinks=True)				
		
	# ReadTextFile
	@staticmethod
	def ReadTextFile(filename):
		file = open("QT_VERSION", "r") 
		ret=file.read().strip()
		file.close()
		return ret
		
	# WriteTextFile
	@staticmethod
	def WriteTextFile(filename,content):
		if not isinstance(content, str):
			content="\n".join(content)+"\n"
		DeployUtils.CreateDirectory(os.path.dirname(os.path.realpath(filename)))
		file = open(filename,"wt") 
		file.write(content) 
		file.close() 		

	# ExtractNamedArgument
	#example arg:=--key='value...' 
	# ExtractNamedArgument("--key")
	@staticmethod
	def ExtractNamedArgument(key):
		for arg in sys.argv:
			if arg.startswith(key + "="):
				ret=arg.split("=",1)[1]
				if ret.startswith('"') or ret.startswith("'"): ret=ret[1:]
				if ret.endswith('"')   or ret.endswith("'"):   ret=ret[:-1]
				return ret
				
		return ""

	# RemoveFiles
	@staticmethod
	def RemoveFiles(pattern):
		files=glob.glob(pattern)
		print("Removing files",files)
		for it in files:
			if os.path.isfile(it):
				os.remove(it)
			else:
				shutil.rmtree(os.path.abspath(it),ignore_errors=True)		
				
	# RecursiveFindFiles
	# glob(,recursive=True) is not supported in python 2.x
	# see https://stackoverflow.com/questions/2186525/use-a-glob-to-find-files-recursively-in-python
	@staticmethod
	def RecursiveFindFiles(rootdir='.', pattern='*'):
	  return [os.path.join(looproot, filename)
	          for looproot, _, filenames in os.walk(rootdir)
	          for filename in filenames
	          if fnmatch.fnmatch(filename, pattern)]

	# PipInstall
	@staticmethod
	def PipInstall(args):
		cmd=[sys.executable,"-m","pip","install"] + args
		print("# Executing",cmd)
		return_code=subprocess.call(cmd)
		return return_code==0

	# PipUninstall
	@staticmethod
	def PipUninstall(args):
		cmd=[sys.executable,"-m","pip","uninstall","-y"] + args
		print("# Executing",cmd)
		return_code=subprocess.call(cmd)
		return return_code==0

	# InstallPyQt5
	@staticmethod
	def InstallPyQt5(QT_VERSION):
		try:
			from PyQt5.Qt import PYQT_VERSION_STR
			if PYQT_VERSION_STR==QT_VERSION:
				print("PyQt5",PYQT_VERSION_STR,"already installed")
				return
		except:
			pass
			
		# need to install a compatible version
		try:
			import PyQt5.QtCore
			if PyQt5.QtCore.QT_VERSION_STR==QT_VERSION:
				print("PyQt5",QT_VERSION,"already installed")
				return
		except:
			pass
			
		DeployUtils.PipUninstall(["PyQt5"])
		for name in ["PyQt5=="+QT_VERSION] + ["PyQt5=="+QT_VERSION[0:4]+"." + str(it) for it in reversed(range(1,10))]:
			if DeployUtils.PipInstall(["--user",name]):
				print("Installed",name)
				return
			print("Failed to install",name)	
			
		raise Exception("Cannot install PyQt5")

	# CopyQt5Plugins
	@staticmethod
	def CopyQt5Plugins(QT5_DIR):
		
		if WIN32:
			raise Exception("internal error")
				
		QT_PLUGIN_PATH=""
		for it in ["../../../plugins","../../qt5/plugins"]:
			if os.path.isdir(os.path.join(QT5_DIR,it)):
				QT_PLUGIN_PATH=os.path.join(QT5_DIR,it) 
				break

		if not QT_PLUGIN_PATH:
			raise Exception("internal error, cannot find Qt plugins","QT_PLUGIN_PATH",QT_PLUGIN_PATH)

		for it in ["iconengines","imageformats","platforms","printsupport","styles"]:
			if os.path.isdir(os.path.join(QT_PLUGIN_PATH,it)):
				DeployUtils.CopyDirectory(os.path.join(QT_PLUGIN_PATH,it) ,"bin/Qt/plugins")		

	# CreateScript
	@staticmethod
	def CreateScript(filename,env):
		
		def setEnv(Key,Value,bAdd=False):
			if WIN32:
				if bAdd: Value="{};%{}%".format(Value,Key)
				return "set {}={}".format(Key,Value)
			else:
				if bAdd: Value="{}:${}".format(Value,Key)
				return "export {}={}".format(Key,Value)
				
		def addEnv(Key,Value):
			return setEnv(Key,Value,bAdd=True)
		
		lines=[]
		if WIN32:
			lines+=[setEnv("this_dir","%~dp0")]
			lines+=[setEnv("PYTHON_EXECUTABLE",sys.executable)]
			lines+=[addEnv("PATH",";".join(["%this_dir%\\bin",os.path.dirname(sys.executable)]))]
			
			for key in env: 
				lines+=[setEnv(key,env[key])]
				
			lines+=["%this_dir%\\{} %*".format(filename)]
		else:	
			LD_LIBRARY_PATH="DYLD_LIBRARY_PATH" if APPLE else "LD_LIBRARY_PATH"
			LIBDIR=os.path.realpath(sysconfig.get_config_var("LIBDIR"))
			
			lines+=["#!/bin/bash"]
			lines+=["""this_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"""]
			lines+=[setEnv("PYTHON_EXECUTABLE",sys.executable)]
			lines+=[addEnv("PYTHONPATH",":".join(["${this_dir}"] + sys.path))]
			lines+=[addEnv(LD_LIBRARY_PATH,LIBDIR)]
			
			for key in env: 
				lines+=[setEnv(key,env[key])]
				
			lines+=["${this_dir}/%s $@" % (filename,) ]
		
		ext="bat" if WIN32 else ("command" if APPLE else "sh")
		script_filename="%s.%s" % (DeployUtils.GetFilenameWithoutExtension(filename),ext)
		print("Creating script",script_filename,"target",filename)
		DeployUtils.WriteTextFile(script_filename,lines)
			
		if not WIN32:
			subprocess.call(["chmod","+rx",script_filename], shell=False)	
			subprocess.call(["chmod","+rx",filename       ], shell=False)	
			

	# PythonDist
	@staticmethod
	def PythonDist():

		print("Executing dist",sys.argv)	
		DeployUtils.PipInstall(["--upgrade","--user","setuptools"])	
		DeployUtils.PipInstall(["--upgrade","--user","wheel"     ])	
		DeployUtils.RemoveFiles("dist/*")
		PYTHON_TAG="cp%s%s" % (sys.version_info[0],sys.version_info[1])
		if WIN32:
			PLAT_NAME="win_amd64"
		elif APPLE:
			PLAT_NAME="macosx_%s_x86_64" % (platform.mac_ver()[0][0:5].replace('.','_'),)	
		else:
			PLAT_NAME="manylinux1_x86_64" 

		print("Creating sdist...")
		DeployUtils.ExecuteCommand([sys.executable,"setup.py","-q","sdist","--formats=%s" % ("zip" if WIN32 else "gztar",)])
		sdist_ext='.zip' if WIN32 else '.tar.gz'
		__filename__ = glob.glob('dist/*%s' % (sdist_ext,))[0]
		sdist_filename=__filename__.replace(sdist_ext,"-%s-none-%s%s" % (PYTHON_TAG,PLAT_NAME,sdist_ext))
		os.rename(__filename__,sdist_filename)
		print("Created sdist",sdist_filename)

		print("Creating wheel...")
		DeployUtils.ExecuteCommand([sys.executable,"setup.py","-q","bdist_wheel","--python-tag=%s" % (PYTHON_TAG,),"--plat-name=%s" % (PLAT_NAME,)])
		wheel_filename=glob.glob('dist/*.whl')[0]
		print("Created wheel",wheel_filename)
			
		print("Finished dist",glob.glob('dist/*'))

# ///////////////////////////////////////
class AppleDeploy:
	
	"""
	see https://gitlab.kitware.com/cmake/community/wikis/doc/cmake/RPATH-handling
	
	NOTE: !!!! DYLD_LIBRARY_PATH seems to be disabled in Python dlopen for security reasons  !!!!

	(*) DYLD_LIBRARY_PATH - an environment variable which holds a list of directories

	(*) RPATH - a list of directories which is linked into the executable.
	    These can contain @loader_path and @executable_path.
	    To see the rpath type: 

	(*) builtin directories - /lib /usr/lib

	(*) DYLD_FALLBACK_LIBRARY_PATH - an environment variable which holds a list of directories


	To check :

		otool -L libname.dylib
		otool -l libVisusGui.dylib  | grep -i "rpath"

	To debug loading 

	DYLD_PRINT_LIBRARIES=1 QT_DEBUG_PLUGINS=1 visusviewer.app/Contents/MacOS/visusviewer
	"""
	
	# constructor
	def __init__(self):
		pass
		
	#findApps
	def findApps(self):
		ret=[]
		for it in glob.glob("bin/*.app"):
			bin="%s/Contents/MacOS/%s" % (it,DeployUtils.GetFilenameWithoutExtension(it))
			if os.path.isfile(bin):
				ret+=[bin]
		return ret
		
	# findFrameworks
	def findFrameworks(self):
		ret=[]
		for it in glob.glob("bin/*.framework"):
			file="%s/Versions/Current/%s" % (it,DeployUtils.GetFilenameWithoutExtension(it))  
			if os.path.isfile(os.path.realpath(file)):
				ret+=[file]
		return ret
		
	# findAllBinaries
	def findAllBinaries(self):	
		ret=[]
		ret+=DeployUtils.RecursiveFindFiles('bin', '*.dylib')
		ret+=DeployUtils.RecursiveFindFiles('bin', '*.so')
		ret+=self.findApps()
		ret+=self.findFrameworks()
		return ret
  
	# extractDeps
	def extractDeps(self,filename):
		output=DeployUtils.GetCommandOutput(['otool', '-L' , filename])
		lines=output.split('\n')[1:]
		deps=[line.strip().split(' ', 1)[0].strip() for line in lines]	
		# remove any reference to myself
		deps=[dep for dep in deps if os.path.basename(filename)!=os.path.basename(dep)]
		return deps
	

	# getLocal
	def getLocal(self,filename):
		key=os.path.basename(filename)
		return self.locals[key] if key in self.locals else None
				
	# addLocal
	def addLocal(self,local):
		
		# already added as local
		if self.getLocal(local): 
			return
		
		key=os.path.basename(local)
		
		if bVerbose: 
			print("# Adding local",key,"=",local)
		
		self.locals[key]=local
		
		deps=self.extractDeps(local)

		for dep in deps:
			self.addGlobal(dep)
				
		
	# addGlobal
	def addGlobal(self,dep):
		
		# already added as local
		if self.getLocal(dep):
			return
			
		# ignoring the OS system libraries
		bGlobal=os.path.isfile(dep) and dep.startswith("/") and not (dep.startswith("/System") or dep.startswith("/lib") or dep.startswith("/usr/lib"))
		if not bGlobal:
			return
			
		key=os.path.basename(dep)
		
		if bVerbose:
			print("# Adding global",dep,"=",dep)
					
		# special case for frameworks (I need to copy the entire directory)
		if ".framework" in dep:
			framework_dir=dep.split(".framework")[0]+".framework"
			DeployUtils.CopyDirectory(framework_dir,"bin")
			local="bin/" + os.path.basename(framework_dir) + dep.split(".framework")[1]
			
		elif dep.endswith(".dylib"):
			local="bin/" + os.path.basename(dep)
			DeployUtils.CopyFile(dep,local)
			
		else:
			raise Exception("Unknonw %s file file" % (dep,))	
			
		# now a global becomes a local one
		self.addLocal(local)
		

	# fixAllDeps
	def fixAllDeps(self):
		
		self.locals={}
		
		for local in self.findAllBinaries():
			self.addLocal(local)
			
		for filename in self.findAllBinaries():
			
			deps=self.extractDeps(filename)
			
			if bVerbose:
				print("")
				print("#/////////////////////////////")
				print("# Fixing",filename,"which has the following dependencies:")
				for dep in deps:
					print("#\t",dep)
				print("")
				
			def getBaseName(filename):
				if ".framework" in filename:
					# example: /bla/bla/ QtOpenGL.framework/Versions/5/QtOpenGL -> QtOpenGL.framework/Versions/5/QtOpenGL
					return os.path.basename(filename.split(".framework")[0]+".framework") + filename.split(".framework")[1]   
				else:
					return os.path.basename(filename)
			
			DeployUtils.ExecuteCommand(["chmod","u+w",filename])
			DeployUtils.ExecuteCommand(['install_name_tool','-id','@rpath/' + getBaseName(filename),filename])

			# example QtOpenGL.framework/Versions/5/QtOpenGL
			for dep in deps:
				local=self.getLocal(dep)
				if local: 
					DeployUtils.ExecuteCommand(['install_name_tool','-change',dep,"@rpath/"+ getBaseName(local),filename])

			DeployUtils.ExecuteCommand(['install_name_tool','-add_rpath','@loader_path',filename])
			
			# how to go from a executable in ./bin/** to ./bin
			A=os.path.realpath("./bin")
			B=os.path.dirname(os.path.realpath(filename))
			if A!=B: 
				N=len(B.split("/"))-len(A.split("/"))	
				DeployUtils.ExecuteCommand(['install_name_tool','-add_rpath','@loader_path' +  "/.." * N,filename])

	# addRPath
	def addRPath(self,value):
		for filename in self.findAllBinaries():
			DeployUtils.ExecuteCommand(["install_name_tool","-add_rpath",value,filename])
		
# ///////////////////////////////////////
class LinuxDeploy:

	"""
	If a shared object dependency does not contain a slash, then it is
	searched for in the following order:
	
		-  Using the directories specified in the DT_RPATH dynamic section
		attribute of the binary if present and DT_RUNPATH attribute does
		not exist.  Use of DT_RPATH is deprecated.
		
		-  Using the environment variable LD_LIBRARY_PATH, unless the
		executable is being run in secure-execution mode (see below), in
		which case this variable is ignored.
		
		-  Using the directories specified in the DT_RUNPATH dynamic section
		attribute of the binary if present.  Such directories are searched
		only to find those objects required by DT_NEEDED (direct
		dependencies) entries and do not apply to those objects' children,
		which must themselves have their own DT_RUNPATH entries.  This is
		unlike DT_RPATH, which is applied to searches for all children in
		the dependency tree.
		
		-  From the cache file /etc/ld.so.cache, which contains a compiled
		list of candidate shared objects previously found in the augmented
		library path.  If, however, the binary was linked with the -z
		nodeflib linker option, shared objects in the default paths are
		skipped.  Shared objects installed in hardware capability
		directories (see below) are preferred to other shared objects.
		
		-  In the default path /lib, and then /usr/lib.  (On some 64-bit
		architectures, the default paths for 64-bit shared objects are
		/lib64, and then /usr/lib64.)  If the binary was linked with the
		-z nodeflib linker option, this step is skipped.
	
	Rpath token expansion:

		The dynamic linker understands certain token strings in an rpath
		specification (DT_RPATH or DT_RUNPATH).  Those strings are
		substituted as follows:
	
		$ORIGIN (or equivalently ${ORIGIN})
		This expands to the directory containing the program or shared
		object.  Thus, an application located in somedir/app could be
		compiled with
		
			gcc -Wl,-rpath,'$ORIGIN/../lib'
		
			so that it finds an associated shared object in somedir/lib no
			matter where somedir is located in the directory hierarchy.
			This facilitates the creation of "turn-key" applications that
			do not need to be installed into special directories, but can
			instead be unpacked into any directory and still find their
			own shared objects.
		
		$LIB (or equivalently ${LIB})
		This expands to lib or lib64 depending on the architecture
		(e.g., on x86-64, it expands to lib64 and on x86-32, it
		expands to lib).
		
		$PLATFORM (or equivalently ${PLATFORM})
		This expands to a string corresponding to the processor type
		of the host system (e.g., "x86_64").  On some architectures,
		the Linux kernel doesn't provide a platform string to the
		dynamic linker.  The value of this string is taken from the
		AT_PLATFORM value in the auxiliary vector (see getauxval(3)).	

	To debug
	
		LD_DEBUG=libs,files ldd bin/visusviewer

		# this shows the rpath
		readelf -d bin/libVisusGui.so

	"""
	
	# constructor
	def __init__(self):
		pass

	# findApps
	def findApps(self):	
		return [it for it in glob.glob("bin/*") if os.path.isfile(it) and not os.path.splitext(it)[1]]
		
	# findAllBinaries
	def findAllBinaries(self):
		ret=[]
		ret+=DeployUtils.RecursiveFindFiles('bin', '*.so')
		ret+=self.findApps()
		return ret

	# extractDeps
	def extractDeps(self,filename):
		ret={}
		output=DeployUtils.GetCommandOutput(['ldd',filename])
		for line in output.splitlines():
			items=[it.strip() for it in line.split(" ") if len(it.strip())]
			if len(items)>=4 and items[1]=='=>' and items[2]!='not' and items[3]!='found':
				key=os.path.basename(items[0])
				target=os.path.realpath(items[2])
				if target.startswith("/"):
					ret[key]=target	
		return ret
		
	# findAllDeps
	def findAllDeps(self):
		ret={}
		for filename in self.findAllBinaries():
			for key,target in self.extractDeps(filename).items():
				ret[key]=target	
		return ret

	# copyGlobalDeps
	def copyGlobalDeps(self):
		"""
		Example:
		linux-vdso.so.1 =>  (0x00007ffc437f5000)
		libVisusKernel.so => /tmp/OpenVisus/build/install/bin/libVisusKernel.so (0x00007f15e31a7000)
		libpython3.6m.so.1.0 => not found
		libVisusIdx.so => /tmp/OpenVisus/build/install/bin/libVisusIdx.so (0x00007f15e2e60000)
		libVisusDb.so => /tmp/OpenVisus/build/install/bin/libVisusDb.so (0x00007f15e2b93000)
		libssl.so.1.0.0 => /tmp/OpenVisus/build/install/bin/libssl.so.1.0.0 (0x00007f15e291c000)	
		"""		
		for key,target in self.findAllDeps().items():
			
			# already inside
			if target.startswith(os.getcwd()):
				continue
			
			# if I copy linux libraries i will get core dump
			if target.startswith("/lib64") or target.startswith("/usr/lib64"):
				continue
			
			print("CopyFile",target,"bin/"+key," (fixing dependency of %s) " % (key,))
			DeployUtils.CopyFile(target,"bin/"+key)	

	# setRPath
	def setRPath(self,rpath):
		for filename in self.findAllBinaries():
			v=[rpath] if rpath else []
			v+=['$ORIGIN']
			
			A=os.path.realpath("./bin")
			B=os.path.dirname(os.path.realpath(filename))
			if A!=B:
				N=len(B.split("/"))-len(A.split("/"))	
				v+=['$ORIGIN' +  "/.." * N]			
			
			DeployUtils.ExecuteCommand(["patchelf", "--set-rpath", '"' + ":".join(v) + '"', filename])	

	# fixAllDeps
	def fixAllDeps(self):
		for I in range(2):
			self.copyGlobalDeps()
			self.setRPath("")




# ////////////////////////////////////////////////////////////////////////////////////////////////
if __name__ == '__main__':
	
	# ___________________________________________________________________
	if sys.argv[1]=="PythonDist":
		DeployUtils.PythonDist()
		sys.exit(0)
		
	# ___________________________________________________________________
	if sys.argv[1]=="OpenVisusCMakePostInstall":

		print("Executing OpenVisusCMakePostInstall",sys.argv)
		
		VISUS_GUI=True if os.path.isfile("QT_VERSION") else False
		
		if VISUS_GUI:
			QT5_DIR=DeployUtils.ExtractNamedArgument("--qt5-dir")
			if not os.path.isdir(QT5_DIR):
				raise Exception("Internal error")

		if WIN32:
			if not VISUS_GUI:
				raise Exception("Internal error") 
				
			windeploy=os.path.abspath(QT5_DIR+"/../../../bin/windeployqt")
			for exe in glob.glob("bin/*.exe"):
				DeployUtils.ExecuteCommand([windeploy,os.path.abspath(exe),
					"--libdir",os.path.abspath("bin"),
					"--plugindir",os.path.abspath("bin/Qt/plugins"),
					"--no-translations"])			
			
		else:
			if VISUS_GUI: 
				DeployUtils.CopyQt5Plugins(QT5_DIR)
				
			deploy=AppleDeploy() if APPLE else LinuxDeploy()
			deploy.fixAllDeps()


		print("Finished OpenVisusCMakePostInstall")
		sys.exit(0)
				

	print("Error in arguments",sys.argv)
	sys.exit(-1)
