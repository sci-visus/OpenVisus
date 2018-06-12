import os, sys, glob
import subprocess
import shutil

qt_dir="/usr/local/opt/qt"

qt_frameworks=("QtOpenGL","QtWidgets","QtGui","QtCore","QtSvg","QtPrintSupport")

qt_plugins=("iconengines","imageformats","platforms","printsupport")

qt_version=5

otool="/usr/bin/install_name_tool"

# ////////////////////////////////////////////////////
def qtFinalPath(prefix,name):
  return prefix + "/"+name+".framework/Versions/"+str(qt_version)+"/"+name

# ////////////////////////////////////////////////////
def existDirectory(path):
  return os.path.isdir(path)

# ////////////////////////////////////////////////////
def copyDirectory(src,dst):

  if existDirectory(dst):
    print("Directory",dst,"exists. Skipping")
    return

  print("Copying",src,dst)
  shutil.copytree(src, dst + "/")

# ////////////////////////////////////////////////////
def executeCommand(args,bVerbose=False):
  if bVerbose: print("  "," ".join(args))
  subprocess.Popen(args)  
  
# ////////////////////////////////////////////////////
def otoolAddRPath(dir,target):
  return executeCommand([otool,"-add_rpath",dir,target])

# ////////////////////////////////////////////////////
def otoolChange(old_path,new_path,target):
  return executeCommand([otool,"-change",old_path,new_path,target])


# ////////////////////////////////////////////////////
def fixLibraries(libs):
  for target in libs:
    print("Fixing rpath of ",target)

    for lib in libs:
      if target==lib: continue
      lib_basename=os.path.basename(lib)
      otoolChange(lib_basename,"@loader_path/"+lib_basename,target)

    for qt_framework in qt_frameworks:
      otoolChange(qtFinalPath(qt_dir + "/lib",qt_framework), qtFinalPath("@loader_path/Frameworks",qt_framework), target)   

# ////////////////////////////////////////////////////
def fixApps(apps,libs):
  for app in apps:
    print("Fixing rpath of ",app)

    executable=app + "/Contents/MacOS/" + os.path.splitext(os.path.basename(app))[0]

    for lib in libs:
      lib_basename=os.path.basename(lib)
      otoolChange(lib_basename,"@loader_path/../../../"+lib_basename,executable)

    for qt_framework in qt_frameworks:
      otoolChange(qtFinalPath(qt_dir + "/lib",qt_framework),qtFinalPath("@loader_path/../../../Frameworks",qt_framework), executable)

# ////////////////////////////////////////////////////
def qtDeploy():

  for qt_framework in qt_frameworks:
    copyDirectory(qt_dir  + "/lib/"+ qt_framework+".framework","./bin/Frameworks/"+qt_framework+".framework")

  for qt_plugin in qt_plugins:
    copyDirectory(qt_dir + "/plugins/"+qt_plugin,"./bin/Plugins/"+qt_plugin)

if __name__ == '__main__':

  libs=glob.glob("./bin/*.dylib") + glob.glob("./bin/*.so") 
  apps=glob.glob("./bin/*.app")
  fixLibraries(libs)
  fixApps(apps,libs)
  qtDeploy()


 