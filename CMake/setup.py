import os
import sys
import shutil
import platform
import glob
import atexit
import setuptools
	
PROJECT_NAME="OpenVisus"

# use a number like 1.0.xxxx for travis testing
# otherwise use a number greater than the one uploaded in pip
# PROJECT_VERSION="1.0.1000"
PROJECT_VERSION="1.3.19"

# ////////////////////////////////////////////////////////////////////
def findFilesInCurrentDirectory():
	# this are cached directories that should not be part of OpenVisus distribution
	for it in ('./build','./__pycache__','./.git','./{}.egg-info'.format(PROJECT_NAME)):
		shutil.rmtree(it, ignore_errors=True)
	files=[]	
	for dirpath, __dirnames__, filenames in os.walk("."):
		for it in filenames:
			filename= os.path.abspath(os.path.join(dirpath, it))
			if filename.startswith(os.path.abspath('./dist')): continue
			if "__pycache__" in filename: continue	    
			files.append(filename)
	return files
				

# ////////////////////////////////////////////////////////////////////
if __name__ == "__main__":
	setuptools.setup(
	  name = PROJECT_NAME,
	  description = "ViSUS multiresolution I/O, analysis, and visualization system",
	  version=PROJECT_VERSION,
	  url="https://github.com/sci-visus/OpenVisus",
	  author="visus.net",
	  author_email="support@visus.net",
	  packages=[PROJECT_NAME],
	  package_dir={PROJECT_NAME:'.'},
	  package_data={PROJECT_NAME: findFilesInCurrentDirectory()},
	  platforms=['Linux', 'OS-X', 'Windows'],
	  license = "BSD",
	  install_requires=["numpy"])



