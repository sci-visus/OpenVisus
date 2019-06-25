
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
if __name__ == '__main__':
	
	"""
	This file show how to: 
	(1) load an image using opencv
	(2) convert the image to a visus idx file and 
	(3) finally view that file inside a visus viewer with some little python processing code
	"""
	
	SetCommandLine("__main__")
	GuiModule.createApplication()
	
	if VISUS_GUI:
		AppKitModule.attach()
	else:
		IdxModule.attach()
	
	img_filename="datasets/cat/rgb.png"
	idx_filename="temp/visus.idx"
	
	# (1) load an image using opencv
	img = cv2.imread(img_filename)
	img=cv2.cvtColor(img,cv2.COLOR_BGR2RGB)
	img=cv2.flip(img,0)	
	width,height,nchannels=img.shape[1],img.shape[0],img.shape[2] if len(img.shape)==3 else 1
	print("Loaded",img_filename,"width",width,"height",height,"nchannels",nchannels)	
	
	# (2) convert the image to idx file
	idxfile=IdxFile()
	idxfile.box=NdBox(NdPoint(0,0),NdPoint.one(width,height))
	idxfile.fields.push_back(Field("data",DType.fromString("uint8[{}]".format(nchannels))))
	bOk=idxfile.save(idx_filename)
	ASSERT(bOk)
		
	dataset=LoadDataset(idx_filename)
	ASSERT(dataset)
		
	array=Array.fromNumPy(img,TargetDim=2,bShareMem=True)
	access=dataset.createAccess()
	field=dataset.getDefaultField()
	time=dataset.getDefaultTime()
	bOk=dataset.writeFullResolutionData(access,field,time,array)
	ASSERT(bOk)
	
	# (3) view the idx in visus viewer...
	if VISUS_GUI:
	
		viewer=Viewer()
		viewer.openFile(idx_filename)	
		viewer.setMinimal()
		
		# ... with some little python scripting
		viewer.setScriptingCode("""
import cv2,numpy

pdim=input.dims.getPointDim()
img=Array.toNumPy(input,bShareMem=True)

img=cv2.Laplacian(img,cv2.CV_64F)

output=Array.fromNumPy(img,TargetDim=pdim)
	""".strip())
		
		GuiModule.execApplication()
		
	if VISUS_GUI:
		AppKitModule.detach()
	else:
		IdxModule.detach()

	
	
