
import os
import sys
import subprocess
import glob
import shutil
import platform
import errno

qt_plugins=("iconengines","imageformats","platforms","printsupport","styles")

bVerbose=False

# ///////////////////////////////////////
class BaseDeployStep:
	
	# constructor
	def __init__(self):
		self.qt_directory=None
		
	# executeCommand
	def executeCommand(self,cmd):	
		if bVerbose: print(" ".join(cmd))
		subprocess.call(cmd)	
		
		
	# copyFile
	def copyFile(self,src,dst):
		
		src=os.path.realpath(src) 
		dst=os.path.realpath(dst)		
		
		if src==dst or not os.path.isfile(src):
			return		

		self.createDirectory(os.path.dirname(dst))
		self.executeCommand(["cp","-rf", src,dst])	
		
		
	#  copyDirectory
	def copyDirectory(self,src,dst):
		
		src=os.path.realpath(src)
		
		if not os.path.isdir(src):
			return
		
		self.createDirectory(dst)
		
		# problems with symbolic links so using shutil
		# self.executeCommand(["cp","-rf",src,dst])
		import shutil
		
		dst=dst+"/" + os.path.basename(src)
		
		if os.path.isdir(dst):
			shutil.rmtree(dst,ignore_errors=True)
			
		shutil.copytree(src, dst, symlinks=True)				
		
	# writeTextFile
	def writeTextFile(self,filename,content):
		
		if not isinstance(content, str):
			content="\n".join(content)+"\n"
			
		self.createDirectory(os.path.dirname(os.path.realpath(filename)))
		file = open(filename,"wt") 
		file.write(content) 
		file.close() 		
		
	# createDirectory
	def createDirectory(self,value):
		try: 
			os.makedirs(value)
		except OSError:
			if not os.path.isdir(value):
				raise
		
	# getFileNameWithoutExtension
	def getFileNameWithoutExtension(self,filename):
		return os.path.splitext(os.path.basename(filename))[0]


# ///////////////////////////////////////
class Win32DeployStep(BaseDeployStep):
	
	# constructor
	def __init__(self):
		BaseDeployStep.__init__(self)

	# run
	def run(self):
		
		# if VISUS_INTERNAL_PYTHON I want to be able to install new pip packages...
		VISUS_INTERNAL_PYTHON=os.path.isdir("Python36")
		
		if VISUS_INTERNAL_PYTHON:
		
			if os.path.isfile("Python36/python36.zip"):
				import zipfile
				zipper = zipfile.ZipFile("Python36/python36.zip", 'r')
				zipper.extractall("Python36/lib")
				zipper.close()	
				os.remove("Python36/python36.zip")		

			# install python pip
			if not os.path.isfile("Python36\\Scripts\\pip.exe"):
				self.executeCommand(["Python36\\python.exe","python36\\get-pip.py"])
			
		self.writeTextFile("visus.bat",[
			r'cd /d %~dp0',
			r'set this_dir=%cd%',
			r'set PYTHONHOME=%this_dir%\Python36',
			r'set PATH=%PYTHONHOME%;%PYTHONHOME%\lib\site-packages\PyQt5\Qt\bin;%this_dir%\bin;%PATH%', 
			r'set QT_PLUGIN_PATH=%PYTHONHOME%\lib\site-packages\PyQt5\Qt\plugins',
			r'python.exe -m pip install -U pip setuptools',
			r'python.exe -m pip install numpy PyQt5',
			r'bin\visus.exe %*'
		])
		
		self.writeTextFile("visusviewer.bat",[
			r'cd /d %~dp0',
			r'set this_dir=%cd%',
			r'set PYTHONHOME=%this_dir%\Python36',
			r'set PATH=%PYTHONHOME%;%PYTHONHOME%\lib\site-packages\PyQt5\Qt\bin;%this_dir%\bin;%PATH%', 
			r'set QT_PLUGIN_PATH=%PYTHONHOME%\lib\site-packages\PyQt5\Qt\plugins',
			r'python.exe -m pip install -U pip setuptools',
			r'python.exe -m pip install numpy PyQt5',
			r'bin\visusviewer.exe %*'
		])	
		
		# deploy using qt
		deployqt=os.path.realpath(self.qt_directory+"/bin/windeployqt")
		self.executeCommand([deployqt,"bin\\visusviewer.exe", "--libdir","bin","--plugindir","bin\\plugins","--no-translations"])



# ///////////////////////////////////////
class AppleDeployStep(BaseDeployStep):
	
	
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
		BaseDeployStep.__init__(self)
		
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
		
	
	#findApps
	def findApps(self):
		ret=[]
		for it in glob.glob("bin/*.app"):
			bin="%s/Contents/MacOS/%s" % (it,self.getFileNameWithoutExtension(it))
			if os.path.isfile(bin):
				ret+=[bin]
		return ret
		
	# findFrameworks
	def findFrameworks(self):
		ret=[]
		for it in glob.glob("bin/*.framework"):
			file="%s/%s" % (it,self.getFileNameWithoutExtension(it))
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
				
	# printAllDeps
	def printAllDeps(self,bFull=False):
		
		deps={}
		for filename in self.findAllBinaries():
			for dep in self.extractDeps(filename):
				if not dep in deps: deps[dep]=[]
				deps[dep]+=[filename]	
		
		print("This are the dependency")
		for dep in deps:
			
			if bFull:
				print(dep)
				for it in deps[dep]:
					print("\t",it)		
				print()
			elif not dep.startswith("@"):
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
				self.executeCommand(["chmod","u+w",filename])
				self.executeCommand(cmd)	
			
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
		
		# ignore system libraries
		if dep.startswith("/System") or dep.startswith("/usr/lib"):
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
			self.copyDirectory(framework_dir,"bin")
			local="bin/" + os.path.basename(framework_dir) + dep.split(".framework")[1]
			
		elif dep.endswith(".dylib"):
			local="bin/" + os.path.basename(dep)
			self.copyFile(dep,local)
			
		else:
			raise Exception("Unknonw %s file file" % (dep,))	
			
		self.addLocal(local)
		
	# create qt.conf (even if not necessary)
	def createQtConfs(self):
		
		for filename in self.findApps():
			qt_conf=os.path.realpath(filename+"/../../Contents/Resources/qt.conf")
			self.writeTextFile(qt_conf,[
				"[Paths]",
				"  Plugins=%s/bin/plugins" % (self.relativeRootDir(filename,"."),)
			])		
			
	# createCommands
	def createCommands(self):
					
		for name in ("visusviewer","visus"):
		
			command_filename="%s.command" % (name,)
			
			self.writeTextFile(command_filename,[
				'#!/bin/bash',
				'this_dir=$(cd "$(dirname "${BASH_SOURCE[0]}" )" && pwd)',
				'cd ${this_dir}',
				'export PATH=${this_dir}/bin:$PATH',
				'export QT_PLUGIN_PATH=${this_dir}/bin/plugins',
				'export PYTHONPATH=${this_dir}:${this_dir}/bin:$PYTHONPATH',
				'./bin/%s.app/Contents/MacOS/%s' %(name,name)])
			
			self.executeCommand(["chmod","a+rx",command_filename])
					
	# run
	def run(self):
		
		# copy qt plugins inplace
		for plugin in qt_plugins :
			src=os.path.realpath(self.qt_directory+"/plugins/"+plugin)	
			self.copyDirectory(src,"bin/plugins")
	
		self.locals={}
		for local in self.findAllBinaries():
			self.addLocal(local)
			
		self.changeAllDeps()
		self.createCommands()
		self.createQtConfs()
		if bVerbose:
			self.printAllDeps()
			


# ///////////////////////////////////////
class LinuxDeployStep(BaseDeployStep):

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
	
	
	"""
	
	# constructor
	def __init__(self):
		BaseDeployStep.__init__(self)

	# copyQtPlugins
	def copyQtPlugins(self):
		for plugin in qt_plugins :
			self.copyDirectory(self.qt_directory+"/qt5/plugins/"+plugin,"bin/plugins")			

	# findAllDeps
	def findAllDeps(self):
		output=subprocess.check_output(('ldd',"bin/visusviewer"))
		if sys.version_info >= (3, 0): output=output.decode("utf-8")
		output=output.strip()	
		deps={}
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
			if filename.startswith("/"):
				self.copyFile(filename,"bin/"+key)	

	# copySharedObjectsSymbolicLinks
	def copySharedObjectsSymbolicLinks(self):
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

		
	# findApps
	def findApps(self):	
		return [it for it in glob.glob("bin/*") if os.path.isfile(it) and not os.path.splitext(it)[1]]
		
	# setOrigins
	def setOrigins(self):
		for filename in glob.glob("bin/*.so")  + self.findApps():
			self.executeCommand(["patchelf", "--set-rpath", "$ORIGIN", filename])	
			
	# createBashScripts
	def createBashScripts(self):
		for app in self.findApps():
			name=self.getFileNameWithoutExtension(app)
			bash_filename="%s.sh" % (name,)
			self.writeTextFile(bash_filename,[
				'#!/bin/bash',
				'this_dir=$(cd "$(dirname "${BASH_SOURCE[0]}" )" && pwd)',
				'cd ${this_dir}',
				'export PATH=${this_dir}/bin:$PATH',
				'export QT_PLUGIN_PATH=${this_dir}/bin/plugins',
				'export PYTHONPATH=${this_dir}:${this_dir}/bin:$PYTHONPATH',
				'./bin/%s' %(name)])
			self.executeCommand(["chmod","a+rx",bash_filename])	

	# printDeps
	def printDeps(self):
		print("Finding deps:")
		deps=self.findAllDeps()
		for key in deps:
			print("%30s" % (key,),deps[key])

	# run
	def run(self):
	
		self.copyQtPlugins()
		
		for I in range(2):
			self.copyGlobalDeps()
			self.copySharedObjectsSymbolicLinks()
			self.setOrigins()
			
		self.createBashScripts()
		
		if bVerbose:
			self.printDeps()
		
		

# //////////////////////////////////////////////////////////////////////////////
if __name__ == "__main__":
	
	print("Starting deploy...")
	
	qt_directory=""
	I=1
	while I < len(sys.argv):
		
		if sys.argv[I]=="--qt-directory":
			qt_directory=sys.argv[I+1]	
			# sys.exit(0)
			I+=2
			continue
			
		print("Unknonwn argument",sys.argv[I])
		sys.exit(-1)	
	
	print("  platform.system()",platform.system())	
	print("  qt_directory",qt_directory)	

	if platform.system()=="Windows" or platform.system()=="win32":
		deploy=Win32DeployStep()
		deploy.qt_directory=os.path.realpath(qt_directory+"/../../..")
		deploy.run()
		
	elif platform.system()=="Darwin":
		deploy=AppleDeployStep()
		deploy.qt_directory=os.path.realpath(qt_directory+"/../../..")
		deploy.run()
	
	else:
		deploy=LinuxDeployStep()
		deploy.qt_directory=os.path.realpath(qt_directory+"/../..")
		deploy.run()
	
	print("done deploy")
	sys.exit(0)
