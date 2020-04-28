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

# ////////////////////////////////////////////////
def PrintDirName():
	this_dir=ThisDir(__file__)
	os.chdir(this_dir)		
	print(this_dir)

# ////////////////////////////////////////////////
def UsePyQt():
	this_dir=ThisDir(__file__)
	os.chdir(this_dir)
	print("Executing",action)
	print("os.getcwd()",os.getcwd())
	print("sys.argv",sys.argv)

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

	PyQt5_HOME=GetCommandOutput([sys.executable,"-c","import os,PyQt5;print(os.path.dirname(PyQt5.__file__))"]).strip()
	print("PyQt5_HOME",PyQt5_HOME)
	if not os.path.isdir(PyQt5_HOME):
		print("Error directory does not exists")
		raise Exception("internal error")
		
	# avoid conflicts removing any Qt file
	RemoveDirectory("bin/qt")		
		
	# for windowss see VisusGui.i (%pythonbegin section, I'm using sys.path)
	if not WIN32:
		
		Qt5_HOME=os.path.join(PyQt5_HOME,'Qt/lib')
		import PyDeploy 
		if APPLE:
			PyDeploy.AddRPath(Qt5_HOME)
		else:
			PyDeploy.SetRPath("$ORIGIN:$ORIGIN/bin:" + Qt5_HOME)
		
	print("done",action)
	


# ////////////////////////////////////////////////////////////////////////////////////////////////
if __name__ == '__main__':

	action=sys.argv[1]

	# _____________________________________________
	if action=="dirname":
		PrintDirName()
		sys.exit(0)
	
	# _____________________________________________
	if action=="use-pyqt":
		UsePyQt()
		sys.exit(0)

	print("EXEPTION Unknown argument " + action)
	sys.exit(-1)



