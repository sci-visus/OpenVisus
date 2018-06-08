import os, sys, glob
import subprocess

qt_old_lib_dir="/usr/local/opt/qt/lib"

# list of all qt frameworks you are using
qt_frameworks=("QtOpenGL","QtWidgets","QtGui","QtCore","QtSvg","QtPrintSupport")

# otool binary
otool="/usr/bin/install_name_tool"

# all the dynamic libraries that need to be fixed
libs=glob.glob("./bin/*.dylib") + glob.glob("./bin/*.so") 

# all the aps
apps=glob.glob("./bin/*.app")

def copyDirectory(src,dst):
  import shutil
  shutil.copytree(src, dst + "/")

def executeCommand(args):
  #print("  "," ".join(args))
  subprocess.Popen(args)  
  
def otoolAddRPath(dir,target):
  return executeCommand([otool,"-add_rpath",dir,target])

def otoolChange(old_path,new_path,target):
  return executeCommand([otool,"-change",old_path,new_path,target])

# copy qt frameworks into bin
for qt_framework in qt_frameworks:
  src_dir=qt_old_lib_dir+"/"+qt_framework+".framework"
  dst_dir="./bin/"+qt_framework+".framework"
  if not os.path.isdir(dst_dir):
    print("Copying",src_dir,dst_dir)
    copyDirectory(src_dir,dst_dir)

# todo ? copy plugins?
# ls visusviewer.app/Contents/PlugIns/
# iconengines 
# imageformats  
# platforms 
# printsupport

# todo ? create a qt.conf for each app?
# more visusviewer.app/Contents/Resources/qt.conf 
# [Paths]
# Plugins = PlugIns
# Imports = Resources/qml
# Qml2Imports = Resources/qml

# fix libraries
for target in libs:
  print("Fixing rpath of ",target)

  for lib in libs:
    if target==lib: continue
    lib_basename=os.path.basename(lib)
    otoolChange(lib_basename,"@loader_path/"+lib_basename,target)
  
  for qt_framework in qt_frameworks:
    otoolChange(qt_old_lib_dir + "/"+qt_framework+".framework/Versions/5/"+qt_framework,"@loader_path/"+qt_framework+".framework/Versions/5/"+qt_framework, target)   

# fix apps
for app in apps:
  print("Fixing rpath of ",app)

  executable=app + "/Contents/MacOS/" + os.path.splitext(os.path.basename(app))[0]
  
  for lib in libs:
    lib_basename=os.path.basename(lib)
    otoolChange(lib_basename,"@loader_path/../../../"+lib_basename,executable)

  for qt_framework in qt_frameworks:
    otoolChange(qt_old_lib_dir + "/"+qt_framework+".framework/Versions/5/"+qt_framework,"@loader_path/../../../"+qt_framework+".framework/Versions/5/"+qt_framework, executable)


 