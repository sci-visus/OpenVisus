#!/usr/bin/env python

from distutils.core import setup
import os, sys, glob

package_name="visuspy"

target_prefix = sys.prefix

for i in range(len(sys.argv)):
  a = sys.argv[i]
  if a=='--prefix':
    target_prefix=sys.argv[i+1]
    sp = a.split("--prefix=")
    if len(sp)==2:
      target_prefix=sp[1]
      print('Target is:',target_prefix)

# findFilesInDirectory (recursive)
def findFilesInDirectory(dirname):
  this_dir=os.path.dirname(__file__)
  ret=[]
  for (dirpath, dirnames, names) in os.walk(os.path.join(this_dir, dirname)):
    for name in names:
      ret.append(os.path.join(dirpath, name))
  return ret

# removeFiles
def removeFiles(list,toremove):
  for it in toremove:
    if it in list:
      list.remove(it)

files=[]
files+=findFilesInDirectory('Copyrights')
files+=findFilesInDirectory('docs')
files+=findFilesInDirectory('include')
files+=findFilesInDirectory('lib')
files+=findFilesInDirectory('resources')

files+=glob.glob('*.config')
files+=glob.glob('*.py')
files+=glob.glob('*.exe')
files+=glob.glob('*.dll')
files+=glob.glob('*.pyd')
files+=glob.glob('*.so')
files+=glob.glob('*.dylib')
files+=glob.glob('*.bat')
files+=glob.glob('*.sh')
files+=glob.glob('*.command')

apps=glob.glob("*.app")
for app in apps:
  files+=findFilesInDirectory(app)

removeFiles(files,glob.glob("Qt5*.dll"))
removeFiles(files,glob.glob("python*.dll"))

# going to use PyQt5 libaries
#for it in glob.glob("Qt*.framework"):
#  files+=findFilesInDirectory(it)

def fixRPathApple(dir):
  import subprocess
  import PyQt5
  visuspy_dir=dir+"/"+package_name
  pyqt5_dir=os.path.dirname(PyQt5.__file__) + "/Qt/lib"
  
  targets=\
    glob.glob(visuspy_dir+"/*.dylib") + \
    glob.glob(visuspy_dir+"/*.so") + \
    [visuspy_dir+"/"+app+"/Contents/MacOS/"+app.split(".")[0] for app in apps]

  for target in targets:
    cmd=["/usr/bin/install_name_tool","-add_rpath",pyqt5_dir,target]
    print(" ".join(cmd))
    subprocess.Popen(cmd)

def postInstall(dir):
  if sys.platform == "darwin":
    fixRPathApple(dir)

from distutils.command.install import install
class myInstall(install):
  def run(self):
    install.run(self)
    self.execute(postInstall, (self.install_lib,),msg="Running post install task")

setup (name = package_name,
       description = "ViSUS multiresolution I/O, analysis, and visualization system",
       version='2.0.0',
       url = "http://visus.net",
       cmdclass={'install': myInstall},
       packages=[package_name],
       package_dir={package_name:''},
       package_data={package_name: files}
       )

