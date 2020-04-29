import os, sys, glob, subprocess, platform, shutil

WIN32=platform.system()=="Windows" or platform.system()=="win32"
APPLE=platform.system()=="Darwin"

# /////////////////////////////////////////////////////////////////////////
def GetFilenameWithoutExtension(filename):
	return os.path.splitext(os.path.basename(filename))[0]

# /////////////////////////////////////////////////////////////////////////
def GetCommandOutput(cmd):
	output=subprocess.check_output(cmd)
	if sys.version_info >= (3, 0): output=output.decode("utf-8")
	return output.strip()
	
# /////////////////////////////////////////////////////////////////////////
def ExecuteCommand(cmd):	
	# print("Executing command", cmd)
	return subprocess.call(cmd, shell=False)
	

	
	# /////////////////////////////////////////////////////////////////////////
def CreateDirectory(value):
	try: 
		os.makedirs(value)
	except OSError:
		if not os.path.isdir(value):
			raise

	
# /////////////////////////////////////////////////////////////////////////
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
	
# /////////////////////////////////////////////////////////////////////////
def FindAllBinaries():
	files=[]
	files+=glob.glob("**/*.so", recursive=True)
	files+=glob.glob("**/*.dylib", recursive=True)
	files+=["%s/Versions/Current/%s" % (it,GetFilenameWithoutExtension(it)) for it in glob.glob("**/*.framework", recursive=True)]
	files+=["%s/Contents/MacOS/%s"   % (it,GetFilenameWithoutExtension(it)) for it in glob.glob("**/*.app",       recursive=True)]		
	return files	
	
# /////////////////////////////////////////////////////////////////////////
def ExtractDeps(filename):
	output=GetCommandOutput(['otool', '-L' , filename])
	deps=[line.strip().split(' ', 1)[0].strip() for line in output.split('\n')[1:]]
	deps=[dep for dep in deps if os.path.basename(filename)!=os.path.basename(dep)] # remove any reference to myself
	return deps
	
# /////////////////////////////////////////////////////////////////////////
def ShowDeps():
	all_deps={}
	for filename in FindAllBinaries():
		#print(filename)
		for dep in ExtractDeps(filename):
			#print("\t",dep)
			all_deps[dep]=1
	all_deps=list(all_deps)
	all_deps.sort()		
	
	print("\nAll dependencies....")
	for filename in all_deps:
		print(filename)				
		
	return all_deps
		
	
# ////////////////////////////////////////////////
def SetRPath(value):

		for filename in glob.glob("*.so"):
			ExecuteCommand(["patchelf","--set-rpath",value, filename])
		
		for filename in glob.glob("bin/*.so"):
			ExecuteCommand(["patchelf","--set-rpath",value, filename])
	
		for filename in ("bin/visus","bin/visusviewer"):
			ExecuteCommand(["patchelf","--set-rpath",value, filename])
	
	

# ///////////////////////////////////////////////
# apple only
def MyDeploy(Qt5_HOME):
	
	print("executing MyDeploy",Qt5_HOME)
	
	if not Qt5_HOME:
		raise Exception("internal error")
		
	if WIN32:

		ExecuteCommand([Qt5_HOME + "/bin/windeployqt.exe", "bin/visusviewer.exe",
				"--libdir","./bin/qt/bin",
				"--plugindir","./bin/qt/plugins",
				"--no-translations"])
				
	elif APPLE:
		
		print("Qt5_HOME",Qt5_HOME)
		
		Qt5_HOME_REAL=os.path.realpath(Qt5_HOME)
		print("Qt5_HOME_REAL", Qt5_HOME_REAL)		
		
		qt_deps=("QtCore","QtDBus","QtGui","QtNetwork","QtPrintSupport","QtQml","QtQuick","QtSvg","QtWebSockets","QtWidgets","QtOpenGL")
		qt_plugins=("iconengines","imageformats","platforms","printsupport","styles")
	
		# copy Qt5 frameworks
		for it in qt_deps:
			CopyDirectory(Qt5_HOME + "/lib/" + it + ".framework","./bin/qt/lib")
		
			# copy Qt5 plugins 
		for it in qt_plugins:
			CopyDirectory(Qt5_HOME + "/plugins/" + it ,"./bin/qt/plugins")
			
		# ShowDeps()
	
		# remove Qt absolute path
		for filename in FindAllBinaries():
			ExecuteCommand(["chmod","u+rwx",filename])
			
			for qt_dep in qt_deps:
				ExecuteCommand(["install_name_tool","-change",Qt5_HOME      + "/lib/{0}.framework/Versions/5/{0}".format(qt_dep), "@rpath/{0}.framework/Versions/5/{0}".format(qt_dep),filename])
				ExecuteCommand(["install_name_tool","-change",Qt5_HOME_REAL + "/lib/{0}.framework/Versions/5/{0}".format(qt_dep), "@rpath/{0}.framework/Versions/5/{0}".format(qt_dep),filename])			
				
		# fix rpath on OpenVisus swig libraries
		for filename in glob.glob("*.so"):
			ExecuteCommand([ "install_name_tool","-delete_rpath",os.getcwd()+"/bin",filename])
			ExecuteCommand([ "install_name_tool","-add_rpath","@loader_path/bin",filename])
			ExecuteCommand([ "install_name_tool","-add_rpath","@loader_path/bin/qt/lib",filename])
			
		# fix rpath for OpenVius dylibs
		for filename in glob.glob("bin/*.dylib"):		
			ExecuteCommand(["install_name_tool","-delete_rpath",os.getcwd()+"/bin",	filename])	
			ExecuteCommand(["install_name_tool","-add_rpath","@loader_path",filename])	
			ExecuteCommand(["install_name_tool","-add_rpath","@loader_path/qt/lib",filename])	
				
	
		# fix rpath for OpenVisus apps
		for filename in ["%s/Contents/MacOS/%s"   % (it,GetFilenameWithoutExtension(it)) for it in glob.glob("bin/*.app")]:
			ExecuteCommand(["install_name_tool","-delete_rpath",os.getcwd()+"/bin",filename])			
			ExecuteCommand(["install_name_tool","-add_rpath","@loader_path/../../..",filename])			
			ExecuteCommand(["install_name_tool","-add_rpath","@loader_path/../../../qt/lib",filename])			
				
			
		# fir rpath for Qt5 frameworks
		for filename in ["%s/Versions/Current/%s" % (it,GetFilenameWithoutExtension(it)) for it in glob.glob("bin/qt/lib/*.framework")]:
			ExecuteCommand(["install_name_tool","-add_rpath","@loader_path/../../..",filename])	
		
		# fix rpath for Qt5 plugins (assuming 2-level plugins)
		for filename in glob.glob("bin/qt/plugins/**/*.dylib", recursive=True):
			ExecuteCommand(["install_name_tool","-add_rpath","@loader_path/../../..",filename])			
			
		ShowDeps()		
		
	else:
		
		qt_deps=("Qt5Core","Qt5DBus","Qt5Gui","Qt5Network","Qt5Svg","Qt5Widgets","Qt5XcbQpa","Qt5OpenGL")
		
		CreateDirectory("bin/qt/lib")
		for it in qt_deps:
			for file in glob.glob("{0}/lib/lib{1}.so*".format(Qt5_HOME,it)):
				shutil.copy(file, "bin/qt/lib/")		
			
		CopyDirectory(Qt5_HOME+ "/plugins","./bin/qt") 		
		SetRPath("$ORIGIN:$ORIGIN/bin:$ORIGIN/qt/lib")
	


# ///////////////////////////////////////////////
# apple only
def AddRPath(value):
	for filename in FindAllBinaries():
		ExecuteCommand(["install_name_tool","-add_rpath",value,filename])				
	

  
# //////////////////////////////////////////
if __name__ == "__main__":
	
	if sys.argv[1]=="--my-deploy":
		MyDeploy(sys.argv[2])	
		sys.exit(0)
		


		

