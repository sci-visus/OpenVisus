
import os
import sys
import subprocess
import glob
import shutil
import platform
import errno
import fnmatch
import os
import sysconfig

WIN32=platform.system()=="Windows" or platform.system()=="win32"
APPLE=platform.system()=="Darwin"

bVerbose=False

# /////////////////////////////////////////////////
def WriteTextFile(filename,lines):
	dirname=os.path.dirname(os.path.realpath(filename))
	try: 
		os.makedirs(dirname)
	except OSError:
		pass
	file = open(filename,"wt") 
	file.write("\n".join(content)+"\n") 
	file.close() 		



# ////////////////////////////////////////////////////////////////////
def CreateScript(exe):

	name=os.path.splitext(os.path.basename(exe))[0]
	script_ext=".bat" if WIN32 else (".command" if APPLE else ".sh")
	script_filename="%s%s" % (name,script_ext)
	inner_exe='%s/Contents/MacOS/%s' % (exe,name) if APPLE else exe
	
	PYTHON_EXECUTABLE=sys.executable
	PYTHONPATH=":".join(sys.path)
	LD_LIBRARY_PATH=os.path.realpath(sysconfig.get_config_var("LIBDIR"))
	
	if WIN32:
		
		try:
			import PyQt5
			Qt5_DIR=os.path.join(os.path.dirname(PyQt5.__file__),"Qt")
		except Exception as err:
			print("ERROR import PyQt5 failed, GUI stuff is not going to work",err)
			Qt5_DIR=r"C:\Qt"
			
		WriteTextFile(script_filename,[
			r'cd /d %~dp0',
			'set PYTHON_EXECUTABLE=%s' %(PYTHON_EXECUTABLE,),
			'set PYTHONPATH=$(pwd):%s' % (PYTHONPATH,),
			'set Qt5_DIR=%s' % (Qt5_DIR,),
			'set QT_PLUGIN_PATH=%s' % (os.path.join(Qt5_DIR,"plugins")	,),
			'set PATH=%s;%s;%s;%s' % (os.path.join(Qt5_DIR,"bin"),os.path.dirname(PYTHON_EXECUTABLE),r"%cd%\bin",r"%PATH%"),
			inner_exe + r' %*'
		])

	else:
		
		try:
			import PyQt5
			Qt5_DIR=os.path.join(os.path.dirname(PyQt5.__file__),"Qt")
		except Exception as err:
			print("ERROR import PyQt5 failed, GUI stuff is not going to work",err)
			Qt5_DIR=r"\Qt\not\found"
								
		WriteTextFile(script_filename,[
			'#!/bin/bash',
			'this_dir=$(cd "$(dirname "${BASH_SOURCE[0]}" )" && pwd)',
			'cd ${this_dir}',
			'export PYTHON_EXECUTABLE=%s' %(PYTHON_EXECUTABLE,),
			'export PYTHONPATH=$(pwd):%s' % (PYTHONPATH,),
			'export Qt5_DIR=%s' % (Qt5_DIR,),
			'export QT_PLUGIN_PATH=%s' % (os.path.join(Qt5_DIR,"plugins")	,),
			'export %s=%s' % ("DYLD_LIBRARY_PATH" if APPLE else "LD_LIBRARY_PATH",LD_LIBRARY_PATH,),
			inner_exe + r' "$@"'
		])	
			
		#WriteTextFile("%s/Contents/Resources/qt.conf" % (app_path,),[
		#	"[Paths]",
		#	"  Plugins=%s" % (os.path.join(Qt5_DIR,"plugins"),)])				
	
	if not WIN32:
		subprocess.call(["chmod","+rx",script_filename], shell=False)
			
	
		
# ////////////////////////////////////////////////////////////////////
def main():
	
	print("Executing configure....")
	
	old_dir = os.getcwd()
	os.chdir(os.path.dirname(os.path.realpath(__file__)))
	
	import pip
	if int(pip.__version__.split('.')[0])>9:
		from pip._internal import main as pip_main
	else:
		from pip import main as pip_main
		
	try:
		pip_main(["install","--user", "-r","requirements.txt"])
	except Exception as err:
		print("ERROR some requirements failed to install",err)
	
	if WIN32:
		# seems to work without any problem, see VisusGuiPy.i
		pass
	
	elif APPLE:
		
		try:
			import PyQt5
			Qt5_DIR=os.path.join(os.path.dirname(PyQt5.__file__),"Qt")
			AppleDeployStep().addRPath(os.path.join(Qt5_DIR,"lib"))	
		except Exception as err:
			print("ERROR import PyQt5 failed, GUI stuff is not going to work",err)
		
	else:
		# nothing todo? how OpenVisus can link PyQt?
		pass
		
	# create scripts
	
	if WIN32:
		executables=glob.glob('bin/*.exe')
	elif APPLE:
		executables=glob.glob('bin/*.app')
	else:
		executables= [it for it in glob.glob('bin/*') if os.path.isfile(it) and not os.path.splitext(it)[1] ]	
	
	for exe in executables:
		self.createScript(exe)
			
	os.chdir(old_dir)	

	print("finished configure.py")	
		

# //////////////////////////////////////////////////////////////////////////////
if __name__ == "__main__":
	print(sys.argv)
	main()
	sys.exit(0)
