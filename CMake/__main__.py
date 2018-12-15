import os
import sys

if __name__ == '__main__':
	if sys.argv[-1]=="configure":
		if (sys.version_info > (3, 0)):
		  from .configure import *
		else:  
		  from configure import *	
		Configure()	
	sys.exit(0)
