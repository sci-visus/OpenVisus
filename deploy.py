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


import sysconfig

this_dir=os.path.dirname(os.path.abspath(__file__))

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
	
# ////////////////////////////////////////////////
def GetScriptExtention():
	if WIN32:
		return ".bat"
	elif APPLE:
		return ".command"
	else:
		return ".sh"	
	
# /////////////////////////////////////////////////////////////////////////
def ExecuteCommand(cmd,bVerbose=False):	
	
	if not WIN32 and os.path.splitext(cmd[0])[1]==GetScriptExtention():
		cmd=["bash"] + cmd
	
	if bVerbose: print("Executing command", cmd)
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
		ExecuteCommand(["patchelf","--set-rpath",value, filename],bVerbose=True)
		
	for filename in glob.glob("bin/*.so"):
		ExecuteCommand(["patchelf","--set-rpath",value, filename],bVerbose=True)
	
	for filename in ("bin/visus","bin/visusviewer"):
		ExecuteCommand(["patchelf","--set-rpath",value, filename],bVerbose=True)
	

# ///////////////////////////////////////////////
# apple only
def InstallQt5(Qt5_HOME="",bDebug=False):
	
	if not Qt5_HOME:
		raise Exception("internal error")
		
	if WIN32:

		ExecuteCommand([Qt5_HOME + "/bin/windeployqt.exe", "bin/visusviewer.exe",
				"--debug" if bDebug else "--release",
				"--libdir","./bin/qt/bin",
				"--plugindir","./bin/qt/plugins",
				"--no-translations"],bVerbose=True)
				
	elif APPLE:
		
		print("Qt5_HOME",Qt5_HOME)
		
		Qt5_HOME_REAL=os.path.realpath(Qt5_HOME)
		print("Qt5_HOME_REAL", Qt5_HOME_REAL)		
		
		# copy Qt5 frameworks
		qt_deps=("QtCore","QtDBus","QtGui","QtNetwork","QtPrintSupport","QtQml","QtQuick","QtSvg","QtWebSockets","QtWidgets","QtOpenGL")
		for it in qt_deps:
			try:
				CopyDirectory(Qt5_HOME + "/lib/" + it + ".framework","./bin/qt/lib")
			except:
				pass
		
			# copy Qt5 plugins 
			qt_plugins=("iconengines","imageformats","platforms","printsupport","styles")
		for it in qt_plugins:
			try:
				CopyDirectory(Qt5_HOME + "/plugins/" + it ,"./bin/qt/plugins")
			except:
				pass
			
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
	print("Removing Qt5...")
	shutil.rmtree("bin/qt",ignore_errors=True)	


# ////////////////////////////////////////////////
def InstallPyQt5(needed):

	print("Installing PyQt5...",needed)

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
		ExecuteCommand(["install_name_tool","-add_rpath",value,filename],bVerbose=True)				
	
# ////////////////////////////////////////////////
def LinkPyQt5():

	print("Linking to PyQt5...")

	try:
		import PyQt5
		PyQt5_HOME=os.path.dirname(PyQt5.__file__)
	
	except:
		# this should cover the case where I just installed PyQt5
		PyQt5_HOME=GetCommandOutput([sys.executable,"-c","import os,PyQt5;print(os.path.dirname(PyQt5.__file__))"]).strip()

	
	print("PyQt5_HOME",PyQt5_HOME)
	if not os.path.isdir(PyQt5_HOME):
		print("Error directory does not exists")
		raise Exception("internal error")

	# on windows it's enough to use sys.path (see *.i %pythonbegin section)
	if WIN32:
		return

	if APPLE:
		AddRPath(os.path.join(PyQt5_HOME,'Qt/lib'))
	else:
		SetRPath("$ORIGIN:$ORIGIN/bin:" + os.path.join(PyQt5_HOME,'Qt/lib'))



# /////////////////////////////////////////////////////////////////////////
__scripts={
"WIN32-nogui" : r"""
cd %~dp0
set PATH=${__python__}\..;%PATH%;.\bin
set PYTHONPATH=${__pythonpath__}
${__target__}" %*
""",

"WIN32-qt5": r"""
cd %~dp0
set PATH=${__python__}\..;.\bin;bin\Qt\bin;%PATH%
set PYTHONPATH=${__pythonpath__}
set QT_PLUGIN_PATH=bin\Qt\plugins
"${__target__}" %*
""",

"WIN32-pyqt5": r"""
cd %~dp0
set PATH=${__python__}\..;.\bin;${__pyqt5_dir__}\Qt\bin;%PATH%
set PYTHONPATH=${__pythonpath__}
set QT_PLUGIN_PATH=${__pyqt5_dir__}\Qt\plugins
"${__target__}" %*
""",

"APPLE-nogui" : r"""
#!/bin/bash
cd $(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
export PYTHONPATH=${__pythonpath__}
export LD_LIBRARY_PATH=${__libdir__}:${DYLD_LIBRARY_PATH}
${__target__} "$@"
""",

"APPLE-qt5" : r"""
#!/bin/bash
cd $(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
export PYTHONPATH=${__pythonpath__}
export DYLD_LIBRARY_PATH=${__libdir__}:${DYLD_LIBRARY_PATH}
export QT_PLUGIN_PATH=$(pwd)/bin/qt/plugins 
${__target__} "$@"
""",

"APPLE-pyqt5" : r"""
#!/bin/bash
cd $(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
export PYTHONPATH=${__pythonpath__}
export DYLD_LIBRARY_PATH=${__libdir__}:${DYLD_LIBRARY_PATH}
export QT_PLUGIN_PATH=${__pyqt5_dir__}/Qt/plugins
${__target__} "$@"
""",

"LINUX-nogui": r"""
#!/bin/bash
cd $(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
export PYTHONPATH=${__pythonpath__}
export LD_LIBRARY_PATH=${__libdir__}:${LD_LIBRARY_PATH}
${__target__} "$@"
""",

"LINUX-qt5" : r"""
#!/bin/bash
cd $(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
export PYTHONPATH=${__pythonpath__}
export LD_LIBRARY_PATH=${__libdir__}:${LD_LIBRARY_PATH}
export QT_PLUGIN_PATH=$(pwd)/bin/qt/plugins 
${__target__} "$@"
""",

"LINUX-pyqt5": r"""
#!/bin/bash
cd $(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
export PYTHONPATH=${__pythonpath__}
export LD_LIBRARY_PATH=${__libdir__}:${LD_LIBRARY_PATH}
export QT_PLUGIN_PATH=${__pyqt5_dir__}/Qt/plugins
${__target__} "$@"
"""
}

# /////////////////////////////////////////////////////////////////////////
def GenerateScript(script_filename, target_filename, gui_lib):
	
	if WIN32:
		script_filename+=".bat"
		target_filename=target_filename.replace("/","\\")+".exe"
		content=__scripts["WIN32-"+gui_lib]
	elif APPLE:
		script_filename+=".command"
		target_filename+=".app/Contents/MacOS/" +GetFilenameWithoutExtension(target_filename)
		content=__scripts["APPLE-"+gui_lib]
	else:
		script_filename+=".sh"
		content=__scripts["LINUX-"+gui_lib]
	
	print("Generate script",script_filename,target_filename)
	content=content.replace("${__python__}", sys.executable)
	content=content.replace("${__target__}", target_filename)
	
	if True:
		PYTHONPATH=sys.path
		# PYTHONPATH.remove(this_dir)
		content=content.replace("${__pythonpath__}", os.pathsep.join(PYTHONPATH))
	
	if not WIN32:
		content=content.replace("${__libdir__}", os.path.realpath(sysconfig.get_config_var('LIBDIR')))
		
	if gui_lib=="pyqt5":
		import PyQt5
		content=content.replace("${__pyqt5_dir__}", os.path.dirname(PyQt5.__file__))
	
	WriteTextFile(script_filename , content)
	
	try:
		os.chmod(script_filename, 0o777)
	except:
		pass

# ////////////////////////////////////////////////
def GenerateScripts(gui_lib):
	print("Generating scripts","gui_lib","...")
	GenerateScript("visus","bin/visus","nogui")
	GenerateScript("visusviewer","bin/visusviewer",gui_lib)

# ////////////////////////////////////////////////
def Main():

	if len(sys.argv)==1:
		return

	action=sys.argv[1]

	# _____________________________________________
	if action=="dirname":
		print(this_dir)
		sys.exit(0)

	print([sys.executable,"-m","OpenVisus"] + sys.argv)
	print("this_dir",this_dir)

	# _____________________________________________
	if action=="test":
		for filename in ["Array.py","Dataflow.py","Dataflow2.py","Idx.py"]: # ,"XIdx.py" ,"DataConversion1.py","DataConversion2.py"
			ExecuteCommand([sys.executable,os.path.join(this_dir, "Samples", "python", filename)],bVerbose=True) 
		sys.exit(0)

	# _____________________________________________
	if action=="convert":
		ExecuteCommand([os.path.join(this_dir, "visus" + GetScriptExtention())] + sys.argv[2:], bVerbose=True)
		sys.exit(0)
	
	# _____________________________________________
	if action=="viewer":
		ExecuteCommand([os.path.join(this_dir, "visusviewer" + GetScriptExtention())] + sys.argv[2:], bVerbose=True)
		sys.exit(0)

	# _____________________________________________
	if action=="install-qt5":
		os.chdir(this_dir)
		InstallQt5(Qt5_HOME=sys.argv[2],bDebug="Debug" in sys.argv)	
		GenerateScripts("qt5")
		print(action,"done")
		sys.exit(0)

	# _____________________________________________
	if action=="generate-scripts":
		os.chdir(this_dir)
		GenerateScripts(sys.argv[2])
		print(action,"done")
		sys.exit(0)

	# _____________________________________________
	if action=="remove-qt5":
		os.chdir(this_dir)
		RemoveQt5()
		print(action,"done")
		sys.exit(0)
	# _____________________________________________
	if action=="install-pyqt5":
		os.chdir(this_dir)
		QT_VERSION=ReadTextFile("QT_VERSION")
		print("QT_VERSION",QT_VERSION)
		InstallPyQt5(QT_VERSION)
		print(action,"done")
		sys.exit(0)

	# _____________________________________________
	if action=="link-pyqt5":
		os.chdir(this_dir)
		LinkPyQt5()
		print(action,"done")
		sys.exit(0)

	# _____________________________________________
	if action=="configure":
		os.chdir(this_dir)
		RemoveQt5()
		QT_VERSION=ReadTextFile("QT_VERSION")
		print("QT_VERSION",QT_VERSION)
		InstallPyQt5(QT_VERSION)
		LinkPyQt5()
		GenerateScripts("pyqt5")
		print(action,"done")
		sys.exit(0)

	print("Wrong arguments",sys.argv)
	sys.exit(-1)
  
# //////////////////////////////////////////
if __name__ == "__main__":
	Main()


		


		

