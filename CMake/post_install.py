import os, sys, glob
import subprocess
import shutil
import platform

# ////////////////////////////////////////////////////
def getArg(index):
  return str(sys.argv[index]) if len(sys.argv)>index else ""

# ////////////////////////////////////////////////////
def executeCommand(args,bVerbose=True):
  if bVerbose: print("  "," ".join(args))
  subprocess.Popen(args)  

# ////////////////////////////////////////////////////
def existDirectory(path):
  return os.path.isdir(path)

# ////////////////////////////////////////////////////
def existFile(path):
  return os.path.isfile(path)

# ////////////////////////////////////////////////////
def copyDirectory(src,dst):

  if existDirectory(dst):
    print("Directory",dst,"exists. Skipping")
    return

  print("Copying",src,dst)
  shutil.copytree(src, dst + "/")

qt_dir,qt_deploy=getArg(1),getArg(2)

print(platform.system(),qt_dir,qt_deploy)

# /////////////////////////////////////////////////////////////////
if platform.system() =="Windows":

	if existFile("bin/visusviewer_d.exe"):
		executeCommand([qt_deploy,"bin/visusviewer_d.exe"])

	if existFile("bin/visusviewer.exe"):
		executeCommand([qt_deploy,"bin/visusviewer.exe"])
	
	sys.exit(0)

# /////////////////////////////////////////////////////////////////
if platform.system()=="Darwin":

	otool="/usr/bin/install_name_tool"
	qt_version=5
	qt_frameworks=("QtOpenGL","QtWidgets","QtGui","QtCore","QtSvg","QtPrintSupport")
	qt_plugins=("iconengines","imageformats","platforms","printsupport")
	def qtFrameworkShortPath(prefix,name) : return prefix + "/" + name + ".framework"
	def qtFrameworkFullPath(prefix,name) : return qtFrameworkShortPath(prefix,name)+"/Versions/" + str(qt_version) + "/" + name
	def otoolAddRPath(dir,target): return executeCommand([otool,"-add_rpath",dir,target])
	def otoolChange(old_path,new_path,target): return executeCommand([otool,"-change",old_path,new_path,target])

	libs=glob.glob("./bin/*.dylib") + glob.glob("./bin/*.so") 
	apps=glob.glob("./bin/*.app")

	# fixing libs
	for target in libs:
		print("Fixing rpath of ",target)

		for lib in libs:
			if target==lib: continue
			lib_basename=os.path.basename(lib)
			otoolChange(lib_basename,"@loader_path/"+lib_basename,target)

		for qt_framework in qt_frameworks:
			otoolChange(qtFrameworkFullPath(qt_dir + "/lib",qt_framework), qtFrameworkFullPath("@loader_path/Frameworks",qt_framework), target)   

	# fixing apps
	for app in apps:
		print("Fixing rpath of ",app)

		executable=app + "/Contents/MacOS/" + os.path.splitext(os.path.basename(app))[0]

		for lib in libs:
			lib_basename=os.path.basename(lib)
			otoolChange(lib_basename,"@loader_path/../../../"+lib_basename,executable)

		for qt_framework in qt_frameworks:
			otoolChange(qtFrameworkFullPath(qt_dir + "/lib",qt_framework),qtFrameworkFullPath("@loader_path/../../../Frameworks",qt_framework), executable)

	# qt deploy
	for qt_framework in qt_frameworks:
		copyDirectory(qtFrameworkShortPath(qt_dir  + "/lib/",qt_framework),qtFrameworkShortPath("./bin/Frameworks",qt_framework))

	for qt_plugin in qt_plugins:
		copyDirectory(qt_dir + "/plugins/"+qt_plugin,"./bin/Plugins/"+qt_plugin)

	sys.exit(0)
 