
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
def runSingleDatasetServer(name,idx_filename,port=10000,dynamic=False):
	
	config=ConfigFile()
	bOk=config.fromXmlString("""
<visus>
	<Configuration>
		<ModVisus dynamic="{}" />
	</Configuration>
	<datasets>
		<dataset name='{}' url='{}' permissions='public' />
	</datasets>
</visus>
""".format(dynamic,name,idx_filename).strip())
	ASSERT(bOk)

	modvisus=ModVisus()
	modvisus.configureDatasets(config)
	netserver=NetServer(port, modvisus)
	netserver.runInBackground()	
	return netserver
	
	

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
	
	srv1=runSingleDatasetServer("cat1","file://datasets/cat/visus.idx",port=10000,dynamic=False)
	srv2=runSingleDatasetServer("cat2","file://datasets/cat/visus.idx",port=10001,dynamic=False)
	
	# (2) view the idx in visus viewer...
	if VISUS_GUI:
		viewer=Viewer()
		viewer.openFile("http://localhost:10000/mod_visus?dataset=cat1")	
		viewer.setMinimal()

		GuiModule.execApplication()
		
	srv1=None
	srv2=None
	
	if VISUS_GUI:
		AppKitModule.detach()
	else:
		IdxModule.detach()

	
	
