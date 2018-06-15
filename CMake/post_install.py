import os, sys, glob
import subprocess
import shutil
import platform
import tarfile


WIN32=True if platform.system() == "Windows" else False
APPLE=True if platform.system() == "Darwin"  else False

# ////////////////////////////////////////////////////
def getArg(index):
  return str(sys.argv[index]) if len(sys.argv)>index else ""

# ////////////////////////////////////////////////////
def executeCommand(args):
  print(" ".join(args))
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
  
# /////////////////////////////////////////////////////////////////
def copyFileIfExist(src,dst):
    if existFile(src):
        executeCommand(["cp",src,dst])    
    
# ////////////////////////////////////////////////////
def copyDirectoryIfExist(src,dst):
    if existDirectory(dst):
        return copyDirectory(src,dst)

# ////////////////////////////////////////////////////
def deployToFile():

	filename="OpenVisus."+ platform.system()
	format="zip" if WIN32 else "gztar"
	compress_dir="./"

	print("shutil.make_archive('%s', '%s', '%s')" % (filename,format,compress_dir))
	shutil.make_archive(filename, format, compress_dir)



qt_dir,qt_deploy=getArg(1),getArg(2)

print("sys.argv:",sys.argv)
print("platform.system():",platform.system())
print("qt_dir:",qt_dir)
print("qt_deploy:",qt_deploy)

# /////////////////////////////////////////////////////////////////
if WIN32:
  
  for app in glob.glob("./bin/*.exe"):
    executeCommand([qt_deploy,app])

  
# /////////////////////////////////////////////////////////////////
elif APPLE:

  qt_dir=os.path.realpath(qt_dir+"/../../..")
  otool="/usr/bin/install_name_tool"
  qt_frameworks=("QtOpenGL","QtWidgets","QtGui","QtCore","QtSvg","QtPrintSupport")
  qt_plugins=("iconengines","imageformats","platforms","printsupport")

  def qtFrameworkShortPath(prefix,name) : return prefix + "/" + name + ".framework"
  def qtFrameworkFullPath(prefix,name) : return qtFrameworkShortPath(prefix,name)+"/Versions/5/" + name
  def otoolAddRPath(dir,target): return executeCommand([otool,"-add_rpath",dir,target])
  def otoolChange(old_path,new_path,target): return executeCommand([otool,"-change",old_path,new_path,target])

  # fixing libs
  libs=glob.glob("./bin/*.dylib") + glob.glob("./bin/*.so") 
  for target in libs:

  	for lib in libs:
  		if target==lib: continue
  		lib_basename=os.path.basename(lib)
  		otoolChange(lib_basename,"@loader_path/"+lib_basename,target)

  	for qt_framework in qt_frameworks:
  		otoolChange(qtFrameworkFullPath(qt_dir + "/lib",qt_framework), qtFrameworkFullPath("@loader_path/Frameworks",qt_framework), target)   

  # fixing apps
  apps=glob.glob("./bin/*.app")
  for app in apps:

  	executable=app + "/Contents/MacOS/" + os.path.splitext(os.path.basename(app))[0]

  	for lib in libs:
  		lib_basename=os.path.basename(lib)
  		otoolChange(lib_basename,"@loader_path/../../../"+lib_basename,executable)

  	for qt_framework in qt_frameworks:
  		otoolChange(qtFrameworkFullPath(qt_dir + "/lib",qt_framework),qtFrameworkFullPath("@loader_path/../../../Frameworks",qt_framework), executable)

  # qt deploy
  for qt_framework in qt_frameworks:
  	copyDirectory(qtFrameworkShortPath(qt_dir  + "/lib",qt_framework),qtFrameworkShortPath("./bin/Frameworks",qt_framework))

  for qt_plugin in qt_plugins:
  	copyDirectory(qt_dir + "/plugins/"+qt_plugin,"./bin/Plugins/"+qt_plugin)


# /////////////////////////////////////////////////////////////////
else:

	# copy qt libs
	for lib in ("libQt5OpenGL.so.5","libQt5Widgets.so.5","libQt5Gui.so.5","libQt5Core.so.5","libQt5Svg.so.5","libQt5PrintSupport.so.5"):
		copyFileIfExist("/usr/lib/x86_64-linux-gnu/"+lib,"bin/")
		copyFileIfExist("/usr/lib64/"+lib,"bin/")       

	# copy qt plugins
	for plugin in ("iconengines","imageformats","platforms","printsupport"):
		copyDirectoryIfExist("/usr/lib/x86_64-linux-gnu/qt5/plugins/"+plugin,"bin/plugins/"+plugin)
		copyDirectoryIfExist("/usr/lib64/qt5/plugins/"+plugin,"bin/plugins/"+plugin)
		
	
deployToFile()


