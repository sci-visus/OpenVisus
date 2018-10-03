import os, sys, setuptools
import shutil
import platform

#increase this number for PIP
VERSION="1.2.41"

WIN32=platform.system()=="Windows" or platform.system()=="win32"
APPLE=platform.system()=="Darwin"
SDIST="sdist" in sys.argv

# ////////////////////////////////////////////////////////////////////
def cleanAll():
	shutil.rmtree('./build', ignore_errors=True)
	shutil.rmtree('./OpenVisus.egg-info', ignore_errors=True)
	shutil.rmtree('./__pycache__', ignore_errors=True)
	
	
# ////////////////////////////////////////////////////////////////////
def findFilesInCurrentDirectory():
	
	ret=[]
	
	for dirpath, __dirnames__, filenames in os.walk("."):
		for it in filenames:

			filename= os.path.abspath(os.path.join(dirpath, it))

			if filename.startswith(os.path.abspath('./dist')): 
				continue
				
			if os.path.basename(filename).startswith(".git"): 
				continue	    	

			if not SDIST:

				if filename.startswith(os.path.abspath('./lib')): 
					continue
					
				if filename.startswith(os.path.abspath('./include')): 
					continue		    	

				if WIN32:
					if filename.startswith(os.path.abspath('./win32/python')): 
						continue
						
					if filename.endswith(".pdb"): 
						continue
						
					if filename.startswith(os.path.abspath('./bin')) and os.path.basename(filename).startswith("Qt5") and os.path.basename(filename).endswith(".dll"): 
						continue
						
					if filename.startswith(os.path.abspath('./bin/plugins')): 
						continue		    	

			ret.append(filename)
			
	return ret
	

# ////////////////////////////////////////////////////////////////////
def runSetupTools():
	setuptools.setup(
	  name = "OpenVisus",
	  description = "ViSUS multiresolution I/O, analysis, and visualization system",
	  version=VERSION,
	  url="https://github.com/sci-visus/OpenVisus",
	  author="visus.net",
	  author_email="support@visus.net",
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
	
# ////////////////////////////////////////////////////////////////////
def autoRenameSDist():

	import glob

	for ext in (".tar.gz", ".zip"):
		
		sdist_filename=glob.glob('dist/*'+ext); 
		wheel_filename =glob.glob('dist/*.whl')	
		
		if len(sdist_filename)!=1: 
			continue
			
		if len(wheel_filename )!=1: 
			continue
		
		sdist_filename=sdist_filename[0]
		wheel_filename=wheel_filename[0]
	
		if not os.path.isfile(sdist_filename): 
			continue
			
		os.rename(sdist_filename, os.path.splitext(wheel_filename)[0]+ext)	
	
# ////////////////////////////////////////////////////////////////////
if __name__ == "__main__":
	
	cleanAll()
	runSetupTools()
	autoRenameSDist()






