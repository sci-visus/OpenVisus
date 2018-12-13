
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

WIN32=platform.system()=="Windows" or platform.system()=="win32"
APPLE=platform.system()=="Darwin"

bVerbose=False


Qt5_DIR=""
OPENSSL_ROOT_DIR=""
PYTHON_TAG=""
PLAT_NAME=""
	
# /////////////////////////////////////////////////
def ExecuteCommand(cmd):	
	if bVerbose: print(" ".join(cmd))
	subprocess.call(cmd)	


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
	ExecuteCommand(["cp","-rf", src,dst])	
	
	
# /////////////////////////////////////////////////
def CopyDirectory(src,dst):
	
	src=os.path.realpath(src)
	
	if not os.path.isdir(src):
		return
	
	CreateDirectory(dst)
	
	# problems with symbolic links so using shutil
	# ExecuteCommand(["cp","-rf",src,dst])
	import shutil
	
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


# /////////////////////////////////////////////////
# glob(,recursive=True) is not supported in python 2.x
# see https://stackoverflow.com/questions/2186525/use-a-glob-to-find-files-recursively-in-python
def recursiveFindFiles(rootdir='.', pattern='*'):
  return [os.path.join(looproot, filename)
          for looproot, _, filenames in os.walk(rootdir)
          for filename in filenames
          if fnmatch.fnmatch(filename, pattern)]

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
		ret+=recursiveFindFiles('bin', '*.dylib')
		ret+=recursiveFindFiles('bin', '*.so')
		ret+=self.findApps()
		ret+=self.findFrameworks()
		return ret
  
	# extractDeps
	def extractDeps(self,filename):
		output=subprocess.check_output(('otool', '-L', filename))
		if sys.version_info >= (3, 0): output=output.decode("utf-8")
		output=output.strip()
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
			
	# relativeToBin
	def relativeToBin(self,filename):
		A=os.path.realpath("./bin")
		B=os.path.realpath(filename)
		N=len(os.path.dirname(B).split("/"))-len(A.split("/"))	
		return "/.." * N

  # getLocalName
	def getLocalName(self,filename):
		if ".framework" in filename:
			return os.path.basename(filename.split(".framework")[0]+".framework") + filename.split(".framework")[1]   
		else:
			return os.path.basename(filename)	
		
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
				
				if not local: 
					continue
					
				# example QtOpenGL.framework/Versions/5/QtOpenGL
				cmd+=["-change",dep,"@rpath/"+ self.getLocalName(local)]
				
			cmd+=['-add_rpath','@loader_path' + self.relativeToBin(filename)]	
			
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
	def addRPath(self,Value):
		for filename in self.findAllBinaries():
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
	
		LD_DEBUG=libs,files ldd install/bin/visusviewer

		# this shows the rpath
		readelf -d libVisusDataflow.so

	"""
	
	# constructor
	def __init__(self):
		pass

	# findApps
	def findApps(self):	
		return [it for it in glob.glob("bin/*") if os.path.isfile(it) and not os.path.splitext(it)[1]]
		
	# findAllBinaries
	def findAllBinaries(self):
		return glob.glob("bin/*.so")  + self.findApps()
		
	# findAllDeps
	def findAllDeps(self):
		ret={}
		for filename in self.findAllBinaries():
			output=subprocess.check_output(('ldd',filename))
			if sys.version_info >= (3, 0): 
				output=output.decode("utf-8")
			output=output.strip()	
			for line in output.splitlines():
				items=[it.strip() for it in line.split(" ") if len(it.strip())]
				if len(items)>=4 and items[1]=='=>' and items[2]!='not' and items[3]!='found':
					key=os.path.basename(items[0])
					target=os.path.realpath(items[2])
					if target.startswith("/"):
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

	# setOrigins
	def setOrigins(self):
		for filename in self.findAllBinaries():
			ExecuteCommand(["patchelf", "--set-rpath", "$ORIGIN", filename])	

	# showDeps
	def showDeps(self):
		for key,target in self.findAllDeps().items():
			# print only the 'outside' target
			if target.startswith(os.getcwd()) or os.path.isfile("bin/" + key):
				continue
			
			print("%30s" % (key,),target)



# ////////////////////////////////////////////////////////////////////
def PipMain(args):
	
	import pip
	if int(pip.__version__.split('.')[0])>9:
		from pip._internal import main
	else:
		from pip import main
		
	main(args)	
	

# ////////////////////////////////////////////////////////////////////
class PipPostInstall():
	
	# constructor
	def __init__(self):
		pass
		
	# findExecutables
	def findExecutables(self):
		
		if WIN32:
			return glob.glob('bin/*.exe')
			
		if APPLE:
			return glob.glob('bin/*.app')
			
		return [it for it in glob.glob('bin/*') if os.path.isfile(it) and not os.path.splitext(it)[1] ]


	# createScript
	def createScript(self,exe):
		
			name=GetFileNameWithoutExtension(exe)
			script_ext=".bat" if WIN32 else (".command" if APPLE else ".sh")
			script_filename="%s%s" % (name,script_ext)
			inner_exe='%s/Contents/MacOS/%s' % (exe,name) if APPLE else exe
			
			import PyQt5
			
			PYTHON_EXECUTABLE=sys.executable
			PYTHONPATH=":".join(sys.path)
			LD_LIBRARY_PATH=os.path.realpath(sysconfig.get_config_var("LIBDIR"))
			Qt5_DIR=os.path.join(os.path.dirname(PyQt5.__file__),"Qt")
			QT_PLUGIN_PATH=os.path.join(Qt5_DIR,"plugins")
			
			if WIN32:
				
				WriteTextFile(script_filename,[
					r'cd /d %~dp0',
					'set PYTHON_EXECUTABLE=%s' %(PYTHON_EXECUTABLE,),
					'set PYTHONPATH=$(pwd):%s' % (PYTHONPATH,),
					'set Qt5_DIR=%s' % (Qt5_DIR,),
					'set QT_PLUGIN_PATH=%s' % (QT_PLUGIN_PATH,),
					'set PATH=%s;%s;%s;%s' % (os.path.join(Qt5_DIR,"bin"),os.path.dirname(PYTHON_EXECUTABLE),r"%cd%\bin",r"%PATH%"),
					inner_exe + r' %*'
				])

			else:
		
				WriteTextFile(script_filename,[
					'#!/bin/bash',
					'this_dir=$(cd "$(dirname "${BASH_SOURCE[0]}" )" && pwd)',
					'cd ${this_dir}',
					'export PYTHON_EXECUTABLE=%s' %(PYTHON_EXECUTABLE,),
					'export PYTHONPATH=$(pwd):%s' % (PYTHONPATH,),
					'export Qt5_DIR=%s' % (Qt5_DIR,),
					'export QT_PLUGIN_PATH=%s' % (QT_PLUGIN_PATH,),
					'export %s=%s' % ("DYLD_LIBRARY_PATH" if APPLE else "LD_LIBRARY_PATH",LD_LIBRARY_PATH,),
					inner_exe + r' "$@"'
				])	
					
				#WriteTextFile("%s/Contents/Resources/qt.conf" % (app_path,),[
				#	"[Paths]",
				#	"  Plugins=%s" % (os.path.join(Qt5_DIR,"plugins"),)])				
			
			if not WIN32:
				subprocess.call("chmod +rx %s" % (script_filename,), shell=True)
					
	# run
	def run(self):
		
		old_dir = os.getcwd()
		os.chdir(os.path.dirname(os.path.realpath(__file__)))
		
		PipMain(["install", "-r","requirements.txt"])
		
		if WIN32:
			
			# TODO...
			pass
		
		elif APPLE:
			
			deploy=AppleDeployStep()
			import PyQt5
			Qt5_DIR=os.path.join(os.path.dirname(PyQt5.__file__),"Qt")
			deploy.addRPath(os.path.join(Qt5_DIR,"lib"))	
			

		# create *.command
		for exe in self.findExecutables():
			self.createScript(exe)
				
		
		os.chdir(old_dir)	

# //////////////////////////////////////////////////////////////////////////////
if __name__ == "__main__":
	
	I=1
	while I<len(sys.argv):

		# _____________________________________________
		if sys.argv[I]=="--Qt5_DIR":
			Qt5_DIR=sys.argv[I+1]
			I+=2; continue	
			
		# _____________________________________________
		if sys.argv[I]=="--OPENSSL_ROOT_DIR":
			OPENSSL_ROOT_DIR=sys.argv[I+1]
			I+=2; continue				
			
		# _____________________________________________
		if sys.argv[I]=="--python-tag":
			PYTHON_TAG=sys.argv[I+1]
			I+=2; continue		
			
		# _____________________________________________
		if sys.argv[I]=="--plat-name":
			PLAT_NAME=sys.argv[I+1]
			I+=2; continue								
		
		# _____________________________________________
		if sys.argv[I]=="--cmake-post-install":
			
			print("Executing --cmake-post-install...")
						
			if WIN32:
				ExecuteCommand([Qt5_DIR+"\\..\\..\\..\\bin\\windeployqt","bin\\visusviewer.exe","--libdir","bin","--plugindir","bin\\Qt\\plugins","--no-translations"])
				subprocess.call(r"rmdir /S /Q bin\Qt", shell=True)
				subprocess.call(r"del /F /S /Q bin\Qt*", shell=True)
				
			elif APPLE:
				AppleDeployStep().fixAllDeps()
				subprocess.call(r"rm -Rf bin/Qt*", shell=True)
				
			else:
				
				# see CMake/build_manylinux.sh for 'minimal' bundle libraries
				deploy=LinuxDeployStep()
				for I in range(2):
					# WRONG: for manylinux i should not copy the low-level dynamic libraries	
					# deploy.copyGlobalDeps()
					deploy.setOrigins()
					
				# WRONG: do not copy libpython
				# if I use manylinux libpython* I will have problems with 'built in' modules (such as math) that are different depending on the platform
				# subprocess.call("cp ..../libpython*  ./bin/", shell=True)
				
				if OPENSSL_ROOT_DIR:
					subprocess.call("cp " + OPENSSL_ROOT_DIR + "/lib/libcrypto.so* bin/", shell=True)
					subprocess.call("cp " + OPENSSL_ROOT_DIR + "/lib/libssl.so*    bin/", shell=True)			
					
			# create sdist and wheel
			PipMain(['install', "--user","--upgrade","setuptools","wheel"])	
			
			print("Creating sdist...")
			subprocess.call("%s setup.py -q sdist --formats=%s" % (sys.executable ,"zip" if WIN32 else "gztar"), shell=True)	
			
			print("Creating wheel...")
			subprocess.call("%s setup.py -q bdist_wheel --python-tag=%s --plat-name=%s" % (sys.executable ,PYTHON_TAG, PLAT_NAME), shell=True)	
			
			sdist_ext='.zip' if WIN32 else '.tar.gz'
			wheel_ext='.whl'
			
			sdist_filename=glob.glob('dist/*%s' % (sdist_ext,))[0]
			wheel_filename=glob.glob('dist/*%s' % (wheel_ext,))[0]
			os.rename(sdist_filename,wheel_filename.replace(wheel_ext,sdist_ext))
			
			print("Finished --cmake-post-install",glob.glob('dist/*'))
			I+=1; continue
					
		# _____________________________________________
		if sys.argv[I]=="--pip-post-install":
			
			print("Executing --pip-post-install....")
			PipPostInstall().run()
			print("finished --pip-post-install")
			I+=1; continue
					
		raise Exception("Unknown argument",sys.argv[I])

	sys.exit(0)
