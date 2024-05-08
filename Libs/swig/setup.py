
import os,sys,shutil,setuptools

try:
	import distutils.command.bdist_conda
except:
	pass

# the following line can be replaced automatically by `NewTag` so be careful not to change its format
PROJECT_VERSION="2.2.133"

# I can override by env variable
if "PROJECT_VERSION" in os.environ:
	PROJECT_VERSION=os.environ("PROJECT_VERSION")

this_dir=os.path.dirname(os.path.abspath(__file__))

# ///////////////////////////////////////////////////////////////
def NewTag():
	old_version=PROJECT_VERSION
	new_version=old_version.split(".")
	new_version[2]=str(int(new_version[2])+1)
	new_version=".".join(new_version)
	with open(__file__, 'r') as file : content = file.read()
	content = content.replace(f'PROJECT_VERSION="{old_version}"', f'PROJECT_VERSION="{new_version}"')
	with open(__file__, 'w') as file: file.write(content)		
	print(new_version)

# ///////////////////////////////////////////////////////////////
def DoSetup():
	# this are cached directories that should not be part of OpenVisus distribution
	shutil.rmtree('./build', ignore_errors=True)	
	shutil.rmtree('./dist', ignore_errors=True)	
	shutil.rmtree('./.git', ignore_errors=True)	
	shutil.rmtree('./tmp', ignore_errors=True)	
	shutil.rmtree('./{}.egg-info'.format("OpenVisus"), ignore_errors=True)	
	shutil.rmtree('./{}.egg-info'.format("OpenVisusNoGui"), ignore_errors=True)	
	files=[]	
	for dirpath, __dirnames__, filenames in os.walk("."):
		for it in filenames:
			filename= os.path.abspath(os.path.join(dirpath, it))
			if "__pycache__" in filename:  continue
			if os.path.splitext(filename)[1] in [".ilk",".pdb",".pyc",".pyo"]:  continue
			files.append(filename)
	
	
	# special name for no-gui
	# see https://github.com/opencv/opencv-python/blob/master/setup.py
	package_name="OpenVisus"
	
	# special case no gui
	if not os.path.isfile(os.path.join(this_dir,"QT_VERSION")):
		package_name="OpenVisusNoGui"	
	
	# dependency on numpy removed otherwise I have problems with conda which downgrade numpy for unknown reasons
	setuptools.setup(
	  name = package_name,
	  description = "ViSUS multiresolution I/O, analysis, and visualization system",
	  version=PROJECT_VERSION,
	  url="https://github.com/sci-visus/OpenVisus",
	  author="visus.net",
	  author_email="support@visus.net",
	  packages=["OpenVisus"],
	  package_dir={"OpenVisus":'.'},
	  package_data={"OpenVisus": files},
	  platforms=['Linux', 'OS-X', 'Windows'],
	  license = "BSD",
	  python_requires='>=3.6',
	  install_requires=[])
	
# ///////////////////////////////////////////////////////////////
if __name__ == "__main__":

  if len(sys.argv)>=2 and sys.argv[1]=="print-tag":
    print(PROJECT_VERSION)
    sys.exit(0)
	
  if len(sys.argv)>=2 and sys.argv[1]=="new-tag":
    NewTag()
    sys.exit(0)	

  DoSetup()
  sys.exit(0)	


