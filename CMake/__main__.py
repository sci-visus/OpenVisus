import os
import sys

if __name__ == '__main__':

	# configure
	if "configure" in sys.argv:
		if (sys.version_info > (3, 0)):
		  from .configure import *
		else:  
		  from configure import *	
		ConfigureStep()
		sys.exit(0)

	# dirname
	if "dirname" in sys.argv:
		print(os.path.abspath(os.path.dirname(__file__)))
		sys.exit(0)

	print("Error in arguments",sys.argv)
	sys.exit(-1)
