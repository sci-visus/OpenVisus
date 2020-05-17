import os,sys

from OpenVisus import *

from OpenVisus.PyUtils   import *
from OpenVisus.PyImage   import *
from OpenVisus.PyDataset import *
from OpenVisus.PyViewer  import *


# //////////////////////////////////////////////
def ExportSideBySideMovies(old_dataset,new_dataset):
	
	old_dataset.exportSlicesToMovie(PyMovie("All_z0.avi"), axis=2)
	new_dataset.exportSlicesToMovie(PyMovie("All_z1.avi"), axis=2)

	old_dataset.exportSlicesToMovie(PyMovie("All_x0.avi"), axis=0)
	new_dataset.exportSlicesToMovie(PyMovie("All_x1.avi"), axis=0)

	old_dataset.exportSlicesToMovie(PyMovie("All_y0.avi"), axis=1)
	new_dataset.exportSlicesToMovie(PyMovie("All_y1.avi"), axis=1)

	PyMovie("Up_x0.avi").compose([(PyMovie("All_x0.avi"),0.0,1.0, 0.0,0.4), (PyMovie("All_x1.avi"),0.0,1.0, 0.0,0.4) ])	
	PyMovie("Dw_x0.avi").compose([(PyMovie("All_x0.avi"),0.0,1.0, 0.6,1.0), (PyMovie("All_x1.avi"),0.0,1.0, 0.6,1.0) ])	

	PyMovie("Up_y0.avi").compose([(PyMovie("All_y0.avi"),0.0,1.0, 0.0,0.4), (PyMovie("All_y1.avi"),0.0,1.0, 0.0,0.4) ])
	PyMovie("Dw_y0.avi").compose([(PyMovie("All_y0.avi"),0.0,1.0, 0.6,1.0), (PyMovie("All_y1.avi"),0.0,1.0, 0.6,1.0) ])	
	
# /////////////////////////////////////////////////////////////////
def VisibleMale():
	
	old_dataset=LoadDatasetPy(r"D:\GoogleSci\visus_dataset\male\visus.idx")
	new_dataset=LoadDatasetPy(r"D:\GoogleSci\visus_dataset\male\RAW\Fullcolor\fullbody\VisusSlamFiles\visus.idx")
	Assert(old_dataset)
	Assert(new_dataset)
	
	# ExportSideBySideMovies(old_dataset)
	
	flag_tatoo=(0.04, 0.95, 0.05,0.73, 0.15,0.15+0.1)	
	head=(0.35,0.65, 0.18,0.82, 0.0,0.2)	
	left_arm=(0.0, 0.4, 0.1,0.8, 0.0,0.3)
	full=(0.00,1.00, 0.00,1.00, 0.00,1.00)
	region=full
	
	dataset=new_dataset	
	
	RGB, bounds=dataset.readData(region,-6)	
	R,G,B=SplitChannels(RGB)
	A=numpy.zeros(R.shape,dtype=R.dtype)
	
	# remove the blueish
	A[R>76]=255
	A[G-20>R]=0
	A[B-20>R]=0		
	
	# remove invalid region
	if region==full:
		d,h,w=A.shape
		A[int(0.0*d):int(1.00*d),int(0.00*h):int(0.03*h),int(0.0*w):int(1.0*w)]=0
		A[int(0.0*d):int(0.5*d),int(0.85*h):int(1.0*h),int(0.0*w):int(1.0*w)]=0
		A[int(0.5*d):int(1.0*d),int(0.7*h):int(1.0*h),int(0.0*w):int(1.0*w)]=0			
	
	R[A==0]=0
	G[A==0]=0
	B[A==0]=0
		
	RGBA=InterleaveChannels([R,G,B,A])		
		
	#for Z in range(RGBA.shape[0]):
	#	ShowImage(RGBA[Z,:,:,:]) 
	#	cv2.waitKey()		
	
	viewer=PyViewer()
	viewer.addGLCamera(viewer.getRoot(),"lookat")	
	
	#viewer.addIsoSurface(field=R, second_field=RGBA, isovalue=100.0, bounds=bounds)
	viewer.addVolumeRender(RGBA, bounds)	
	
	viewer.run()
	

# /////////////////////////////////////////////////////////////////
def VisibleFemale():
	
	dataset=LoadDatasetPy(r"D:\GoogleSci\visus_dataset\female\visus.idx")
	Assert(dataset)

	# ExportSideBySideMovies(old_dataset)
	
	full=(0,1, 0,1, 0,1.0)	
	head=(0,1, 0,1, 0,0.1)
	region=head

	RGB, bounds=dataset.readData(region,-6)	
	R,G,B=SplitChannels(RGB)
	A=numpy.zeros(R.shape,dtype=R.dtype)
	
	# remove the blueish
	A[R>76  ]=255
	A[G-20>R]=0
	A[B-20>R]=0		
	
	# remove invalid region
	if region==full:
		d,h,w=A.shape
		
		def removeRegion(x1,x2,y1,y2,z1,z2):
			A[int(z1*d):int(z2*d),int(y1*h):int(y2*h),int(x1*w):int(x2*w)]=0
				
		removeRegion(0.00,0.04,  0.00,1.00,  0.00,1.00)
		removeRegion(0.97,1.00,  0.00,1.00,  0.00,1.00)
		removeRegion(0.00,1.00,  0.00,0.05,  0.00,1.00)
		removeRegion(0.00,1.00,  0.75,1.00,  0.00,1.00)
		removeRegion(0.00,1.00,  0.68,1.00,  0.50,1.00)
	
	R[A==0]=0
	G[A==0]=0
	B[A==0]=0
		
	RGBA=InterleaveChannels([R,G,B,A])		
		
	#for Z in range(RGBA.shape[0]):
	#	ShowImage(RGBA[Z,:,:,:]) 
	#	cv2.waitKey()		
	
	viewer=PyViewer()
	viewer.addGLCamera("camera",viewer.getRoot(),"lookat")
	viewer.addIsoSurface(field=R, second_field=None, isovalue=100.0, bounds=bounds)
	#viewer.addVolumeRender(RGBA, bounds)	
	
	viewer.run()
	

# //////////////////////////////////////////////
def Main(argv):

	# set PYTHONPATH=D:/projects/OpenVisus/build/RelWithDebInfo
	# c:\Python37\python.exe Samples\python\ExtractSlices.py
	
	SetCommandLine("__main__")
	GuiModule.createApplication()
	GuiModule.attach()  	
	
	#VisibleMale()
	
	# ConvertVisibleFemale
	VisibleFemale()
	
	GuiModule.detach()
	print("ALL DONE")
	sys.stdin.read(1)
	sys.exit(0)	


# //////////////////////////////////////////////
if __name__ == '__main__':
	Main(sys.argv)




