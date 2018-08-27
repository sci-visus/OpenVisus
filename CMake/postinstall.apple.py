
import os
import sys
import subprocess
import glob


# ///////////////////////////////////////
class PostInstallStep:
	
	# extractDeps
	def extractDeps(self,filename):
		output=subprocess.check_output(('otool', '-L', filename))
		if sys.version_info >= (3, 0): output=output.decode("utf-8")
		output=output.strip()
		lines=output.split('\n')[1:]
		return [line.strip().split(' ', 1)[0] for line in lines]		
		
	# beginChange
	def beginChange(self,filename):
		print("")
		print("# Fixing",filename)	
		self.deps=self.extractDeps(filename)
		self.changes=[]
		
	# addChange
	def addChange(self,Old,New):
		self.changes+=[[Old,New]]	
		
	# endChange
	def endChange(self,filename):
		
		install_path=os.path.abspath(".")
		
		command=["install_name_tool"] 
		for Old,New in self.changes:
			filename_path=os.path.abspath(filename+"/..")
			num_up_level=len(filename_path.split("/"))- len(install_path.split("/"))
			if num_up_level<0: raise Exception("internal error")				
			
			New=New.replace("${FrameworksDir}","${ThisDir}/visusviewer.app/Contents/Frameworks")
			
			New=New.replace("${ThisDir}","@loader_path" + ("/.." * num_up_level))
			
			command+=["-change",Old,New]
			
		command+=[filename]
		
		print(" ".join(command))
		subprocess.call(command)		
				 	
	# run
	def run(self):
			             
		dy_libs=glob.glob("libVisus*.dylib")
		so_libs=glob.glob("_Visus*.so")
		executables= ["libmod_visus.dylib","visus.app/Contents/MacOS/visus","visusviewer.app/Contents/MacOS/visusviewer"]		
		             
		for filename in dy_libs + so_libs + executables:
			self.beginChange(filename)

			# fix visus dylib
			for Old in filter(lambda x: "libVisus" in x , self.deps):
				self.addChange(Old ,"${ThisDir}/" + os.path.split(Old)[1])

			# python library
			for Old in filter(lambda x: "libpython" in x , self.deps):
				self.addChange(Old,"${FrameworksDir}/" + os.path.split(Old)[1])

			# qt frameworks
			for Old in filter(lambda x: ".framework/Versions/5/Qt" in x , self.deps):
				self.addChange(Old,"${FrameworksDir}/" + "/".join(Old.split("/")[-4:]))

			self.endChange(filename)
			
		# fix qt frameworks
		for it in glob.glob('visusviewer.app/Contents/Frameworks/*.framework'):
			filename=it+'/Versions/5/' + it.split('/')[-1].split('.')[0]
			self.beginChange(filename)
			for Old in filter(lambda x: ".framework/Versions/5/Qt" in x , self.deps):
				self.addChange(Old,"${FrameworksDir}/" + "/".join(Old.split("/")[-4:]))
			self.endChange(filename)

if __name__ == "__main__":
	post_install_step=PostInstallStep()
	post_install_step.run()