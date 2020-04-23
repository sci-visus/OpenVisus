import sys
import traceback
from PyUtils import *


"""
Fix the problem about shared library path finding

on windows: it seems sufficient to modify the sys.path before importing the module.
Example (see __init__.py):

on osx sys.path does not seem to work. Even changing the DYLIB_LIBRARY_PATH
seems not to work. SO the only viable solution seems to be to modify the Rpath
(see AppleDeploy)


on linux sys.path does not seem to work. I didnt' check if LD_LIBRARY_PATH
is working or not. To be coherent with OSX I'm using the rpath
"""


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


	# __findAllBinaries
	def __findAllBinaries(self):	
		ret=[]
		
		ret+=RecursiveFindFiles('.', '*.so')
		ret+=RecursiveFindFiles('.', '*.dylib')
		
		# apps
		for it in glob.glob("bin/*.app"):
			bin="%s/Contents/MacOS/%s" % (it,GetFilenameWithoutExtension(it))
			if os.path.isfile(bin):
				ret+=[bin]	
			
		# frameworks
		for it in glob.glob("bin/*.framework"):
			file="%s/Versions/Current/%s" % (it,GetFilenameWithoutExtension(it))  
			if os.path.isfile(os.path.realpath(file)):
				ret+=[file]			

		return ret
  
	# __extractDeps
	def __extractDeps(self,filename):
		output=GetCommandOutput(['otool', '-L' , filename])
		lines=output.split('\n')[1:]
		deps=[line.strip().split(' ', 1)[0].strip() for line in lines]
	
		# remove any reference to myself
		deps=[dep for dep in deps if os.path.basename(filename)!=os.path.basename(dep)]
		return deps
	
	# __findLocal
	def __findLocal(self,filename):
		key=os.path.basename(filename)
		return self.locals[key] if key in self.locals else None
				
	# __addLocal
	def __addLocal(self,filename):
		
		# already added 
		if self.__findLocal(filename): 
			return
		
		key=os.path.basename(filename)
		
		print("# Adding local",key,"=",filename)
		
		self.locals[key]=filename
		
		for dep in self.__extractDeps(filename):
			self.__addGlobal(dep)
				
		
	# __addGlobal
	def __addGlobal(self,dep):
		
		# it's already a local
		if self.__findLocal(dep):
			return
		
		# wrong file
		if not os.path.isfile(dep) or not dep.startswith("/"):
			return
		
		# ignoring the OS system libraries
		if dep.startswith("/System") or dep.startswith("/lib") or dep.startswith("/usr/lib"):
			return
			
		# I don't want to copy Python dependency
		if "Python.framework" in dep :
			print("Ignoring Python.framework:",dep)
			return			
			
		key=os.path.basename(dep)
		
		print("# Adding global",dep,"=",dep)
					
		# special case for frameworks (I need to copy the entire directory)
		if APPLE and ".framework" in dep:
			framework_dir=dep.split(".framework")[0]+".framework"
			CopyDirectory(framework_dir,"bin")
			filename="bin/" + os.path.basename(framework_dir) + dep.split(".framework")[1]
			self.__addLocal(filename) # now a global becomes a local one
			return
			
		if dep.endswith(".dylib") or dep.endswith(".so"):
			filename="bin/" + os.path.basename(dep)
			CopyFile(dep,filename)
			self.__addLocal(filename) # now a global becomes a local one
			return 
		
		raise Exception("Unknonw dependency %s file file" % (dep,))	
			

	# copyExternalDependenciesAndFixRPaths
	def copyExternalDependenciesAndFixRPaths(self):

		self.locals={}
		
		for filename in self.__findAllBinaries():
			self.__addLocal(filename)
			
		# note: __findAllBinaries need to be re-executed
		for filename in self.__findAllBinaries():
			
			deps=self.__extractDeps(filename)
			
			if False:
				print("")
				print("#/////////////////////////////")
				print("# Fixing",filename,"which has the following dependencies:")
				for dep in deps:
					print("#\t",dep)
				print("")
				
			def getRPathBaseName(filename):
				if ".framework" in filename:
					# example: /bla/bla/ QtOpenGL.framework/Versions/5/QtOpenGL -> QtOpenGL.framework/Versions/5/QtOpenGL
					return os.path.basename(filename.split(".framework")[0]+".framework") + filename.split(".framework")[1]   
				else:
					return os.path.basename(filename)
			
			ExecuteCommand(["chmod","u+rwx",filename])
			ExecuteCommand(['install_name_tool','-id','@rpath/' + getRPathBaseName(filename),filename])

			# example QtOpenGL.framework/Versions/5/QtOpenGL
			for dep in deps:
				local=self.__findLocal(dep)
				if local: 
					ExecuteCommand(['install_name_tool','-change',dep,"@rpath/"+ getRPathBaseName(local),filename])

			ExecuteCommand(['install_name_tool','-add_rpath','@loader_path',filename])
			
			# how to go from a executable in ./** to ./bin
			root=os.path.realpath("")
			curr=os.path.dirname(os.path.realpath(filename))
			N=len(curr.split("/"))-len(root.split("/"))	
			ExecuteCommand(['install_name_tool','-add_rpath','@loader_path' +  ("/.." * N) + "/bin",filename])


	# addRPath
	def addRPath(self,value):
		for filename in self.__findAllBinaries():
			ExecuteCommand(["install_name_tool","-add_rpath",value,filename])	
		
		
		
# ///////////////////////////////////////

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

QT_DEBUG_PLUGINS=1 LD_DEBUG=libs,files  ./visusviewer.sh

LD_DEBUG=libs,files ldd bin/visus

# this shows the rpath
readelf -d bin/visus

"""
	


def SetLinuxRPaths(external_rpaths=[]):

	binaries=[]
	
	# *.so
	binaries+=RecursiveFindFiles('.', '*.so')
	
	# executables in bin
	binaries+=[it for it in glob.glob("bin/*") if os.path.isfile(it) and not os.path.splitext(it)[1]]

	for filename in binaries:
		
		v=['$ORIGIN']

		# how many ../ I should do to go back to the root of the installation directory
		root=os.path.realpath(".")
		curr=os.path.dirname(os.path.realpath(filename))
		N=len(curr.split("/"))-len(root.split("/"))	
		v+=['$ORIGIN' +  ("/.." * N) + "/bin"]	
		
		if external_rpaths:
			v+=external_rpaths
			
		ExecuteCommand(["patchelf", "--set-rpath", ":".join(v) , filename])





# ////////////////////////////////////////////////////////////////////////////////////////////////
def Main(argv):
	
	this_dir=ThisDir(__file__)
	os.chdir(this_dir)

	action=argv[1]

	# _____________________________________________
	if action=="dirname":
		print(this_dir)
		return 0
	
	# _____________________________________________
	if action=="post-install":	
		print("Executing",action,"cwd",os.getcwd(),"args",argv)
		if WIN32:
			raise Exception("not supported")
		elif APPLE:
			AppleDeploy().copyExternalDependenciesAndFixRPaths()
		else:
			SetLinuxRPaths()		

		print("done",action)
		return 0

	# _____________________________________________
	if action=="use-pyqt":

		print("Executing",action,"cwd",os.getcwd(),"args",argv)

		QT_VERSION = ReadTextFile("QT_VERSION")

		packagename="PyQt5=="+"{}.{}".format(QT_VERSION.split(".")[0],QT_VERSION.split(".")[1])
		if not PipInstall(packagename,["--ignore-installed"]):
			raise Exception("Cannot install PyQt5")
		print("Installed",packagename)

		PipInstall("PyQt5-sip",["--ignore-installed"])
		
		# avoid conflicts removing any Qt file
		RemoveFiles("bin/Qt*")
		RemoveFiles("bin/libQt*")

		Qt5_DIR=GetCommandOutput([sys.executable,"-c","import os,PyQt5;print(os.path.join(os.path.dirname(PyQt5.__file__),'Qt'))"]).strip()
		print("Qt5_DIR",Qt5_DIR)
		if not os.path.isdir(Qt5_DIR):
			print("Error directory does not exists")
			raise Exception("internal error")
			
		# for windowss see VisusGui.i (%pythonbegin section, I'm using sys.path)
		if WIN32:
			pass
		elif APPLE:	
			AppleDeploy().addRPath(os.path.join(Qt5_DIR,"lib"))
		else:
			SetLinuxRPaths([os.path.join(Qt5_DIR,"lib")])

		print("done",action)
		return 0

	raise Exception("Unknown argument " + action)

# ////////////////////////////////////////////////////////////////////////////////////////////////
if __name__ == '__main__':

	try:		
		retcode=Main(sys.argv)
		sys.exit(retcode)
		
	except Exception as e:
		print(e)
		traceback.print_exc(file=sys.stdout)
		sys.exit(-1)



