import os
import sys
import pip
import shutil
import platform
import glob
import atexit
import setuptools
from configure import *
	
PROJECT_VERSION="1.2.150"
PROJECT_NAME="OpenVisus"
PROJECT_URL="https://github.com/sci-visus/OpenVisus"
PROJECT_DESCRIPTION="ViSUS multiresolution I/O, analysis, and visualization system"
PROJECT_AUTHOR="visus.net"
PROJECT_EMAIL="support@visus.net"
PROJECT_LICENSE="BSD"
PROJECT_REQUIRES=["numpy","PyQt5==5.11.2"]

WIN32=platform.system()=="Windows" or platform.system()=="win32"
APPLE=platform.system()=="Darwin"
BDIST_WHEEL="bdist_wheel" in sys.argv	

# ////////////////////////////////////////////////////////////////////
if __name__ == "__main__":
	
	shutil.rmtree('./build', ignore_errors=True)
	shutil.rmtree('./__pycache__', ignore_errors=True)	
	shutil.rmtree('./.git', ignore_errors=True)	
	shutil.rmtree('./%s.egg-info' % (PROJECT_NAME,), ignore_errors=True)
	
	files=[]	
	for dirpath, __dirnames__, filenames in os.walk("."):
		for it in filenames:

			filename= os.path.abspath(os.path.join(dirpath, it))

			if filename.startswith(os.path.abspath('./dist')): 
				continue
			
			# for bdist wheel (i.e. *.whl file) I do not add developer files
			if BDIST_WHEEL and filename.startswith(os.path.abspath('./lib')):
				continue
			
			if BDIST_WHEEL and filename.startswith(os.path.abspath('./include')): 
				continue		    	

			if BDIST_WHEEL and WIN32 and filename.startswith(os.path.abspath('./win32/python')):
				continue

			if BDIST_WHEEL and WIN32 and filename.endswith(".pdb"): 
				continue
				
			if "__pycache__" in filename:
				continue	    				
						
			files.append(filename)
			
	setuptools.setup(
	  name = PROJECT_NAME,
	  description = PROJECT_DESCRIPTION,
	  version=PROJECT_VERSION,
	  url=PROJECT_URL,
	  author=PROJECT_AUTHOR,
	  author_email=PROJECT_EMAIL,
	  packages=[PROJECT_NAME],
	  package_dir={PROJECT_NAME:'.'},
	  package_data={PROJECT_NAME: files},
	  platforms=['Linux', 'OS-X', 'Windows'],
	  license = PROJECT_LICENSE,
	  install_requires=PROJECT_REQUIRES,
	)



