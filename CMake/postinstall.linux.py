
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
		output=subprocess.check_output(('readelf','-d',filename))
		if sys.version_info >= (3, 0): output=output.decode("utf-8")
		return output.strip()
		
	# debugLibDeps
	def debugLibDeps(self,filename):
		output=subprocess.check_output(('LD_DEBUG=libs','ldd',filename))
		if sys.version_info >= (3, 0): output=output.decode("utf-8")
		return output.strip()
		
	# executeCommand
	def executeCommand(self,cmd):	
		print(" ".join(cmd))
		subprocess.call(cmd)	
		
	# installSharedLib
	def installSharedLib(self,filename):
		filename=os.path.realpath(path) 
		if (os.path.isfile(filename)):
			self.executeCommand("cp",filename,".")
								
	# run
	def run(self):
		
		for name in ("QCore","Widgets","Gui","OpenGL"):
			filename=self.Qt5_DIR+...+name
			self.installSharedLib(filename)
				
		#if (NOT VISUS_INTERNAL_OPENSSL)
		#	self.installSharedLib(${OPENSSL_SSL_LIBRARY})
		#	self.installSharedLib(${OPENSSL_CRYPTO_LIBRARY})
		#endif()
		
		self.installSharedLib("/usr/lib64/libGLU.so")
		
	
		for filename in glob.glob("libVisus*.so") + glob.glob("_Visus*.so") + ["libmod_visus.so","visus","visusviewer"]		:
			self.executeCommand(["patchelf", "--set-rpath", "$ORIGIN", filename])
			
		# problem with symbolic link 
		for filename in glob.glob("*.so.*"):
			print("Fixing symbolic link of",filename)
			link,ext=os.path.splitext(filename)
			while not ext==".so":
				if(os.path.islink(link)):os.remove(link)
				os.symlink(filename, link)
				link,ext=os.path.splitext(link)			
		
if __name__ == "__main__":
	Qt5_DIR=sys.argv[1]
	print("Executing post install step","Qt5_DIR",Qt5_DIR)
	post_install_step=PostInstallStep(Qt5_DIR)
	post_install_step.run()