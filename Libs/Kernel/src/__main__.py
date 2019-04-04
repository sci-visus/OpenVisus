import os
import sys
import platform
import glob

WIN32=platform.system()=="Windows" or platform.system()=="win32"
APPLE=platform.system()=="Darwin"

from Deploy import DeployUtils

if __name__ == '__main__':
	
	__this_dir__=os.path.dirname(os.path.abspath(__file__))
	
	# ___________________________________________________________________
	if sys.argv[1]=="dirname":
		print(__this_dir__)
		sys.exit(0)	

	# ___________________________________________________________________
	if sys.argv[1]=="configure":
		
		os.chdir(__this_dir__)
	
		print("Executing configure",sys.argv)
		
		VISUS_GUI=True if os.path.isfile("QT_VERSION") else False
		if VISUS_GUI:
			bUsePyQt=not os.path.isdir(os.path.join("bin","Qt","plugins")) or "--use-pyqt" in sys.argv
			if bUsePyQt:
				print("Forcing the use of pyqt")
				QT_VERSION = DeployUtils.ReadTextFile("QT_VERSION")
				DeployUtils.InstallPyQt5(QT_VERSION)
				
				if WIN32:
					pass # see VisusGui.i (%pythonbegin section, I'm using sys.path)
				else:
					import PyQt5
					deploy=AppleDeploy() if APPLE else LinuxDeploy()
					deploy.addRPath(os.path.join(os.path.dirname(PyQt5.__file__),"Qt","lib"))	

				# avoid conflicts removing any Qt file
				DeployUtils.RemoveFiles("bin/Qt*")

		# create scripts
		if True:
			
			env={}
			if VISUS_GUI:
				QT_PLUGIN_PATH = os.path.join("bin","Qt","plugins")
				if not os.path.isdir(QT_PLUGIN_PATH):
					import PyQt5
					QT_PLUGIN_PATH = os.path.join(os.path.dirname(PyQt5.__file__),"Qt","plugins")
				env["QT_PLUGIN_PATH"]=QT_PLUGIN_PATH
			
			if WIN32:
				filenames=glob.glob('bin/*.exe')
			elif APPLE:	
				filenames=["{}/Contents/MacOS/{}".format(app,DeployUtils.GetFilenameWithoutExtension(app))for app in glob.glob('bin/*.app')]
				
			else:
				filenames=[filename for filename in glob.glob('bin/*') if os.path.isfile(filename) and os.access(filename, os.X_OK) and not os.path.splitext(filename)[1] ]
					
			for filename in filenames:
				DeployUtils.CreateScript(filename,env)

		print("finished configure")
		sys.exit(0)
		

	print("Error in arguments",sys.argv)
	sys.exit(-1)
