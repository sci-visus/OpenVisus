
import os
import sys
import subprocess
import glob
import shutil
import platform
import errno

qt_plugins=("iconengines","imageformats","platforms","printsupport","styles")

# ///////////////////////////////////////
class BaseDeployStep:
	
	# constructor
	def __init__(self):
		self.qt_directory=None
		
	# executeCommand
	def executeCommand(self,cmd):	
		#print(" ".join(cmd))
		subprocess.call(cmd)	
		
	# writeTextFile
	def writeTextFile(self,filename,lines):
		content="\n".join(lines)+"\n"
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
		
	#	copyDirectory
	def copyDirectory(self,src,dst):
		self.createDirectory(dst)
		self.executeCommand(["cp","-R",src,dst])
		
	# copyFile
	def copyFile(self,src,dst):
		self.createDirectory(os.path.dirname(dst))
		self.executeCommand(["cp",src,dst])	
		
	# getFileNameWithoutExtension
	def getFileNameWithoutExtension(self,filename):
		return os.path.splitext(os.path.basename(filename))[0]
				
	# findFilesWithExt
	def findFilesWithExt(self,ext,dir=os.getcwd()):
		pushd=os.getcwd()
		os.chdir(dir)
		ret=[]
		for it in glob.glob("**/*"+ext,recursive=True):
			if os.path.isfile(it):
				ret+=[it]		
		os.chdir(pushd)
		return ret
		
	# findDirectoriesWithExt
	def findDirectoriesWithExt(self,ext):
		ret=[]
		for it in glob.glob("**/*"+ext,recursive=True):
			if os.path.isdir(it):
				ret+=[it]
		return ret	

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
				
			self.writeTextFile("Python36/python36._pth",[
				".",
				".\\Lib",
				".\\Lib\\site-packages",
				"..\\",
				"import site"
			])
				
			# install python dependencies
			self.executeCommand(["Python36\\python.exe","python36\\get-pip.py"])
			self.executeCommand(["Python36\\Scripts\\pip.exe","install","numpy","pymap3d","PyQt5"])

		for exe in ("visus.exe","visusviewer.exe"):
			
			name=self.getFileNameWithoutExtension(exe)	
			
			PATH=[]
			
			QT_PLUGIN_PATH="%cd%\\.\\Dependencies\\plugins"
			
			# important that pyqt comes first! otherwise PyQt5 and Qt5 will fight
			if VISUS_INTERNAL_PYTHON:
				PATH+=[
					"%cd%\\Python36",
					"%cd%\\Python36\\Scripts",
					"%cd%\\Python36\\lib\\site-packages\\PyQt5\\Qt\\bin"
				]
				QT_PLUGIN_PATH="%cd%\\Python36\\lib\\site-packages\\PyQt5\\Qt\\plugins"
				
			PATH+=["%cd%\\Dependencies\dlls"]
			PATH+=["%PATH%"]

			# create a batch file
			self.writeTextFile("%s.bat" % (name,),[
				"cd /d %~dp0",
				"set PATH=" + (";".join(PATH)),
				"set QT_PLUGIN_PATH=" + QT_PLUGIN_PATH,
				exe + " %*"
			])
		
			# deploy using qt
			deployqt=os.path.realpath(self.qt_directory+"/bin/windeployqt")
			self.executeCommand([deployqt,exe, "--libdir","Dependencies/dlls","--plugindir","Dependencies/plugins","--no-translations"])



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
		lines=output.split('\n')[2:]
		ret=[line.strip().split(' ', 1)[0].strip() for line in lines]	
		return ret
		
	
	#findAppBinaries
	def findAppBinaries(self):
		ret=[]
		for it in self.findDirectoriesWithExt(".app"):
			bin="%s/Contents/MacOS/%s" % (it,self.getFileNameWithoutExtension(it))
			if os.path.isfile(bin):
				ret+=[bin]
		return ret
		
	# findFrameworkBinaries
	def findFrameworkBinaries(self):
		ret=[]
		for it in self.findDirectoriesWithExt(".framework"):
			file=os.path.realpath("%s/%s" % (it,self.getFileNameWithoutExtension(it)))
			if os.path.isfile(file):
				ret+=[file]
		return ret
		
	# findAllBinaries
	def findAllBinaries(self):	
		ret=[]
		ret+=self.findFilesWithExt(".dylib")
		ret+=self.findFilesWithExt(".so")
		ret+=self.findAppBinaries()
		ret+=self.findFrameworkBinaries()
		return ret
				
	# printAllDeps
	def printAllDeps(self):
		
		deps={}
		for filename in self.findAllBinaries():
			for dep in self.extractDeps(filename):
				if not dep in deps: deps[dep]=[]
				deps[dep]+=[filename]	
		
		print("This are the dependency")
		for dep in deps:
			print(dep)
			for it in deps[dep]:
				print("\t",it)		
			print()
						
	# relativeRootDir
	def relativeRootDir(self,local,prefix="@loader_path"):
		A=os.path.realpath(".").split("/")
		B=os.path.dirname(os.path.realpath(local)).split("/")
		N=len(B)-len(A)	
		return prefix + "/.." * N		
	
	# changeAllDeps
	def changeAllDeps(self):
		
		for filename in self.findAllBinaries():
			cmd=["install_name_tool"]	
			for dep in self.extractDeps(filename):
				local=self.getLocal(dep)
				if not local: continue
				Old=dep
				New=self.relativeRootDir(filename)+ "/" + local
				cmd+=["-change",Old,New]
			cmd+=[filename]
			
			if "-change" in cmd:
				self.executeCommand(["chmod","u+w",filename])
				print(" ".join(cmd),"\n")
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
			
		print("Copying",dep,"...")
					
		key=os.path.basename(dep)
		
		# special case for frameworks (I need to copy the entire directory)
		if ".framework" in dep:
			framework_dir=dep.split(".framework")[0]+".framework"
			self.copyDirectory(framework_dir,"Dependencies/Frameworks")
			local="Dependencies/Frameworks/" + os.path.basename(framework_dir) + dep.split(".framework")[1]
			
		elif dep.endswith(".dylib"):
			local="Dependencies/dylibs/" + os.path.basename(dep)
			self.copyFile(dep,local)
			
		else:
			raise Exception("Unknonw %s file file" % (dep,))	
			
		self.addLocal(local)
		
	# create qt.conf (even if not necessary)
	def createQtConfs(self):
		
		for filename in self.findAppBinaries():
			qt_conf=os.path.realpath(filename+"/../../Contents/Resources/qt.conf")
			content="\n".join()
			self.writeTextFile(qt_conf,[
				"[Paths]",
				"  Plugins=%s/Dependencies/plugins" % (self.relativeRootDir(filename,"."),)
			])					
					
	# run
	def run(self):
		
		# copy qt plugins inplace
		for plugin in qt_plugins :
			src=os.path.realpath(self.qt_directory+"/plugins/"+plugin)	
			self.copyDirectory(src,"Dependencies/plugins")
	
		self.locals={}
		for local in self.findAllBinaries():
			self.addLocal(local)
			
		self.changeAllDeps()
		self.createQtConfs()
		#self.printAllDeps()
			


# ///////////////////////////////////////
class LinuxDeployStep(BaseDeployStep):
	
	# constructor
	def __init__(self):
		BaseDeployStep.__init__(self)
	
	# extractDeps
	def extractDeps(self,filename):
		output=subprocess.check_output(('readelf','-d',filename))
		if sys.version_info >= (3, 0): output=output.decode("utf-8")
		return output.strip()
		
	# debugLibDeps
	def debugLibDeps(self,filename):
		output=subprocess.check_output(('LD_DEBUG=libs','ldd',filename))
		if sys.version_info >= (3, 0): output=output.decode("utf-8")
		return output.strip()
		

	# installSharedLib
	def installSharedLib(self,filename):
		filename=os.path.realpath(path) 
		if (os.path.isfile(filename)):
			self.executeCommand("cp",filename,".")
								
	# run
	def run(self):
		
		for name in ("QCore","Widgets","Gui","OpenGL"):
			raise Exception("TPDP")
			#filename=self.qt_directory+...+name
			#self.installSharedLib(filename)
				
		#if (NOT VISUS_INTERNAL_OPENSSL)
		#	self.installSharedLib(${OPENSSL_SSL_LIBRARY})
		#	self.installSharedLib(${OPENSSL_CRYPTO_LIBRARY})
		#endif()
		
		self.installSharedLib("/usr/lib64/libGLU.so")
		
		for filename in glob.glob("libVisus*.so") + glob.glob("_Visus*.so") + ["libmod_visus.so","visus","visusviewer"]		:
			self.executeCommand(["patchelf", "--set-rpath", "$ORIGIN", filename])
			
		# problem with symbolic link 
		for filename in glob.glob("*.so.*"):
			print("Fixing symbolic link of",filename)
			link,ext=os.path.splitext(filename)
			while not ext==".so":
				if(os.path.islink(link)):os.remove(link)
				os.symlink(filename, link)
				link,ext=os.path.splitext(link)			
		

# //////////////////////////////////////////////////////////////////////////////
if __name__ == "__main__":
	qt_directory=os.path.realpath(sys.argv[1])
	
	print("platform.system()",platform.system())
	
	deploy=None
	if platform.system()=="Windows" or platform.system()=="win32":
		deploy=Win32DeployStep()
		
	elif platform.system()=="Darwin":
		deploy=AppleDeployStep()
	
	else:
		deploy=LinuxDeployStep()
		
	I=1
	while I < len(sys.argv):
		
		if sys.argv[I]=="--qt-directory":
			deploy.qt_directory=sys.argv[I+1]
			print("qt_directory",deploy.qt_directory)		
			I+=2
			continue
			
		print("Unknonwn argument",sys.argv[I])
		sys.exit(-1)
			
	deploy.run()
	sys.exit(0)