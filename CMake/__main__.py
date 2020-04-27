import sys
import traceback

from PyUtils import *

	
"""
Linux:
	QT_DEBUG_PLUGINS=1 LD_DEBUG=libs,files  ./visusviewer.sh
	LD_DEBUG=libs,files ldd bin/visus
	readelf -d bin/visus

OSX:
	otool -L libname.dylib
	otool -l libVisusGui.dylib  | grep -i "rpath"
	DYLD_PRINT_LIBRARIES=1 QT_DEBUG_PLUGINS=1 visusviewer.app/Contents/MacOS/visusviewer
"""


# ///////////////////////////////////////
class MyDeploy:

	
	# constructor
	def __init__(self):
		pass

	# showDeps
	def showDeps(self):
		
		deps={}
		for filename in self.findAllBinaries():
			for dep in self.extractDeps(filename):
				if not dep in deps:
					deps[dep]=True
					
		v=[it for it in deps.keys()]
		v.sort()
		for it in v:
			print(it)
		

	# findAllBinaries
	def findAllBinaries(self):	
		ret=[]
		
		ret+=RecursiveFindFiles(".","*.so")
		ret+=RecursiveFindFiles(".","*.dylib")
		
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
  
	# extractDeps
	def extractDeps(self,filename):
		output=GetCommandOutput(['otool', '-L' , filename])
		lines=output.split('\n')[1:]
		deps=[line.strip().split(' ', 1)[0].strip() for line in lines]
	
		# remove any reference to myself
		deps=[dep for dep in deps if os.path.basename(filename)!=os.path.basename(dep)]
		return deps
	
	# findLocal
	def findLocal(self,filename):
		key=os.path.basename(filename)
		return self.locals[key] if key in self.locals else None
				
	# addLocal
	def addLocal(self,filename):
		
		# already added 
		if self.findLocal(filename): 
			return
		
		key=os.path.basename(filename)
		
		print("# Adding local",key,"=",filename)
		
		self.locals[key]=filename
		
		for dep in self.extractDeps(filename):
			self.addGlobal(dep)
				
		
	# addGlobal
	def addGlobal(self,dep):
		
		# it's already a local
		if self.findLocal(dep):
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
			self.addLocal(filename) # now a global becomes a local one
			return
			
		if dep.endswith(".dylib") or dep.endswith(".so"):
			filename="bin/" + os.path.basename(dep)
			CopyFile(dep,filename)
			self.addLocal(filename) # now a global becomes a local one
			return 
		
		raise Exception("Unknonw dependency %s file file" % (dep,))	
			

	# run
	def run(self, Qt5_HOME):
		
			# copy plugins
		CopyDirectory(Qt5_HOME + "/plugins/iconengines"   ,"./bin/qt/plugins")
		CopyDirectory(Qt5_HOME + "/plugins/platforms"     ,"./bin/qt/plugins")
		CopyDirectory(Qt5_HOME + "/plugins/rintsupport"   ,"./bin/qt/plugins")
		CopyDirectory(Qt5_HOME + "/plugins/styles"        ,"./bin/qt/plugins")
			

		self.locals={}
		
		for filename in self.findAllBinaries():
			self.addLocal(filename)
			
		# note: findAllBinaries need to be re-executed
		for filename in self.findAllBinaries():
			
			deps=self.extractDeps(filename)
			
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
				local=self.findLocal(dep)
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
		for filename in self.findAllBinaries():
			ExecuteCommand(["install_name_tool","-add_rpath",value,filename])	
		


# ////////////////////////////////////////////////////////////////////////////////////////////////
if __name__ == '__main__':

	action=sys.argv[1]

	# _____________________________________________
	if action=="dirname":
		this_dir=ThisDir(__file__)
		os.chdir(this_dir)		
		print(this_dir)
		sys.exit(0)
	
	# _____________________________________________
	if action=="show-deps":		
		MyDeploy().showDeps()	
		sys.exit(0)
		
	# _____________________________________________
	if action=="post-install":	
		print("Executing",action,"cwd",os.getcwd(),"args",sys.argv)
		if not APPLE: raise Exception("not supported")
		d=MyDeploy()
		d.run(sys.argv[2])	
		d.showDeps()
		print("done",action)
		sys.exit(0)

	# _____________________________________________
	if action=="use-pyqt":
		this_dir=ThisDir(__file__)
		os.chdir(this_dir)
		print("Executing",action,"cwd",os.getcwd(),"args",sys.argv)

		QT_VERSION = ReadTextFile("QT_VERSION")
		
		CURRENT_QT_VERSION=""
		try:
			from PyQt5 import Qt
			CURRENT_QT_VERSION=str(Qt.qVersion())
		except:
			pass
			
		if QT_VERSION.split('.')[0:2] == CURRENT_QT_VERSION.split('.')[0:2]:
			print("Using current Pyqt5",CURRENT_QT_VERSION)
		else:
			print("Installing a new PyQt5")

			def InstallPyQt5():
				QT_MAJOR_VERSION,QT_MINOR_VERSION=QT_VERSION.split(".")[0:2]
				versions=[]
				versions+=["{}".format(QT_VERSION)]
				versions+=["{}.{}".format(QT_MAJOR_VERSION,QT_MINOR_VERSION)]
				versions+=["{}.{}.{}".format(QT_MAJOR_VERSION,QT_MINOR_VERSION,N) for N in reversed(range(1,10))]
				for version in versions:
					packagename="PyQt5=="+version
					if PipInstall(packagename,["--ignore-installed"]):
						PipInstall("PyQt5-sip",["--ignore-installed"])
						print("Installed",packagename)
						return True 
				raise Exception("Cannot install PyQt5")

			InstallPyQt5()

		Qt5_DIR=GetCommandOutput([sys.executable,"-c","import os,PyQt5;print(os.path.join(os.path.dirname(PyQt5.__file__),'Qt'))"]).strip()
		print("Qt5_DIR",Qt5_DIR)
		if not os.path.isdir(Qt5_DIR):
			print("Error directory does not exists")
			raise Exception("internal error")
			
		# avoid conflicts removing any Qt file
		RemoveDirectory("bin/qt")		
			
		# for windowss see VisusGui.i (%pythonbegin section, I'm using sys.path)
		if WIN32:
			pass
		elif APPLE:	
			RemoveFiles("bin/Qt*")	
			AppleDeploy().addRPath(os.path.join(Qt5_DIR,"lib"))
		else:
			script=ThisDir(__file__) + "/scripts/set_rpath.sh"
			arg=":".join(["$ORIGIN","$ORIGIN/bin",Qt5_DIR+"/lib"])
			subprocess.check_call(["bash",script,arg])
			

		print("done",action)
		sys.exit(0)

	print("EXEPTION Unknown argument " + action)
	sys.exit(-1)



