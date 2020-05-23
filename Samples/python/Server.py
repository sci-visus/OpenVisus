
import sys
from OpenVisus import *


def ASSERT(cond):
	if not cond: raise Exception("Assert failed")	
	
# ////////////////////////////////////////////////////////
def RunOpenVisusServer(name,idx_filename,port=10000,dynamic=False):
	
	config=ConfigFile.fromString("""
<visus>
	<Configuration>
		<ModVisus dynamic="{}" />
	</Configuration>
	<datasets>
		<dataset name='{}' url='{}' permissions='public' />
	</datasets>
</visus>
""".format(dynamic,name,idx_filename).strip())

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
	
	srv1=RunOpenVisusServer("cat_rgb", "file://datasets/cat/rgb.idx" , port=10000, dynamic=False)
	srv2=RunOpenVisusServer("cat_gray","file://datasets/cat/gray.idx", port=10001, dynamic=False)
	
	# (2) view the idx in visus viewer...
	"""
	viewer=Viewer()
	viewer.open("http://localhost:10000/mod_visus?dataset=cat_rgb")	
	viewer.setMinimal()
	GuiModule.execApplication()
	"""

	sys.exit(0)
	
	
