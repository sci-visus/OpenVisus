import os,sys,glob,threading,platform,sysconfig,re,time,subprocess, errno, fnmatch, shutil

import numpy,cv2

# ////////////////////////////////////////////////////////////////////////////////
def FindImages(template="./**/*.*",recursive=True,image_extensions=('.jpg','.png','.tif','.bmp')):
	
	ret=[]
	for filename in glob.glob(template,recursive=recursive):
		
		# look for extension, must be an image
		if image_extensions:
			ext=	os.path.splitext(filename)[1].lower()
			if not ext in image_extensions:
				continue
			
		ret.append(filename)
		
	return ret

# ////////////////////////////////////////////////////////////////////////////////
def MatrixToNumPy(value):
	ret=numpy.eye(3, 3, dtype=numpy.float32)
	for R in range(3):
		for C in range(3):
			ret[R,C]=value.getRow(R)[C]
	return ret

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
		
# ////////////////////////////////////////////////////////////////////////////////
def ConvertImageToGrayScale(img):
	if len(img.shape)>=3 and img.shape[2]==3:
		return cv2.cvtColor(img[:,:,0:3], cv2.COLOR_RGB2GRAY)
	else:
		return img[:,:,0] 

# ////////////////////////////////////////////////////////////////////////////////
def ResizeImage(src,max_size):
	H,W=src.shape[0:2]
	vs=max_size/float(max([W,H]))
	if vs>=1.0: return src
	return cv2.resize(src, (int(vs*W),int(vs*H)), interpolation=cv2.INTER_CUBIC)


# //////////////////////////////////////////////
def AddAlphaChannel(data):
	Assert(isinstance(data,numpy.ndarray))
	dtype=data.dtype
	channels = [data[:,:,:,C] for C in range(data.shape[-1])]
	alpha=numpy.zeros(channels[0].shape,dtype=dtype)
	return channels + [alpha,]

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


		
# ////////////////////////////////////////////////////////////////////////////////
def ComputeImageRange(src):
	if len(src.shape)<3:
		return (numpy.amin(src)),float(numpy.amax(src))
	return [ComputeImageRange(src[:,:,C]) for C in range(src.shape[2])]

# ////////////////////////////////////////////////////////////////////////////////
def NormalizeImage32f(src):
		
	if len(src.shape)<3:
		dst = numpy.zeros(src.shape, dtype=numpy.float32)
		m,M=ComputeImageRange(src)
		delta=(M-m)
		if delta==0.0: delta=1.0
		return (src.astype('float32')-m)*(1.0/delta)
			
	dst = numpy.zeros(src.shape, dtype=numpy.float32)
	for C in range(src.shape[2]):
		dst[:,:,C]=NormalizeImage32f(src[:,:,C])
	return dst
		
# ////////////////////////////////////////////////////////////////////////////////
def ComputeGradientImage(src):
	src = NormalizeImage32f(src)
	return cv2.addWeighted(
		numpy.absolute(cv2.Sobel(src,cv2.CV_32F,1,0,ksize=3)), 0.5, 
		numpy.absolute(cv2.Sobel(src,cv2.CV_32F,0,1,ksize=3)), 0.5, 
		0)				
					
# ////////////////////////////////////////////////////////////////////////////////
def ConvertImageToUint8(img):
	if img.dtype==numpy.uint8: return img
	return (NormalizeImage32f(img) * 255).astype('uint8')

# ////////////////////////////////////////////////////////////////////////////////
def ComposeImage(images, axis):
	H = [single.shape[0] for single in images]
	W = [single.shape[1] for single in images]
	W,H=[(sum(W),max(H)),(max(W), sum(H))][axis]
	shape=list(images[0].shape)
	shape[0],shape[1]=H,W
	ret=numpy.zeros(shape=shape,dtype=images[0].dtype)
	cur=[0,0]
	for single in images:
		H,W=single.shape[0],single.shape[1]
		ret[cur[1]:cur[1]+H,cur[0]:cur[0]+W,:]=single
		cur[axis]+=[W,H][axis]
	return ret

# ////////////////////////////////////////////////////////////////////////////////
def SaveImage(filename,img):
	os.makedirs(os.path.dirname(filename), exist_ok=True)
	if os.path.isfile(filename):
		os.remove(filename)

	if len(img.shape)>3:
		raise Exception("cannot save 3d image")

	num_channels=img.shape[2] if len(img.shape)==3 else 1

	# opencv supports only grayscale, rgb and rgba
	if num_channels>3:
		img=img[:,:,0:3]
		num_channels=3

	# opencv does not support saving of 2 channel images
	if num_channels==2:
		R,G=img[:,:,0],img[:,:,1]
		B=numpy.zeros(R.shape,dtype=R.dtype)
		img=InterleaveChannels([R,G,B])

	cv2.imwrite(filename, img)

# ////////////////////////////////////////////////////////////////////////////////
def SaveUint8Image(filename,img):
	SaveImage(filename, ConvertImageToUint8(img))	


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
