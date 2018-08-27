
import os
import sys
import subprocess
import glob


# ///////////////////////////////////////
class PostInstallStep:
	
	# extractDeps
	def extractDeps(self,filename):
		output=subprocess.check_output(('readelf','-d',filename))
		if sys.version_info >= (3, 0): output=output.decode("utf-8")
		return output.strip()
		
	# debugLibDeps
	def debugLibDeps(self,filename):
		output=subprocess.check_output(('LD_DEBUG=libs','ldd',filename))
		if sys.version_info >= (3, 0): output=output.decode("utf-8")
		return output.strip()
		
	# fixSymLinks
	def fixSymLinks(self,filename):
		print("Fixing symbolic link of",filename)
		link,ext=os.path.splitext(filename)
		while not ext==".so":
			os.symlink(filename, link)
			link,ext=os.path.splitext(link)
			
	# run
	def run(self):
	
		so_libs=glob.glob("libVisus*.so") + glob.glob("_Visus*.so")
		executables= ["libmod_visus.so","visus","visusviewer"]		
		             
		for filename in so_libs + executables:
			command=["patchelf", "--set-rpath", "$ORIGIN:$ORIGIN/Frameworks", filename]
			print(" ".join(command))
			subprocess.call(command)	
			
		os.chdir("Frameworks")
		
		# problem with symbolic link in Frameworks
		for filename in glob.glob("*.so.*"):
			self.fixSymLinks(filename)
		
		for filename in glob.glob('*.so'):
			command=["patchelf","--set-rpath","$ORIGIN",filename]
			print(" ".join(command))
			subprocess.call(command)	
		os.chdir("../")

if __name__ == "__main__":
	post_install=PostInstallStep()
	post_install.run()