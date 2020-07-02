import os,sys

from OpenVisus     import *
from OpenVisus.gui import *

# //////////////////////////////////////////////
def SwapRedBlue(img):
	Assert(len(img.shape)==3) #YXC
	ret=img.copy()
	ret[:,:,0],ret[:,:,2]=img[:,:,2],img[:,:,0]
	return ret
	
# //////////////////////////////////////////////
def SplitChannels(data):
	N=len(data.shape)
	
	# width*height
	if N==2: 
		channels = [data]
		
	# width*height*channel
	elif N==3: 
		channels = [data[  :,:,C] for C in range(data.shape[-1])]
			
	# width*height*depth*channel
	elif N==4: 
		channels = [data[:,:,:,C] for C in range(data.shape[-1])]
			
	else:
		raise Exception("internal error")	
		
	return channels
	
# //////////////////////////////////////////////
def InterleaveChannels(channels):
	
	first=channels[0]
		
	N=len(channels)		
	
	if N==1: 
		return first	
	
	ret=numpy.zeros(first.shape + (N,),dtype=first.dtype)
	
	# 2D arrays
	if len(first.shape)==2:
		for C in range(N):
			ret[:,:,C]=channels[C]
				
	# 3d arrays
	elif len(first.shape)==3:
		for C in range(N):
			ret[:,:,:,C]=channels[C]
		
	else:
		raise Exception("internal error")
	

	return ret 
	
# //////////////////////////////////////////////
def ShowImage(img,max_preview_size=1024, win_name="img", wait_msec=1):
	
	if max_preview_size:	
	
		w,h=img.shape[1],img.shape[0]
		r=h/float(w)
		if w>h:
			w=min([w,max_preview_size])
			h=int(w*r)
		else:
			h=min([h,max_preview_size])
			w=int(h/r)
			
		img=cv2.resize(img,(w,h))
		
		# imshow does not support RGBA images
	if len(img.shape)>=3 and img.shape[2]==4:
		A=img[:,:,3].astype("float32")*(1.0/255.0)
		img=InterleaveChannels([
			numpy.multiply(img[:,:,0],A).astype(R.dtype),
			numpy.multiply(img[:,:,1],A).astype(G.dtype),
			numpy.multiply(img[:,:,2],A).astype(B.dtype)])
	
	cv2.imshow(win_name,img)
	cv2.waitKey(wait_msec) # wait 1 msec just to allow the image to appear
	

# //////////////////////////////////////////////
class PyMovie:

	# constructor
	def __init__(self,filename):
		self.filename=filename
		self.out=None
		self.input=None

	# release
	def release(self):
		if self.out:
			self.out.release()

	# writeFrame
	def writeFrame(self,img):
		if self.out is None:
			self.width,self.height=img.shape[1],img.shape[0]
			self.out = cv2.VideoWriter(self.filename,cv2.VideoWriter_fourcc(*'DIVX'), 15, (self.width,self.height))
		ShowImage(img)
		self.out.write(img)

	# readFrame
	def readFrame(self):
		if self.input is None:
			self.input=cv2.VideoCapture(self.filename)
		success,img=self.input.read()
		if not success: return None
		self.width,self.height=img.shape[1],img.shape[0]
		return img
		
	# compose
	def compose(self, args, axis=1):

		while True:

			images=[]
			
			for in_movie,x1,x2,y1,y2 in args:
				img=in_movie.readFrame()
				
				if img is None: 
					continue 
				
				# select a portion of the video	
				w,h=img.shape[1],img.shape[0]
				images.append(img[
					int(float(y1)*h):int(float(y2)*h),
					int(float(x1)*w):int(float(x2)*w),
					:] )
				
			if not images:
				out_movie.release()
				return

			out_movie.writeFrame(numpy.concatenate(images, axis=axis))

	
# //////////////////////////////////////////////
def ExportSlicesToMovie(out_movie, db, axis,preserve_ratio=True):
		
	offsets=range(db.getLogicBox().p1[axis],db.getLogicBox().p2[axis],1)

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
	
	old_dataset=LoadDataset(r"D:\GoogleSci\visus_dataset\male\visus.idx")
	new_db=LoadDataset(r"D:\GoogleSci\visus_dataset\male\RAW\Fullcolor\fullbody\VisusSlamFiles\visus.idx")
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
	
	db=LoadDataset(r"D:\GoogleSci\visus_dataset\female\visus.idx")
	Assert(db)

	# ExportSideBySideMovies(old_dataset)
	
	full=(0,1, 0,1, 0,1.0)	
	head=(0,1, 0,1, 0,0.1)
	region=head

	logic_box=db.getLogicBox(x=region[0:2],y=region[2:4],z=region[4:6])

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




