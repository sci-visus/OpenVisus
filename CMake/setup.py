import os, sys, setuptools

# ///////////////////////////////////////////////////////
def findFiles(dir,ignore_directories=("build", "dist","visuspy.egg-info")):
  ret=[]
  for root, dirnames, filenames in os.walk(dir):
    bIgnore=False
    for purge_dir in ignore_directories:
      if root.startswith(os.path.join(dir,purge_dir)) :
        bIgnore=True
    if bIgnore: continue
    for filename in filenames:
      ret.append(os.path.join(root, filename))
  return ret
  
# ///////////////////////////////////////////////////////
def readTextFile(filename):
  with open(filename, "r") as file:
    return file.read()  

setuptools.setup(
  name = "visuspy",
  description = "ViSUS multiresolution I/O, analysis, and visualization system",
  version='1.0.0',
  url="https://github.com/sci-visus/OpenVisus",
  author="visus.net",
  author_email="support@visus.net​",
  long_description=readTextFile("README.md"),
  long_description_content_type="text/markdown",
  packages=["visuspy"],
  package_dir={"visuspy":''},
  package_data={"visuspy": findFiles(os.path.abspath(os.path.dirname(__file__)))},
  classifiers=(
    "Programming Language :: Python :: 3",
    "License :: OSI Approved :: BSD License",
    'Operating System :: MacOS :: MacOS X',
    'Operating System :: Microsoft :: Windows',
    'Operating System :: POSIX',
    'Operating System :: Unix',
  ),
  platforms=['Linux', 'OS-X', 'Windows'],
  license = "BSD",
  install_requires=[
    'numpy', 
    "PyQt5; python_version >= '5.9'",
  ],
)
