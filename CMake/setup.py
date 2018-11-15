import os, sys, setuptools
import shutil
import platform
import glob

#increase this number for PIP
VERSION="1.2.64"

# ////////////////////////////////////////////////////////////////////
def getArgValue(name) :
	for I,arg in enumerate(sys.argv):
		if arg.startswith(name+"="):
			return arg.split("=")[1]
	return None

WIN32=platform.system()=="Windows" or platform.system()=="win32"
APPLE=platform.system()=="Darwin"
BDIST_WHEEL="bdist_wheel" in sys.argv
PYTHON_TAG=getArgValue("--python-tag")
PLATFORM_NAME=getArgValue("--plat-name")
	
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
				
			if "__pycache__" in filename:
				continue	    	

			# for bdist_wheel I don't need to add files for compilation
			if BDIST_WHEEL: 
				
				if filename.startswith(os.path.abspath('./lib')):
					continue
				
				if filename.startswith(os.path.abspath('./include')): 
					continue		    	

				if WIN32:
					
					if filename.startswith(os.path.abspath('./win32/python')):
						continue
						
					if filename.endswith(".pdb"): 
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
if __name__ == "__main__":
	
  cleanAll()
  runSetupTools()
  
  # for pip/linux I need to have a specific name
  if True:
    wheel_filename = glob.glob('dist/*.whl')
    if PLATFORM_NAME=="linux_x86_64" and len(wheel_filename)==1:
      os.rename(wheel_filename[0], wheel_filename[0].replace("linux_x86_64","manylinux1_x86_64"))  

  # the sdist filename should be the same as the wheel
  if True:
    sdist_ext='.zip' if WIN32 else ".tar.gz"
    wheel_filename = glob.glob('dist/*.whl')
    sdist_filename = glob.glob('dist/*' +sdist_ext)
    if len(wheel_filename)==1 and len(sdist_filename)==1:
      os.rename(sdist_filename[0], os.path.splitext(wheel_filename[0])[0] + sdist_ext)	






