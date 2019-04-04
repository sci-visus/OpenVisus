import os
import sys

if __name__ == '__main__':
	
	# ___________________________________________________________________
	if sys.argv[1]=="dirname":
		print(OpenVisusDir)
		sys.exit(0)	

	# ___________________________________________________________________
	if sys.argv[1]=="configure":
		
		os.chdir(OpenVisusDir)
	
		print("Executing --openvisus-configure",sys.argv)
		
		VISUS_GUI=True if os.path.isfile("QT_VERSION") else False
		if VISUS_GUI:
			bUsePyQt=not os.path.isdir(os.path.join("bin","Qt","plugins")) or "--use-pyqt" in sys.argv
			if bUsePyQt:
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
				filenames=glob.glob('bin/*.app')
			else:
				filenames=[exe for exe in glob.glob('bin/*') if os.path.isfile(exe) and os.access(exe, os.X_OK) and not os.path.splitext(exe)[1] ]:
					
			for filename in filenames:
				name=os.path.splitext(os.path.basename(filename))[0]
				DeployUtils.CreateScript(name,env)

		print("finished --openvisus-configure")
		sys.exit(0)
		

	print("Error in arguments",sys.argv)
	sys.exit(-1)
