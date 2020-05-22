import os,sys

from OpenVisus import *

from OpenVisus.PyImage   import *
from OpenVisus.PyViewer  import *


# //////////////////////////////////////////////
def ExportSlicesToMovie(out_movie, db, axis,preserve_ratio=True):
		
	offsets=range(db.getLogicBox().p1[axis],db.getLogicBox().p2[axis],1):

	for offset in offsets:

		img,logic_box,physic_box=db.readSlice(axis, offset)
			
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
def ExportSideBySideMovies(old_dataset,new_db):
	
	ExportSlicesToMovie(PyMovie("All_z0.avi"), old_dataset, axis=2)
	ExportSlicesToMovie(PyMovie("All_z1.avi"), new_db, axis=2)

	ExportSlicesToMovie(PyMovie("All_x0.avi"), old_dataset, axis=0)
	ExportSlicesToMovie(PyMovie("All_x1.avi"), new_db, axis=0)

	ExportSlicesToMovie(PyMovie("All_y0.avi"), old_dataset, axis=1)
	ExportSlicesToMovie(PyMovie("All_y1.avi"), new_db, axis=1)

	PyMovie("Up_x0.avi").compose([(PyMovie("All_x0.avi"),0.0,1.0, 0.0,0.4), (PyMovie("All_x1.avi"),0.0,1.0, 0.0,0.4) ])	
	PyMovie("Dw_x0.avi").compose([(PyMovie("All_x0.avi"),0.0,1.0, 0.6,1.0), (PyMovie("All_x1.avi"),0.0,1.0, 0.6,1.0) ])	

	PyMovie("Up_y0.avi").compose([(PyMovie("All_y0.avi"),0.0,1.0, 0.0,0.4), (PyMovie("All_y1.avi"),0.0,1.0, 0.0,0.4) ])
	PyMovie("Dw_y0.avi").compose([(PyMovie("All_y0.avi"),0.0,1.0, 0.6,1.0), (PyMovie("All_y1.avi"),0.0,1.0, 0.6,1.0) ])	
	
# /////////////////////////////////////////////////////////////////
def VisibleMale():
	
	old_dataset=PyDataset(r"D:\GoogleSci\visus_dataset\male\visus.idx")
	new_db=PyDataset(r"D:\GoogleSci\visus_dataset\male\RAW\Fullcolor\fullbody\VisusSlamFiles\visus.idx")
	Assert(old_dataset)
	Assert(new_db)
	
	# ExportSideBySideMovies(old_dataset)
	
	flag_tatoo=(0.04, 0.95, 0.05,0.73, 0.15,0.15+0.1)	
	head=(0.35,0.65, 0.18,0.82, 0.0,0.2)	
	left_arm=(0.0, 0.4, 0.1,0.8, 0.0,0.3)
	full=(0.00,1.00, 0.00,1.00, 0.00,1.00)
	region=full
	
	db=new_db	

	logic_box=db.getLogicBox(x=region[0:2],y==region[2:4],z==region[4:6])
	RGB=db.read(logic_box,quality=-6)	
	bounds=db.getBounds(logic_box)

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
	
	db=PyDataset(r"D:\GoogleSci\visus_dataset\female\visus.idx")
	Assert(db)

	# ExportSideBySideMovies(old_dataset)
	
	full=(0,1, 0,1, 0,1.0)	
	head=(0,1, 0,1, 0,0.1)
	region=head

	logic_box=db.getLogicBox(x=region[0:2],y==region[2:4],z==region[4:6])

	RGB =db.read(logic_box=logic_box,quality=G-6)	
	bounds=db.getBounds(logic_box)
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
	
	#VisibleMale()
	
	# ConvertVisibleFemale
	VisibleFemale()
	
	print("ALL DONE")
	sys.stdin.read(1)
	sys.exit(0)	


# //////////////////////////////////////////////
if __name__ == '__main__':
	Main(sys.argv)




