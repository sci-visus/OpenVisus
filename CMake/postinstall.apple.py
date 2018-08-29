
import os
import sys
import subprocess
import glob


# ///////////////////////////////////////
class PostInstallStep:
	
	# constructor
	def __init__(self,Qt5_DIR):
		self.Qt5_DIR=Qt5_DIR
	
	# extractDeps
	def extractDeps(self,filename):
		output=subprocess.check_output(('otool', '-L', filename))
		if sys.version_info >= (3, 0): output=output.decode("utf-8")
		output=output.strip()
		lines=output.split('\n')[1:]
		return [line.strip().split(' ', 1)[0] for line in lines]		
		
	# executeCommand
	def executeCommand(self,cmd):	
		print(" ".join(cmd))
		subprocess.call(cmd)			
		
	# change
	def change(self,Old,New,filename):
		install_path=os.path.abspath(".")
		filename_path=os.path.abspath(filename+"/..")
		num_up_level=len(filename_path.split("/"))- len(install_path.split("/"))
		if num_up_level<0: raise Exception("internal error")				
		New=New.replace("${ThisDir}","@loader_path" + ("/.." * num_up_level))
		self.executeCommand(["install_name_tool","-change",Old,New,filename])	
			 	
	# run
	def run(self):
			 
		# not portable right now...    
		return 
		
		# todo
		"""
		
		# copy Qt frameworks 
		qt_frameworks=("QtCore","QtGui","QtWidgets","QtOpenGL")
		for name in [ for it in qt_frameworks]:
			dir=self.Qt5_DIR+"/../../"+name
			filename=dir+"Versions/5/"+name
			self.executeCommand(["cp","-R",dir,"."])
			
			
		# fix qt references
		
		# fix visus libs references
		
		# fix python references
		
			
			
			
			self.change()
			
			# fix qt inter-dependencies
			deps=self.extractDeps(filename)
			for Old in filter(lambda x: ".framework/Versions/5/Qt" in x , deps):
				self.change(Old,"@loader_path/../../../" + "/".join(Old.split("/")[-4:]),filename)
				
		
		
		deploy_qt=self.Qt5_DIR+ "/../../../bin/macdeployqt"
		self.executeCommand([deploy_qt,"visusviewer.app"])
			             
		visus_libs=glob.glob("libVisus*.dylib") + glob.glob("_Visus*.so")
		visus_executables= ["libmod_visus.dylib","visus.app/Contents/MacOS/visus","visusviewer.app/Contents/MacOS/visusviewer"]		
		             
		for filename in visus_libs + visus_executables:
			
			deps=self.extractDeps(filename)

			# fix visus library
			for Old in filter(lambda x: "libVisus" in x , deps):
				self.change(Old ,"${ThisDir}/" + os.path.split(Old)[1],filename)

			# python library
			for Old in filter(lambda x: "libpython" in x , deps):
				self.change(Old,"${ThisDir}/" + os.path.split(Old)[1],filename)

			# qt frameworks
			for Old in filter(lambda x: ".framework/Versions/5/Qt" in x , deps):
				self.change(Old,"${ThisDir}/" + "/".join(Old.split("/")[-4:]),filename)

		# 
		for it in glob.glob('visusviewer.app/Contents/Frameworks/Qt*.framework'):
			filename=it+'/Versions/5/' + it.split('/')[-1].split('.')[0]
			for Old in filter(lambda x: ".framework/Versions/5/Qt" in x , deps):
				
	"""


if __name__ == "__main__":
	Qt5_DIR=sys.argv[1]
	print("Executing post install step","Qt5_DIR",Qt5_DIR)
	post_install_step=PostInstallStep(Qt5_DIR)
	post_install_step.run()