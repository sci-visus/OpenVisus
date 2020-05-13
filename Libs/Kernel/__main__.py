import os,sys

# ////////////////////////////////////////////////////////////////////////////////////////////////
if __name__ == '__main__':
	
	# no arguments
	if len(sys.argv)==1:
		sys.exit(0)

	action=sys.argv[1]

	#  redirect to slam
	if action=="slam":
		import OpenVisus.Slam.Main
		OpenVisus.Slam.Main.Main()
		sys.exit(0)	
	
	# forward to deploy
	import OpenVisus.deploy
	OpenVisus.deploy.Main()




