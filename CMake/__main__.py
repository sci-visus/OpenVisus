import os
import sys

if __name__ == '__main__':
	if sys.argv[-1]=="configure":
		from BundleUtils import *
		PipPostInstall().run()	
    sys.exit(0)
