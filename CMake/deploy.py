import os, sys, glob, subprocess, platform, shutil
# *** NOTE: this file must be self-contained ***

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

WIN32=platform.system()=="Windows" or platform.system()=="win32"
APPLE=platform.system()=="Darwin"

	# /////////////////////////////////////////////////////////////////////////
def CreateDirectory(value):
	try: 
		os.makedirs(value)
	except OSError:
		if not os.path.isdir(value):
			raise

# /////////////////////////////////////////////////////////////////////////
def ReadTextFile(filename):
	file = open(filename, "r") 
	ret=file.read().strip()
	file.close()
	return ret

# /////////////////////////////////////////////////////////////////////////
def WriteTextFile(filename,content):
	content=content if isinstance(content, str) else "\n".join(content)+"\n"
	CreateDirectory(os.path.dirname(os.path.realpath(filename)))
	file = open(filename,"wt") 
	file.write(content) 
	file.close() 	

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
def CopyDirectory(src,dst):
	src=os.path.realpath(src)
	CreateDirectory(dst)
	dst=dst+"/" + os.path.basename(src)
	shutil.rmtree(dst,ignore_errors=True) # remove old
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
def PostInstall(Qt5_HOME):
	
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
	


# ////////////////////////////////////////////////
def RemoveQt5():
	shutil.rmtree("bin/qt",ignore_errors=True)	


# ////////////////////////////////////////////////
def InstallPyQt5(needed):
	major,minor=needed.split('.')[0:2]

	current=[0,0,0]
	try:
		from PyQt5 import Qt
		current=str(Qt.qVersion()).split('.')
	except:
	  pass
	
	
	if major==current[0] and minor==current[1]:
		print("installed Pyqt5",current,"is compatible with",needed)
		return

	print("Installing a new PyQt5 compatible with",needed)

	bIsConda=False
	try:
		import conda.cli
		bIsConda=True
	except:
		pass
		
	if bIsConda:
		conda.cli.main('conda', 'install',  '-y', "pyqt={}.{}".format(major,minor))
	else:
		cmd=[sys.executable,"-m","pip","install","--user","PyQt5~={}.{}.0".format(major,minor),"--progress-bar","off"]
		print("# Executing",cmd)
		if subprocess.call(cmd)!=0:
			raise Exception("Cannot install PyQt5")
	

# ///////////////////////////////////////////////
def AddRPath(value):
	for filename in FindAllBinaries():
		ExecuteCommand(["install_name_tool","-add_rpath",value,filename])				
	
# ////////////////////////////////////////////////
def LinkPyQt5():

	PyQt5_HOME=GetCommandOutput([sys.executable,"-c","import os,PyQt5;print(os.path.dirname(PyQt5.__file__))"]).strip()
	print("PyQt5_HOME",PyQt5_HOME)
	if not os.path.isdir(PyQt5_HOME):
		print("Error directory does not exists")
		raise Exception("internal error")

	# on windows it's enough to use sys.path (see VisusGui.i %pythonbegin section)
	if WIN32:
		return

	if APPLE:
		AddRPath(os.path.join(PyQt5_HOME,'Qt/lib'))
	else:
		SetRPath("$ORIGIN:$ORIGIN/bin:" + os.path.join(PyQt5_HOME,'Qt/lib'))
		
	print("done",action)



# /////////////////////////////////////////////////////////////////////////
__scripts={}

__scripts["WIN32"]=r"""
cd %~dp0
set PYTHON=${Python_EXECUTABLE}
set PATH=%PYTHON%\..;%PATH%;.\bin
${TARGET_FILENAME}" %*
"""

__scripts["WIN32-qt5"]=r"""
cd %~dp0
set PYTHON=${Python_EXECUTABLE}
set PATH=%PYTHON%\..;%PATH%;.\bin 
set Qt5_DIR=bin\Qt
set PATH=%Qt5_DIR%\bin;%PATH%
set QT_PLUGIN_PATH=%Qt5_DIR%\plugins
"${TARGET_FILENAME}" %*
"""

__scripts["WIN32-pyqt5"]=r"""
cd %~dp0
set PYTHON=${Python_EXECUTABLE}
set PATH=%PYTHON%\..;%PATH%;.\bin
for /f "usebackq tokens=*" %%G in (`%PYTHON% -c "import os,PyQt5; print(os.path.dirname(PyQt5.__file__))"`) do set Qt5_DIR=%%G\Qt
set PATH=%Qt5_DIR%\bin;%PATH%
set QT_PLUGIN_PATH=%Qt5_DIR%\plugins
"${TARGET_FILENAME}" %*
"""

__scripts["APPLE"]=r"""
#!/bin/bash
cd $(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
PYTHON=${Python_EXECUTABLE}
export PYTHONPATH=$(${PYTHON} -c "import sys;print(sys.path)"):${PYTHONPATH}
export LD_LIBRARY_PATH=$(${PYTHON} -c "import os,sysconfig;print(os.path.realpath(sysconfig.get_config_var('LIBDIR')))"):${DYLD_LIBRARY_PATH}
${TARGET_FILENAME} "$@"
"""

__scripts["APPLE-qt5"]=r"""
#!/bin/bash
cd $(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
PYTHON=${Python_EXECUTABLE}
export PYTHONPATH=$(${PYTHON} -c "import sys;print(sys.path)"):${PYTHONPATH}
export DYLD_LIBRARY_PATH=$(${PYTHON} -c "import os,sysconfig;print(os.path.realpath(sysconfig.get_config_var('LIBDIR')))"):${DYLD_LIBRARY_PATH}
Qt5_DIR=$(pwd)/bin/qt QT_PLUGIN_PATH=${Qt5_DIR}/plugins ${TARGET_FILENAME} "$@"
"""

__scripts["APPLE-pyqt5"]=r"""
#!/bin/bash
cd $(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
PYTHON=${Python_EXECUTABLE}
export PYTHONPATH=$(${PYTHON} -c "import sys;print(sys.path)"):${PYTHONPATH}
export DYLD_LIBRARY_PATH=$(${PYTHON} -c "import os,sysconfig;print(os.path.realpath(sysconfig.get_config_var('LIBDIR')))"):${DYLD_LIBRARY_PATH}
export Qt5_DIR=$("${PYTHON}" -c "import os,PyQt5; print(os.path.dirname(PyQt5.__file__)+'/Qt')")
export QT_PLUGIN_PATH=${Qt5_DIR}/plugins
${TARGET_FILENAME} "$@"
"""
__scripts["LINUX"]=__scripts["APPLE"].replace("DYLD_LIBRARY_PATH","LD_LIBRARY_PATH")
__scripts["LINUX-qt5"]=__scripts["APPLE-qt5"].replace("DYLD_LIBRARY_PATH","LD_LIBRARY_PATH")
__scripts["LINUX-pyqt5"]=__scripts["APPLE-pyqt5"].replace("DYLD_LIBRARY_PATH","LD_LIBRARY_PATH")

# /////////////////////////////////////////////////////////////////////////
def GenerateScript(script_filename, target_filename, content):
	content=content.replace("${Python_EXECUTABLE}"   , sys.executable)
	content=content.replace("${Python_VERSION_MAJOR}", str(sys.version_info[0]))
	content=content.replace("${Python_VERSION_MINOR}", str(sys.version_info[1]))
	content=content.replace("${TARGET_FILENAME}"   , target_filename)
	WriteTextFile(script_filename , content)
	os.chmod(script_filename, 0o777)



# ////////////////////////////////////////////////
def GenerateScripts(gui_lib):
	if WIN32:
		GenerateScript("visus.bat","bin\\visus.exe",__scripts["WIN32"])
		GenerateScript("visusviewer.bat","bin\\visusviewer.exe",__scripts["WIN32-" + gui_lib])
	elif APPLE:
		GenerateScript("visus.command","bin/visus.app/Contents/MacOS/visus",__scripts["APPLE"])
		GenerateScript("visusviewer.command","bin/visusviewer.app/Contents/MacOS/visusviewer",__scripts["APPLE-"+gui_lib])
	else:
		GenerateScript("visus.sh","bin/visus",__scripts["LINUX"])
		GenerateScript("visusviewer.sh","bin/visusviewer",__scripts["LINUX-"+gui_lib])

# ////////////////////////////////////////////////
def Main():

	action=sys.argv[1].upper()

	# _____________________________________________
	if action=="POST_INSTALL":
		Qt5_HOME=sys.argv[2]
		print("executing",action,"os.getcwd()",os.getcwd(),"Qt5_HOME",Qt5_HOME)
		PostInstall(Qt5_HOME)	
		GenerateScripts("qt5")
		sys.exit(0)

	# _____________________________________________
	if action=="DIRNAME":
		print(os.path.dirname(__file__))
		sys.exit(0)

	# _____________________________________________
	if action=="GENERATE-SCRIPTS":
		this_dir=os.path.dirname(__file__)
		os.chdir(this_dir)
		print("Executing",action)
		GenerateScripts(sys.argv[2])
		sys.exit(0)

	# _____________________________________________
	if action=="CONFIGURE":
		this_dir=os.path.dirname(__file__)
		os.chdir(this_dir)
		print("Executing",action)
		RemoveQt5()
		InstallPyQt5(ReadTextFile("QT_VERSION"))
		LinkPyQt5()
		GenerateScripts("pyqt5")
		sys.exit(0)

	print("EXCEPTION Unknown argument",action)
	sys.exit(-1)
  
# //////////////////////////////////////////
if __name__ == "__main__":
	Main()


		


		

