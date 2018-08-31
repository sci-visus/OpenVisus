
import os
import sys
import subprocess
import glob
import shutil

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

# ///////////////////////////////////////
class DeployStep:
	
	# constructor
	def __init__(self,Qt5_DIR):
		self.Qt5_DIR=Qt5_DIR
		
	# executeCommand
	def executeCommand(self,cmd):	
		#print(" ".join(cmd))
		subprocess.call(cmd)	
		
	# writeTextFile
	def writeTextFile(self,filename,content):
		self.createDirectory(os.path.dirname(filename))
		file = open(filename,"wt") 
		file.write(content) 
		file.close() 		
		
	# createDirectory
	def createDirectory(self,value):
		return self.executeCommand(["mkdir","-p",value]);
		
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
		
	# extractDeps
	def extractDeps(self,filename):
		output=subprocess.check_output(('otool', '-L', filename))
		if sys.version_info >= (3, 0): output=output.decode("utf-8")
		output=output.strip()
		lines=output.split('\n')[2:]
		ret=[line.strip().split(' ', 1)[0].strip() for line in lines]	
		return ret
		
	# findFilesWithExt
	def findFilesWithExt(self,ext):
		ret=[]
		for it in glob.glob("**/*"+ext,recursive=True):
			if os.path.isfile(it):
				ret+=[it]
		return ret
		
	# findDirectoriesWithExt
	def findDirectoriesWithExt(self,ext):
		ret=[]
		for it in glob.glob("**/*"+ext,recursive=True):
			if os.path.isdir(it):
				ret+=[it]
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
			content="\n".join(["[Paths]","  Plugins=%s/Dependencies/plugins" % (self.relativeRootDir(filename,"."),)])
			self.writeTextFile(qt_conf,content)					
					
	# run
	def run(self):
		
		# copy qt plugins inplace
		for plugin in ("iconengines","imageformats","platforms","printsupport","styles"):
			src=os.path.realpath(self.Qt5_DIR+"/plugins/"+plugin)	
			self.copyDirectory(src,"Dependencies/plugins")
	
		self.locals={}
		for local in self.findAllBinaries():
			self.addLocal(local)
			
		self.changeAllDeps()
		self.createQtConfs()
		#self.printAllDeps()
			

if __name__ == "__main__":
	Qt5_DIR=os.path.realpath(sys.argv[1])
	print("Executing deploy","Qt5_DIR",Qt5_DIR)
	DeployStep(Qt5_DIR).run()