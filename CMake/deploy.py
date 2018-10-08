
import os
import sys
import subprocess
import glob
import shutil
import platform
import errno

WIN32=platform.system()=="Windows" or platform.system()=="win32"
APPLE=platform.system()=="Darwin"

bVerbose=False

	
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
			file="%s/%s" % (it,GetFileNameWithoutExtension(it))
			if os.path.isfile(os.path.realpath(file)):
				ret+=[file]
		return ret
		
	# findAllBinaries
	def findAllBinaries(self):	
		ret=[]
		ret+=glob.glob("bin/*.dylib")
		ret+=glob.glob("bin/*.so")
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
		
		deps={}
		for filename in self.findAllBinaries():
			for dep in self.extractDeps(filename):
				if not dep in deps: deps[dep]=[]
				deps[dep]+=[filename]	
		
		for dep in deps:
			if not dep.startswith("@"):
				print(dep)
						
	# relativeRootDir
	def relativeRootDir(self,local,prefix="@loader_path"):
		A=os.path.realpath(".").split("/")
		B=os.path.dirname(os.path.realpath(local)).split("/")
		N=len(B)-len(A)	
		return prefix + "/.." * N		
	
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
			
			
			cmd=["install_name_tool"]	
			for dep in deps:
				local=self.getLocal(dep)
				if not local: continue
				Old=dep
				New=self.relativeRootDir(filename)+ "/" + local
				cmd+=["-change",Old,New]
			
			cmd+=[filename]
			
			if "-change" in cmd:
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
	
		LD_DEBUG=libs ldd install/bin/visusviewer

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
		deps={}
		for filename in self.findAllBinaries():
			output=subprocess.check_output(('ldd',filename))
			if sys.version_info >= (3, 0): 
				output=output.decode("utf-8")
			output=output.strip()	
			for line in output.splitlines():
				items=[it.strip() for it in line.split(" ") if len(it.strip())]
				if len(items)>=4 and items[1]=='=>' and items[2]!='not' and items[3]!='found':
					deps[items[0]]=items[2]	
		return deps

	# copyGlobalDeps
	def copyGlobalDeps(self):
		deps=self.findAllDeps()
		for key in deps:
			filename=deps[key]
			# only absolute file
			if filename.startswith("/"):
				CopyFile(filename,"bin/"+key)	

	# fixSymbolicLinks
	def fixSymbolicLinks(self):
		pushd=os.getcwd()
		os.chdir("bin")	
		for filename in glob.glob("*.so.*"):
			if bVerbose:
				print("Fixing symbolic link of",filename)
			link,ext=os.path.splitext(filename)
			while not ext==".so":
				if(os.path.islink(link)):
					os.remove(link)
				os.symlink(filename, link)
				link,ext=os.path.splitext(link)		
		os.chdir(pushd)			

	# setOrigins
	def setOrigins(self):
		for filename in self.findAllBinaries():
			ExecuteCommand(["patchelf", "--set-rpath", "$ORIGIN", filename])	

	# showDeps
	def showDeps(self):
		deps=self.findAllDeps()
		for key in deps:
			if deps[key].startswith(os.getcwd()):
				continue
			print("%30s" % (key,),deps[key])

	# fixAllDeps
	def fixAllDeps(self):
		# need to run two times
		for I in range(2):
			self.copyGlobalDeps()
			self.fixSymbolicLinks()
			self.setOrigins()
		

		
		

# //////////////////////////////////////////////////////////////////////////////
if __name__ == "__main__":
	
	deploy=None
	
	if WIN32:
		print("Not supported")
		sys.exit(-1)
		
	deploy=	AppleDeployStep() if APPLE else LinuxDeployStep()

	I=1
	while I < len(sys.argv):
	
		# verbose	
		if sys.argv[I]=="--verbose":
			bVerbose=True
			I+=1
			continue
		
		if sys.argv[I]=="--show-deps":
			I+=1
			print("Current deps:")
			
			if (not APPLE):
				print("NOTE for linux, you can still see some global deps due to the ldd command")
				
			deploy.showDeps()
			continue
		
		# fix all
		if sys.argv[I]=="--fix-deps":
			I+=1
			print("Fixing deps")
			deploy.fixAllDeps()
			continue		
			
		print("Unknonwn argument",sys.argv[I])
		sys.exit(-1)		
	
	sys.exit(0)
