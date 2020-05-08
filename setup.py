import os,sys,shutil,setuptools
	
PROJECT_NAME="OpenVisus"
PROJECT_VERSION="1.3.98"
if __name__ == "__main__":
	
	if len(sys.argv)>=2 and sys.argv[1]=="old-version":
		print(PROJECT_VERSION)
		sys.exit()
		
	if len(sys.argv)>=2 and sys.argv[1]=="new-version":
		v=PROJECT_VERSION.split(".")
		v[2]=str(int(v[2])+1)
		print(".".join(v))
		sys.exit()		
	
	# this are cached directories that should not be part of OpenVisus distribution
	shutil.rmtree('./build', ignore_errors=True)	
	shutil.rmtree('./dist', ignore_errors=True)	
	shutil.rmtree('./.git', ignore_errors=True)	
	shutil.rmtree('./{}.egg-info'.format(PROJECT_NAME), ignore_errors=True)	

	files=[]	
	for dirpath, __dirnames__, filenames in os.walk("."):
		for it in filenames:
			filename= os.path.abspath(os.path.join(dirpath, it))
			
			if "__pycache__" in filename: 
				continue
			
			if os.path.splitext(filename)[1] in [".ilk",".pdb",".pyc",".pyo"]: 
				continue
				
			files.append(filename)	

	setuptools.setup(
	  name = PROJECT_NAME,
	  description = "ViSUS multiresolution I/O, analysis, and visualization system",
	  version=PROJECT_VERSION,
	  url="https://github.com/sci-visus/OpenVisus",
	  author="visus.net",
	  author_email="support@visus.net",
	  packages=[PROJECT_NAME],
	  package_dir={PROJECT_NAME:'.'},
	  package_data={PROJECT_NAME: files},
	  platforms=['Linux', 'OS-X', 'Windows'],
	  license = "BSD",
	  install_requires=["numpy"])



