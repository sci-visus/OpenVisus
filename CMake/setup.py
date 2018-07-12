import os, sys, setuptools
import shutil

#increase this number for PIP
VERSION="1.1.0"
this_dir="."

bSmall = False
if '--small' in sys.argv:
	index = sys.argv.index('--small')
	sys.argv.pop(index)  
	bSmall = True


# findFilesInCurrentDirectory
def findFilesInCurrentDirectory():
	ret=[]
	for dirpath, __dirnames__, filenames in os.walk(this_dir):
	  for filename in filenames:
	    file= os.path.abspath(os.path.join(dirpath, filename))
	    first_dir=dirpath.replace("\\","/").split("/")
	    first_dir=first_dir[1] if len(first_dir)>=2 else ""
			
		 # bSkip?
	    if first_dir in ("build","dist","OpenVisus.egg-info","__pycache__") or (bSmall and first_dir=="debug") or (bSmall and file.endswith(".pdb")):
	    	continue

	    ret.append(file)
	return ret

setuptools.setup(
  name = "OpenVisus",
  description = "ViSUS multiresolution I/O, analysis, and visualization system",
  version=VERSION,
  url="https://github.com/sci-visus/OpenVisus",
  author="visus.net",
  author_email="support@visus.net​",
  packages=["OpenVisus"],
  package_dir={"OpenVisus":'.'},
  package_data={"OpenVisus": findFilesInCurrentDirectory()},
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
    # "PyQt5>='5.9'" removed because I'm having some problems with travis
  ],
)
