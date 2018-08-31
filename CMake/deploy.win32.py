
import os
import sys
import subprocess
import globs

# ///////////////////////////////////////
class DeployStep:
	
	# constructor
	def __init__(self,Qt5_DIR):
		self.Qt5_DIR=Qt5_DIR
			
	# executeCommand
	def executeCommand(self,cmd):	
		print(" ".join(cmd))
		subprocess.call(cmd)			
		
	# run
	def run(self):
		deployqt=self.Qt5_DIR+/../../../bin/windeployqt
		self.executeCommand[deployqt,"visusviewer.exe"])
		
# ///////////////////////////////////////
if __name__ == "__main__":
	Qt5_DIR=sys.argv[1]
	print("Executing deploy","Qt5_DIR",Qt5_DIR)
	DeployStep(Qt5_DIR).run()