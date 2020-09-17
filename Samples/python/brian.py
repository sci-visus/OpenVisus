import os
import sys

from OpenVisus     import *
from OpenVisus.gui import *

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
def ConvertToGrayScale(data):	
	
	# [R,G,B]
	if not isinstance(data,numpy.ndarray):
		R,G,B=data[0],data[1],data[2]
		return (0.299 * R + 0.587 * G + 0.114 * B).astype(R.dtype)
		
	# width*height*channel
	if len(data.shape)==3:
		R,G,B=data[:,:,0],data[:,:,1],data[:,:,2]
			
	# width*height*depth*channel
	elif len(data.shape)==4:
		# 3D data
		R,G,B=data[:,:,:,0],data[:,:,:,1] ,data[:,:,:,2]
	else:
		raise Exception("internal error")
	
	return ConvertToGrayScale([R,G,B])
	
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
	for I,offset in enumerate(offsets):

		logic_box=getSliceLogicBox(axis, offset)
		img=data.read(logic_box=logic_box)
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
	
	db=LoadDataset(r"D:\GoogleSci\visus_dataset\male\visus.idx")
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




