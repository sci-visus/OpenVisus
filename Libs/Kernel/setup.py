import os,sys,shutil,setuptools

PROJECT_NAME="OpenVisus"
PROJECT_VERSION="2.1.36"

this_dir=os.path.dirname(os.path.abspath(__file__))

# ///////////////////////////////////////////////////////////////
def NewTag():
	new_version=PROJECT_VERSION.split(".")
	new_version[2]=str(int(new_version[2])+1)
	new_version=".".join(new_version)
	with open(__file__, 'r') as file : content = file.read()
	content = content.replace('PROJECT_VERSION="{0}"'.format(PROJECT_VERSION), 'PROJECT_VERSION="{0}"'.format(new_version))
	with open(__file__, 'w') as file: file.write(content)		
	print(new_version)

# ///////////////////////////////////////////////////////////////
def DoSetup():

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
			
	install_requires=[]
	
	try:
		import conda.cli
		bIsConda=True
	except:
		bIsConda=False	

	# dependency on numpy
	install_requires+=["numpy"]

	if not bIsConda:

		#dependency from PyQt5
		"""
		python -m pip install johnnydep

		johnnydep PyQt5~=5.14.0	-> PyQt5-sip<13,>=12.7
		johnnydep PyQt5~=5.13.0	-> PyQt5_sip<13,>=4.19.19
		johnnydep PyQt5~=5.12.0 -> PyQt5_sip<13,>=4.19.14
		johnnydep PyQt5~=5.11.0 -> empty
		johnnydep PyQt5~=5.10.0 -> empty
		johnnydep PyQt5~=5.9.0  -> empty

		johnnydep PyQtWebEngine~=5.14.0 -> PyQt5>=5.14 
		johnnydep PyQtWebEngine~=5.13.0 -> PyQt5>=5.13
		johnnydep PyQtWebEngine~=5.12.0 -> PyQt5>=5.12
		johnnydep PyQtWebEngine~=5.11.0 -> does not exist
		johnnydep PyQtWebEngine~=5.10.0 -> does not exist
		johnnydep PyQtWebEngine~=5.19.0 -> does not exist

		johnnydep PyQt5-sip~=12.7.0 -> empty
		"""

		def ReadTextFile(filename):
			file = open(filename, "r") 
			ret=file.read().strip()
			file.close()
			return ret

		QT_VERSION=ReadTextFile("QT_VERSION")
		print("QT_VERSION",QT_VERSION)
		major,minor=QT_VERSION.split('.')[0:2]

		install_requires+=["PyQt5~={}.{}.0".format(major,minor)]

		if int(major)==5 and int(minor)>=12:
			install_requires+=["PyQtWebEngine~={}.{}.0".format(major,minor)]
			install_requires+=["PyQt5-sip<13,>=12.7"] 

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
	  python_requires='>=3.6',
	  install_requires=install_requires)
	
# ///////////////////////////////////////////////////////////////
if __name__ == "__main__":
	
	if len(sys.argv)>=2 and sys.argv[1]=="new-tag":
		NewTag()
		sys.exit(0)	

	DoSetup()
	sys.exit(0)	


