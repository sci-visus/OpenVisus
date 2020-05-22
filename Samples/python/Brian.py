import os
import sys

from OpenVisus           import *
from OpenVisus.PyImage   import *
from OpenVisus.PyViewer  import *

# Scripting Node to remove the white-sh (TO TRY)
"""

import numpy 

input=Array.toNumPy(input)
R,G,B=SplitChannels(input)
A=ConvertToGrayScale([R,G,B])

# remove white-ish
A[A>245]=0 

output=InterleaveChannels([R,G,B,A])
output=Array.fromNumPy(output,TargetDim=3)

"""

# //////////////////////////////////////////////
def ExportSlicesToMovie(out_movie, db, axis,preserve_ratio=True):
		
	offsets=range(db.getLogicBox().p1[axis],db.getLogicBox().p2[axis],1)
	for I,offset in enumerate(offsets):

		logic_box=getSliceLogicBox(axis, offset)
		img=next(data.read(logic_box=logic_box))
		physic_box=db.getBounds(logic_box).toAxisAlignedBox()

		img,logic_box,=db.readSlice()
			
		# resize to preserve the right ratio
		if preserve_ratio:
			density=[float(logic_box.size()[I])/float(physic_box.size()[I]) for I in range(3)]
			max_density=max(density)
			num_pixels=[int(physic_box.size()[I] * max_density) for I in range(3)]
			perm=((1,2,0),(0,2,1),(0,1,2))
			X,Y=perm[axis][0],perm[axis][1]				
			new_size=(num_pixels[X],num_pixels[Y])
			img=cv2.resize(img,new_size)

		out_movie.writeFrame(SwapRedBlue(img))
			
	out_movie.release()

# //////////////////////////////////////////////
def Main(argv):

	# set PYTHONPATH=D:/projects/OpenVisus/build/RelWithDebInfo
	# c:\Python37\python.exe Samples\python\Brian.py
	
	db=PyDataset(r"D:\GoogleSci\visus_dataset\male\visus.idx")
	Assert(db)

	# ExportSlicesToMovie(PyMovie("All_z.avi"), db, axis=2)
	# ExportSlicesToMovie(PyMovie("All_x.avi"), db, axis=0)
	# ExportSlicesToMovie(PyMovie("All_y.avi"), db, axis=1)

	# specify region in [0,1] 
	logic_box=db.getLogicBox(x=[0.0,1.0],y=[0.0,1.0],z=[0.0,1.0])
	bounds=db.getBounds(logic_box)
	RGB = db.read(logic_box = logic_box, quality=-6)	
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
	
	print("ALL DONE")
	sys.stdin.read(1)
	sys.exit(0)	


# //////////////////////////////////////////////
if __name__ == '__main__':
	Main(sys.argv)




