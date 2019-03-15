
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

WIN32=platform.system()=="Windows" or platform.system()=="win32"
APPLE=platform.system()=="Darwin"

bVerbose=False

this_dir=os.path.dirname(os.path.realpath(__file__))
os.chdir(this_dir)

# /////////////////////////////////////////////////
def ExecuteCommand(cmd):	
	"""
	note: shell=False does not support wildcard but better to use this version
	because quoting the argument is not easy
	"""
	print("# Executing command: ",cmd)
	subprocess.call(cmd, shell=False)

# /////////////////////////////////////////////////
def GetCommandOutput(cmd):
	output=subprocess.check_output(cmd)
	if sys.version_info >= (3, 0): output=output.decode("utf-8")
	return output.strip()


# /////////////////////////////////////////////////
def CreateDirectory(value):
	try: 
		os.makedirs(value)
	except OSError:
		if not os.path.isdir(value):
			raise
	
# /////////////////////////////////////////////////
def GetFileNameWithoutExtension(filename):
	return os.path.splitext(os.path.basename(filename))[0]


# /////////////////////////////////////////////////
def CopyFile(src,dst):
	
	src=os.path.realpath(src) 
	dst=os.path.realpath(dst)		
	
	if src==dst or not os.path.isfile(src):
		return		

	CreateDirectory(os.path.dirname(dst))
	shutil.copyfile(src, dst)	
	
	
# /////////////////////////////////////////////////
def CopyDirectory(src,dst):
	
	src=os.path.realpath(src)
	
	if not os.path.isdir(src):
		return
	
	CreateDirectory(dst)
	
	# problems with symbolic links so using shutil	
	dst=dst+"/" + os.path.basename(src)
	
	if os.path.isdir(dst):
		shutil.rmtree(dst,ignore_errors=True)
		
	shutil.copytree(src, dst, symlinks=True)				
	
# /////////////////////////////////////////////////
def WriteTextFile(filename,content):
	if not isinstance(content, str):
		content="\n".join(content)+"\n"
	CreateDirectory(os.path.dirname(os.path.realpath(filename)))
	file = open(filename,"wt") 
	file.write(content) 
	file.close() 		

# ////////////////////////////////////////////////////////////////////
#example arg:=--key='value...' 
# ExtractNamedArgument("--key")
def ExtractNamedArgument(key):
	for arg in sys.argv:
		if arg.startswith(key + "="):
			ret=arg.split("=",1)[1]
			if ret.startswith('"') or ret.startswith("'"): ret=ret[1:]
			if ret.endswith('"')   or ret.endswith("'"):   ret=ret[:-1]
			return ret
			
	return ""


# ////////////////////////////////////////////////////////////////////
def RemoveFiles(pattern):
	files=glob.glob(pattern)
	print("Removing files",files)
	for it in files:
		if os.path.isfile(it):
			os.remove(it)
		else:
			shutil.rmtree(os.path.abspath(it),ignore_errors=True)		
			
# //////////////////////////////////////
# glob(,recursive=True) is not supported in python 2.x
# see https://stackoverflow.com/questions/2186525/use-a-glob-to-find-files-recursively-in-python
def RecursiveFindFiles(rootdir='.', pattern='*'):
  return [os.path.join(looproot, filename)
          for looproot, _, filenames in os.walk(rootdir)
          for filename in filenames
          if fnmatch.fnmatch(filename, pattern)]

# ////////////////////////////////////////////////////////////////////
def PipInstall(args):
	cmd=[sys.executable,"-m","pip","install"] + args
	print("# Executing",cmd)
	return_code=subprocess.call(cmd)
	return return_code==0

# ////////////////////////////////////////////////////////////////////
def PipUninstall(args):
	cmd=[sys.executable,"-m","pip","uninstall","-y"] + args
	print("# Executing",cmd)
	return_code=subprocess.call(cmd)
	return return_code==0


# ///////////////////////////////////////////////////////////////////////////
class Win32DeployStep:

	# fixAllDeps
	def fixAllDeps(self):

		VISUS_GUI=True if os.path.isfile("QT_VERSION") else False

		# i need qt for deployment
		if not VISUS_GUI:
			raise "Internal error"

		QT5_DIR=ExtractNamedArgument("--qt5-dir")

		if not os.path.isdir(QT5_DIR):
			raise Exception("internal error")		
		
		for exe in glob.glob("bin/*.exe"):
			ExecuteCommand([
				os.path.abspath(QT5_DIR+"/../../../bin/windeployqt"),
				os.path.abspath(exe),
				"--libdir",os.path.abspath("bin"),
				"--plugindir",os.path.abspath("bin/Qt/plugins"),
				"--no-translations"])


# ///////////////////////////////////////
class AppleDeployStep:
	
	"""
	see https://gitlab.kitware.com/cmake/community/wikis/doc/cmake/RPATH-handling

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
			bin="%s/Contents/MacOS/%s" % (it,GetFileNameWithoutExtension(it))
			if os.path.isfile(bin):
				ret+=[bin]
		return ret
		
	# findFrameworks
	def findFrameworks(self):
		ret=[]
		for it in glob.glob("bin/*.framework"):
			file="%s/Versions/Current/%s" % (it,GetFileNameWithoutExtension(it))  
			if os.path.isfile(os.path.realpath(file)):
				ret+=[file]
		return ret
		
	# findAllBinaries
	def findAllBinaries(self):	
		ret=[]
		ret+=RecursiveFindFiles('bin', '*.dylib')
		ret+=RecursiveFindFiles('bin', '*.so')
		ret+=self.findApps()
		ret+=self.findFrameworks()
		return ret
  
	# extractDeps
	def extractDeps(self,filename,bFull=False):
		output=subprocess.check_output(('otool', '-l' if bFull else '-L' , filename))
		if sys.version_info >= (3, 0): output=output.decode("utf-8")
		output=output.strip()
		if bFull: return output
		lines=output.split('\n')[1:]
		deps=[line.strip().split(' ', 1)[0].strip() for line in lines]	
		# remove any reference to myself
		deps=[dep for dep in deps if os.path.basename(filename)!=os.path.basename(dep)]
		return deps
	
	# showDeps
	def showDeps(self):
		for filename in self.findAllBinaries():
			output=subprocess.check_output(('otool', '-L', filename))
			if sys.version_info >= (3, 0): output=output.decode("utf-8")
			output=output.strip()
			print(output)
			
  	# getLocalName
	def getLocalName(self,filename):
		if ".framework" in filename:
			return os.path.basename(filename.split(".framework")[0]+".framework") + filename.split(".framework")[1]   
		else:
			return os.path.basename(filename)	
		
	# getLoaderPath
	def getLoaderPath(self,dw_filename):
		A=os.path.realpath("./bin")
		B=os.path.dirname(os.path.realpath(dw_filename))
		N=len(B.split("/"))-len(A.split("/"))	
		ret=['@loader_path']
		if N: ret+=['@loader_path' +  "/.." * N]
		return ret

	# changeAllDeps
	def changeAllDeps(self):
		
		for filename in self.findAllBinaries():
			
			deps=self.extractDeps(filename)
			
			if bVerbose:
				print("")
				print("#/////////////////////////////")
				print("# Fixing",filename,"which has the following dependencies:")
				for dep in deps:
					print("#\t",dep)
				print("")
			
			cmd=['install_name_tool']
			cmd+=['-id','@rpath/' + self.getLocalName(filename)]
			
			for dep in deps:
				local=self.getLocal(dep)
				if not local: continue
				# example QtOpenGL.framework/Versions/5/QtOpenGL
				cmd+=["-change",dep,"@rpath/"+ self.getLocalName(local)]

			for it in self.getLoaderPath(filename):
				cmd+=['-add_rpath',it]	
			
			cmd+=[filename]
				
			if True:
				ExecuteCommand(["chmod","u+w",filename])
				ExecuteCommand(cmd)	
			
	# getLocal
	def getLocal(self,filename):
		key=os.path.basename(filename)
		return self.locals[key] if key in self.locals else None
				
	# addLocal
	def addLocal(self,local):
		
		# already added
		if self.getLocal(local):
			return
		
		key=os.path.basename(local)
		if bVerbose: print("# Adding local",key,"=",local)
		self.locals[key]=local
		
		deps=self.extractDeps(local)

		for dep in deps:
			self.addGlobal(dep)
				
	# isGlobal
	def isGlobal(self,dep):	
		
		# only absolute path
		if not dep.startswith("/"):
			return False
		
		# ignore system libraries and built in directories
		if dep.startswith("/System") or dep.startswith("/lib") or dep.startswith("/usr/lib"):
			return False
			
		# ignore non existent dependencies
		if not os.path.isfile(dep):
			return False	
			
		return True			
				
	# addGlobal
	def addGlobal(self,dep):
		
		# already added
		if self.getLocal(dep):
			return
			
		if not self.isGlobal(dep):
			return
			
		key=os.path.basename(dep)
		
		if bVerbose:
			print("# Adding global",dep,"=",dep)
					
		# special case for frameworks (I need to copy the entire directory)
		if ".framework" in dep:
			framework_dir=dep.split(".framework")[0]+".framework"
			
			CopyDirectory(framework_dir,"bin")
			local="bin/" + os.path.basename(framework_dir) + dep.split(".framework")[1]
			
		elif dep.endswith(".dylib"):
			local="bin/" + os.path.basename(dep)
			CopyFile(dep,local)
			
		else:
			raise Exception("Unknonw %s file file" % (dep,))	
			
		self.addLocal(local)
		
	# fixAllDeps
	def fixAllDeps(self):

		self.locals={}
		for local in self.findAllBinaries():
			self.addLocal(local)

		self.changeAllDeps()
			
	# addRPath
	def addRPath(self,filename,Value):
		ExecuteCommand(["install_name_tool","-add_rpath",Value,filename])


# ///////////////////////////////////////
class LinuxDeployStep:

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

   Rpath token expansiono
   
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
		ret+=RecursiveFindFiles('bin', '*.so')
		ret+=self.findApps()
		return ret

	# extractDeps
	def extractDeps(self,filename,bFull=False):
		ret={}
		output=subprocess.check_output(('ldd',filename))
		if sys.version_info >= (3, 0): 
			output=output.decode("utf-8")
		output=output.strip()	
		if bFull: return output
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
			CopyFile(target,"bin/"+key)	

	# setRPath
	def setRPath(self,filename,value):
		ExecuteCommand(["patchelf", "--set-rpath", value, filename])	

	# showDeps
	def showDeps(self):
		for key,target in self.findAllDeps().items(): 
			# print only the 'outside' target
			if target.startswith(os.getcwd()) or os.path.isfile("bin/" + key):
				continue
			print("%30s" % (key,),target)

	# guessRPath
	def guessRPath(self,dw_filename):
		A=os.path.realpath("./bin")
		B=os.path.dirname(os.path.realpath(dw_filename))
		N=len(B.split("/"))-len(A.split("/"))	
		ret=['$ORIGIN']
		if N: ret+=['$ORIGIN' +  "/.." * N]
		return ":".join(ret)

	# fixAllDeps
	def fixAllDeps(self):
		for I in range(2):
			self.copyGlobalDeps()
			for filename in self.findAllBinaries():
				self.setRPath(filename,self.guessRPath(filename))



# ///////////////////////////////////////////////////////////////////////////
def CMakePostInstallStep():
	
	print("Executing cmake post install",sys.argv)

	if WIN32:
		Win32DeployStep().fixAllDeps()
		
	else:

		# copy plugins
		VISUS_GUI=True if os.path.isfile("QT_VERSION") else False
		if VISUS_GUI:

			QT5_DIR=ExtractNamedArgument("--qt5-dir")
			QT_PLUGIN_PATH=""
			for it in ["../../../plugins","../../qt5/plugins"]:
				if os.path.isdir(os.path.join(QT5_DIR,it)):
					QT_PLUGIN_PATH=os.path.join(QT5_DIR,it) 
					break

			if not QT_PLUGIN_PATH:
				raise Exception("internal error, cannot find Qt plugins","QT_PLUGIN_PATH",QT_PLUGIN_PATH)

			for it in ["iconengines","imageformats","platforms","printsupport","styles"]:
				if os.path.isdir(os.path.join(QT_PLUGIN_PATH,it)):
					CopyDirectory(os.path.join(QT_PLUGIN_PATH,it) ,"bin/Qt/plugins")

		if APPLE:
			AppleDeployStep().fixAllDeps()
	
		else:
			LinuxDeployStep().fixAllDeps()


	print("Finished cmake post install")

# ///////////////////////////////////////////////////////////////////////////
def CMakeDistStep():
	
	print("Executing dist step",sys.argv)	

	# create sdist and wheel
	# for conda I need to ignore any error
	PipInstall(["--upgrade","--user","setuptools"])	
	PipInstall(["--upgrade","--user","wheel"     ])	

	# remove any previous distibution
	RemoveFiles("dist/*")
	
	print("sys.version_info",sys.version_info)
	PYTHON_TAG="cp%s%s" % (sys.version_info[0],sys.version_info[1])

	if WIN32:
		PLAT_NAME="win_amd64"

	elif APPLE:
		print("platform.mac_ver()",platform.mac_ver())
		PLAT_NAME="macosx_%s_x86_64" % (platform.mac_ver()[0][0:5].replace('.','_'),)	

	else:
		# I' m calling the wheel with "PLAT_NAME="manylinux1_x86_64" " in case I want to install it locally (even if it's not portable)
		PLAT_NAME="manylinux1_x86_64" 

	# create sdist distribution
	if True:
		print("Creating sdist...")
		ExecuteCommand([sys.executable,"setup.py","-q","sdist","--formats=%s" % ("zip" if WIN32 else "gztar",)])
		sdist_ext='.zip' if WIN32 else '.tar.gz'
		__filename__ = glob.glob('dist/*%s' % (sdist_ext,))[0]
		sdist_filename=__filename__.replace(sdist_ext,"-%s-none-%s%s" % (PYTHON_TAG,PLAT_NAME,sdist_ext))
		os.rename(__filename__,sdist_filename)
		print("Created sdist",sdist_filename)

	# creating wheel distribution
	if True: 
		print("Creating wheel...")
		ExecuteCommand([sys.executable,"setup.py","-q","bdist_wheel","--python-tag=%s" % (PYTHON_TAG,),"--plat-name=%s" % (PLAT_NAME,)])
		wheel_filename=glob.glob('dist/*.whl')[0]
		print("Created wheel",wheel_filename)
		
	print("Finished dist step",glob.glob('dist/*'))				
		


# ////////////////////////////////////////////////////////////////////
def ConfigureStep():
	
	print("Executing configure step",sys.argv)
	
	VISUS_GUI=True if os.path.isfile("QT_VERSION") else False
	if VISUS_GUI:

		file = open("QT_VERSION", "r") 
		QT_VERSION=file.read().strip()
		file.close()
		print("QT_VERSION",QT_VERSION)

		if os.path.isdir(os.path.join(this_dir,"bin","Qt")) and not "--use-pyqt" in sys.argv:

			QT_BIN_PATH    = os.path.join(this_dir,"bin")
			QT_PLUGIN_PATH = os.path.join(this_dir,"bin","Qt","plugins")

		else:
			
			bInstalled=False
			
			try:
				from PyQt5.Qt import PYQT_VERSION_STR
				if PYQT_VERSION_STR==QT_VERSION:
					print("PyQt5",PYQT_VERSION_STR,"already installed")
					bInstalled=True
			except:
				pass
				
			if not bInstalled:
				# need to install a compatible version
				bPyQt5AlreadyInstalled=False
				try:
					import PyQt5.QtCore
					if PyQt5.QtCore.QT_VERSION_STR==QT_VERSION:
						print("PyQt5",QT_VERSION,"already installed")
						bPyQt5AlreadyInstalled=True
				except:
					pass
					
				if not bPyQt5AlreadyInstalled:
					PipUninstall(["PyQt5"])
					for name in ["PyQt5=="+QT_VERSION] + ["PyQt5=="+QT_VERSION[0:4]+"." + str(it) for it in reversed(range(1,10))]:
						if PipInstall(["--user",name]):
							print("Installed",name)
							break
						print("Failed to install",name)					

			import PyQt5
			print(dir(PyQt5))

			QT_BIN_PATH    = os.path.join(os.path.dirname(PyQt5.__file__),"Qt","bin" if WIN32 else "lib")
			QT_PLUGIN_PATH = os.path.join(os.path.dirname(PyQt5.__file__),"Qt","plugins")
			
			if WIN32:
				# see VisusGui.i (%pythonbegin section, I'm using sys.path)
				pass

			elif APPLE:
				rpath=QT_BIN_PATH
				deploy=AppleDeployStep()
				for filename in deploy.findAllBinaries():
					print("# Executing","deploy.addRPath('%s','%s')" % (filename,rpath,))
					deploy.addRPath(filename,rpath)	
			else:
				
				deploy=LinuxDeployStep()
				for filename in deploy.findAllBinaries():
					rpath=QT_BIN_PATH + ":" + deploy.guessRPath(filename)
					print("# Executing","deploy.setRPath('%s,'%s')" % (filename,rpath,))
					deploy.setRPath(filename, rpath)

			# avoid conflicts removing any Qt file
			RemoveFiles("bin/Qt*")

	if WIN32:
		for exe in glob.glob('bin/*.exe'):
			name=os.path.splitext(os.path.basename(exe))[0]
			script_filename="%s.bat" % (name,)
			WriteTextFile(script_filename,[
				"""set this_dir=%~dp0""",
				"""set PYTHON_EXECUTABLE=""" + sys.executable,
				"""set PATH=%this_dir%\\bin;""" + os.path.dirname(sys.executable) + """;%PATH%""",
				"""set PATH=""" + QT_BIN_PATH + """;%PATH%""" if VISUS_GUI else "",
				"""set QT_PLUGIN_PATH=""" + QT_PLUGIN_PATH if VISUS_GUI else "",
				"%this_dir%\\" + exe + " %*"])

	elif APPLE:	
	
		# PyQt5 is linked using rpath (see configure)
		for exe in glob.glob('bin/*.app'):
			name=os.path.splitext(os.path.basename(exe))[0]
			script_filename="%s.command" % (name,)
			WriteTextFile(script_filename,[
				"""#!/bin/bash""",
				"""this_dir=$(cd "$(dirname "${BASH_SOURCE[0]}" )" && pwd)""",
				"""export PYTHON_EXECUTABLE=""" + sys.executable,
				"""export PYTHONPATH=${this_dir}:""" + ':'.join(sys.path),
				"""export DYLD_LIBRARY_PATH=""" + os.path.realpath(sysconfig.get_config_var('LIBDIR')) + """:${DYLD_LIBRARY_PATH}""",
				"""export QT_PLUGIN_PATH=""" + QT_PLUGIN_PATH if VISUS_GUI else "",
				"${this_dir}/" + exe + "/Contents/MacOS/" + name + ' "$@"'])
			subprocess.call(["chmod","+rx",script_filename], shell=False)
		
	else:
		# PyQt5 is linked using rpath (see configure)
		for exe in glob.glob('bin/*'):
			if os.path.isfile(exe) and os.access(exe, os.X_OK) and not os.path.splitext(exe)[1]:
				name=os.path.splitext(os.path.basename(exe))[0]
				script_filename="%s.sh" % (name,)
				WriteTextFile(script_filename,[
					"""#!/bin/bash""",
					"""this_dir=$(cd "$(dirname "${BASH_SOURCE[0]}" )" && pwd)""",
					"""export PYTHON_EXECUTABLE=""" + sys.executable,
					"""export PYTHONPATH=${this_dir}:""" + ':'.join(sys.path),
					"""export LD_LIBRARY_PATH=""" + os.path.realpath(sysconfig.get_config_var('LIBDIR')) + """:${LD_LIBRARY_PATH}""",
					"""export QT_PLUGIN_PATH=""" + QT_PLUGIN_PATH if VISUS_GUI else "",
					"${this_dir}/" + exe + ' "$@"'])
				subprocess.call(["chmod","+rx",script_filename], shell=False)


	print("finished configure step")

# //////////////////////////////////////////////////////////////////////////////
if __name__ == "__main__":

	if "cmake_post_install" in sys.argv:
		CMakePostInstallStep()
		sys.exit(0)
		
	if "cmake_dist_step" in sys.argv:
		CMakeDistStep()
		sys.exit(0)
		
	ConfigureStep()
	sys.exit(0)


