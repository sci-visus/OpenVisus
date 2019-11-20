
import sys
import cv2
from OpenVisus import *

if VISUS_GUI:
	from VisusGuiPy      import *
	from VisusGuiNodesPy import *
	from VisusAppKitPy   import *

def ASSERT(cond):
	if not cond: raise Exception("Assert failed")	
	
# ////////////////////////////////////////////////////////
def RunServer(port, content):
	config=ConfigFile()
	config.storage=StringTree.fromString(content)
	modvisus=ModVisus()
	modvisus.configureDatasets(config)
	srv=NetServer(port, modvisus)
	srv.runInBackground()	
	return srv
	

# ////////////////////////////////////////////////////////
if __name__ == '__main__':
	
	"""
	This file show how to: 
	(1) Create a local visus server
	(2) Open a viewer which connect to the server in order to show some idx files
	"""
	
	SetCommandLine("__main__")
	GuiModule.createApplication()
	
	if VISUS_GUI:
		AppKitModule.attach()
	else:
		IdxModule.attach()
		
	config_file="""
		<visus>
			<Configuration>
			  <ModVisus dynamic="False" />
			</Configuration>
			<datasets>
			  <dataset name='cat' url='file://./datasets/cat/rgb.idx' permissions='public' />
			</datasets>
		</visus>
	"""

	srv=RunServer(10000, config_file)

	# (2) view the idx in visus viewer...
	if VISUS_GUI:
		viewer=Viewer()
		viewer.open("http://localhost:%d/mod_visus?dataset=cat" % (srv.getPort()))	
		viewer.setMinimal()
		GuiModule.execApplication()
		
	srv=None
	
	if VISUS_GUI:
		AppKitModule.detach()
	else:
		IdxModule.detach()

	sys.exit(0)
	
	
