
import os
import sys
import subprocess
import glob


# ///////////////////////////////////////
class PostInstallStep:
	
	# init
	def __init__(self):
		self.install_path=os.path.abspath(".")
		
	# beginChange
	def beginChange(self,filename):
				
		print("")
		print("# Fixing",filename)				
		
		# extract dependencies
		output=subprocess.check_output(('otool', '-L', filename))
		if sys.version_info >= (3, 0): output=output.decode("utf-8")
		output=output.strip()
		lines=output.split('\n')[1:]
		self.deps=[line.strip().split(' ', 1)[0] for line in lines]			
		
		self.changes=[]
		
	# addChange
	def addChange(self,Old,New):
		self.changes+=[[Old,New]]	
		
	# endChange
	def endChange(self,filename):
		
		command=["install_name_tool"] 
		for Old,New in self.changes:
			filename_path=os.path.abspath(filename+"/..")
			num_up_level=len(filename_path.split("/"))- len(self.install_path.split("/"))
			if num_up_level<0: raise Exception("internal error")				
			New=New.replace("${FrameworksDir}","${ThisDir}/visusviewer.app/Contents/Frameworks")
			New=New.replace("${ThisDir}","@loader_path" + ("/.." * num_up_level)) 
			command+=["-change",Old,New]
			
		command+=[filename]
		
		print(" ".join(command))
		subprocess.call(command)		
				 	
	# run
	def run(self):
			
		for filename in [
			"libVisusKernel.dylib","_VisusKernelPy.so",
			"libVisusDataflow.dylib","_VisusDataflowPy.so",
			"libVisusDb.dylib","_VisusDbPy.so",
			"libVisusIdx.dylib","_VisusIdxPy.so",
			"libVisusNodes.dylib","_VisusNodesPy.so",
			"libVisusGui.dylib" ,"_VisusGuiPy.so" ,
			"libVisusGuiNodes.dylib","_VisusGuiNodesPy.so",
			"libVisusAppKit.dylib","_VisusAppKitPy.so",
			"libmod_visus.dylib",
			"visus.app/Contents/MacOS/visus",
			"visusviewer.app/Contents/MacOS/visusviewer",								
		]:
			self.beginChange(filename)

			# fix visus dylib
			self.addChange("@rpath/libVisusKernel.dylib"  ,"${ThisDir}/libVisusKernel.dylib")
			self.addChange("@rpath/libVisusDataflow.dylib","${ThisDir}/libVisusDataflow.dylib")
			self.addChange("@rpath/libVisusDb.dylib"      ,"${ThisDir}/libVisusDb.dylib")
			self.addChange("@rpath/libVisusIdx.dylib"     ,"${ThisDir}/libVisusIdx.dylib")
			self.addChange("@rpath/libVisusNodes.dylib"   ,"${ThisDir}/libVisusNodes.dylib")
			self.addChange("@rpath/libVisusGui.dylib"     ,"${ThisDir}/libVisusGui.dylib")
			self.addChange("@rpath/libVisusGuiNodes.dylib","${ThisDir}/libVisusGuiNodes.dylib")
			self.addChange("@rpath/libVisusAppKit.dylib"  ,"${ThisDir}/libVisusAppKit.dylib")

			# python library
			for Old in filter(lambda x: "libpython" in x , self.deps):
				if "@" not in Old: 
					self.addChange(Old,"${FrameworksDir}/" + os.path.split(Old)[1])

			# qt frameworks
			for Old in filter(lambda x: ".framework/Versions/5/Qt" in x , self.deps):
				if "@" not in Old: 
					self.addChange(Old,"${FrameworksDir}//" + "/".join(Old.split("/")[-4:]))

			self.endChange(filename)			
		
		for filename in [
			"visusviewer.app/Contents/Frameworks/QtCore.framework/Versions/5/QtCore",
			"visusviewer.app/Contents/Frameworks/QtGui.framework/Versions/5/QtGui",
			"visusviewer.app/Contents/Frameworks/QtWidgets.framework/Versions/5/QtWidgets",
			"visusviewer.app/Contents/Frameworks/QtOpenGL.framework/Versions/5/QtOpenGL"
		]:
			self.beginChange(filename)
			self.addChange("@executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore"      ,"${FrameworksDir}/QtCore.framework/Versions/5/QtCore")
			self.addChange("@executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui"        ,"${FrameworksDir}/QtGui.framework/Versions/5/QtGui")
			self.addChange("@executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets","${FrameworksDir}/QtWidgets.framework/Versions/5/QtWidgets")
			self.addChange("@executable_path/../Frameworks/QtOpenGL.framework/Versions/5/QtOpenGL"  ,"${FrameworksDir}/QtOpenGL.framework/Versions/5/QtOpenGL")
			self.endChange(filename)


if __name__ == "__main__":
	post_install_step=PostInstallStep()
	post_install_step.run()