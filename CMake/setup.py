import os, sys, setuptools
import shutil
import platform
import glob

PROJECT_VERSION="1.2.134"
PROJECT_NAME="OpenVisus"
PROJECT_URL="https://github.com/sci-visus/OpenVisus"
PROJECT_DESCRIPTION="ViSUS multiresolution I/O, analysis, and visualization system"

WIN32=platform.system()=="Windows" or platform.system()=="win32"
APPLE=platform.system()=="Darwin"
BDIST_WHEEL="bdist_wheel" in sys.argv	

# ////////////////////////////////////////////////////////////////////
def findFilesInCurrentDirectory():
	
	ret=[]	
	
	for dirpath, __dirnames__, filenames in os.walk("."):
		
		for it in filenames:

			filename= os.path.abspath(os.path.join(dirpath, it))

			if filename.startswith(os.path.abspath('./dist')): 
				continue
			
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
						
			ret.append(filename)
			
	return ret
	


# ////////////////////////////////////////////////////////////////////
if __name__ == "__main__":
	
	shutil.rmtree('./build', ignore_errors=True)
	shutil.rmtree('./__pycache__', ignore_errors=True)	
	shutil.rmtree('./.git', ignore_errors=True)	
	shutil.rmtree('./%s.egg-info' % (PROJECT_NAME,), ignore_errors=True)
	
	setuptools.setup(
	  name = PROJECT_NAME,
	  description = PROJECT_DESCRIPTION,
	  version=PROJECT_VERSION,
	  url=PROJECT_URL,
	  author="visus.net",
	  author_email="support@visus.net",
	  packages=[PROJECT_NAME],
	  package_dir={PROJECT_NAME:'.'},
	  package_data={PROJECT_NAME: findFilesInCurrentDirectory()},
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




