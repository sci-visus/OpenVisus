import os
import sys

from OpenVisus           import *
from OpenVisus.PyUtils   import *
from OpenVisus.PyImage   import *
from OpenVisus.PyDataset import *
from OpenVisus.PyViewer  import *

# Scripting Node to remove the white-sh (TO TRY)
"""

import numpy 
from OpenVisus.PyUtils import *

input=Array.toNumPy(input)
R,G,B=SplitChannels(input)
A=ConvertToGrayScale([R,G,B])

# remove white-ish
A[A>245]=0 

output=InterleaveChannels([R,G,B,A])
output=Array.fromNumPy(output,TargetDim=3)

"""

# //////////////////////////////////////////////
def Main(argv):

	# set PYTHONPATH=D:/projects/OpenVisus/build/RelWithDebInfo
	# c:\Python37\python.exe Samples\python\Brian.py
	
	SetCommandLine("__main__")
	GuiModule.createApplication()
	GuiModule.attach()  	
	
	dataset=LoadDatasetPy(r"D:\GoogleSci\visus_dataset\male\visus.idx")
	Assert(dataset)

	# dataset.exportSlicesToMovie(PyMovie("All_z.avi"), axis=2)
	# dataset.exportSlicesToMovie(PyMovie("All_x.avi"), axis=0)
	# dataset.exportSlicesToMovie(PyMovie("All_y.avi"), axis=1)

	# specify region in [0,1] range (X,Y,Z)
	region=(0.00,1.00, 0.00,1.00, 0.00,1.00)
	
	RGB, bounds = dataset.readData(region,resolution=-6)	
	R,G,B=SplitChannels(RGB)
	
	A=numpy.zeros(R.shape,dtype=R.dtype)
	
	# remove the white-ish
	A[R>76]=255
	
	R[A==0]=0
	G[A==0]=0
	B[A==0]=0
		
	RGBA=InterleaveChannels([R,G,B,A])		
		
	# show the slices
	if False:
		for Z in range(RGBA.shape[0]):
			ShowImage(RGBA[Z,:,:,:]) 
			cv2.waitKey()		
	
	viewer=PyViewer()
	viewer.addGLCamera("glcamera1",viewer.getRoot(),"lookat")	
	
	viewer.addIsoSurface(field=R, second_field=RGBA, isovalue=100.0, bounds=bounds)
	#viewer.addVolumeRender(RGBA, bounds)	
	
	viewer.run()
	
	GuiModule.detach()
	print("ALL DONE")
	sys.stdin.read(1)
	sys.exit(0)	


# //////////////////////////////////////////////
if __name__ == '__main__':
	Main(sys.argv)




