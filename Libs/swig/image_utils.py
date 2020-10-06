import os,sys
import numpy

# show image and PyMovie wont' work without cv2
try:
	import cv2
except:
	pass

# ///////////////////////////////////////////////////
def SplitChannels(array):
	return [array[...,C] for C in range(array.shape[-1])]

# ///////////////////////////////////////////////////
def InterleaveChannels(v):
	N=len(v)

	if N==0:
		raise Exception("empty image")

	if N==1: 
		return v[0]
	
	ret=numpy.zeros(v[0].shape + (N,), dtype=v[0].dtype)
	for C in range(N): 
		ret[...,C]=v[C]
	return ret 
	
# ///////////////////////////////////////////////////
def SwapRedBlue(array):
	v=SplitChannels(array)
	v[0],v[2]=v[2],v[0]
	return InterleaveChannels(v)	
	
# ///////////////////////////////////////////////////
def ShowImage(img, win_name="Show image", pos=None, max_preview_size=1024):
	
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
		
	# imshow does not support RGBA images so I'm applying the alpha
	if len(img.shape)>=3 and img.shape[2]==4:
		A=img[:,:,3].astype("float32")*(1.0/255.0)
		img=InterleaveChannels([
			numpy.multiply(img[:,:,0],A).astype(img.dtype),
			numpy.multiply(img[:,:,1],A).astype(img.dtype),
			numpy.multiply(img[:,:,2],A).astype(img.dtype)])
	
	# opencv wants BGR
	if len(img.shape)==3:
		img = cv2.cvtColor(img, cv2.COLOR_RGB2BGR)
		
	cv2.imshow(win_name,img)
	
	if pos is not None:
		cv2.moveWindow(win_name, pos[0],pos[1])
		

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
			
		img=SwapRedBlue(img)
		ShowImage(img)
		cv2.waitKey(1) # wait 1 msec just to allow the image to appear
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