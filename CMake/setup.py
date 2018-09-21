import os, sys, setuptools
import shutil

#increase this number for PIP
VERSION="1.2.33"

# ////////////////////////////////////////////////////////////////////
def cleanAll():
	shutil.rmtree('./build', ignore_errors=True)
	shutil.rmtree('./dist', ignore_errors=True)
	shutil.rmtree('./OpenVisus.egg-info', ignore_errors=True)
	shutil.rmtree('./__pycache__', ignore_errors=True)
	
# ////////////////////////////////////////////////////////////////////
def findFilesInCurrentDirectory():
	ret=[]
	for dirpath, __dirnames__, filenames in os.walk("."):
	  for filename in filenames:
	    file= os.path.abspath(os.path.join(dirpath, filename))
	    ret.append(file)
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






