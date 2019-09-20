import os,sys
import numpy
import cv2

from OpenVisus import *

WIN32=platform.system()=="Windows" or platform.system()=="win32"
APPLE=platform.system()=="Darwin"

# ////////////////////////////////////////////////////////////////////////////////
def ThisDir():
	return os.path.dirname(os.path.abspath(__file__))

# ////////////////////////////////////////////////////////////////////////////////
def Assert(condition):
	if not condition:
		raise Exception("Assert failed")


# ////////////////////////////////////////////////////////////////////////////////
def ParseDouble(value):
	try:
		return float(value)
	except:
		return 0.0

# ////////////////////////////////////////////////////////////////////////////////
def ParseInt(value):
	try:
		return int(value)
	except:
		return 0			
	
# ////////////////////////////////////////////////////////////////////////////////
def LoadTextDocument(filename):

	if not os.path.isfile(filename):
		return []

	file=open(filename,"rt")
	content=file.read()
	file.close()	
	return content

# ////////////////////////////////////////////////////////////////////////////////
def SaveTextDocument(filename,content):
	try:
		os.makedirs(os.path.dirname(filename),exist_ok=True)
	except:
		pass

	file=open(filename,"wt")
	file.write(content)
	file.close()		
	
# ////////////////////////////////////////////////////////////////////////////////
def GuessUniqueFilename(pattern):
	I=0
	while True:
		filename=pattern %(I,)
		if not os.path.isfile(filename): 
			return filename
		I+=1

# ////////////////////////////////////////////////////////////////////////////////
def KillProcess(process):
	if not process:
		return
	try:
		process.kill()
	except:
		pass	
	
# ////////////////////////////////////////////////////////////////////////////////
def ExecuteCommand(cmd):
	print(" ".join(cmd))
	subprocess.Popen(cmd).wait()
			
# ////////////////////////////////////////////////////////////////////////////////
def TryRemoveFiles(mask):

	for filename in glob.glob(mask):
		try:
			os.remove(filename)
		except:
			pass

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
		
	return sorted(ret)

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
def InterleaveChannels(channels):
	shape=channels[0].shape + (len(channels),)
	ret=numpy.zeros(shape,dtype=channels[0].dtype)
	pdim=len(channels[0].shape)
	for C in range(len(channels)):
		if pdim==2:
			ret[:,:,C]=channels[C] # YXC
		elif pdim==3:
			ret[:,:,:,C]=channels[C] # ZYXC
		else:
			raise Exception("internal error")
	return ret 

# //////////////////////////////////////////////
def SplitChannels(data):
	N=len(data.shape)
	if N==3: return [data[  :,:,C] for C in range(data.shape[-1])]
	if N==4: return [data[:,:,:,C] for C in range(data.shape[-1])]
	raise Exception("internal error")	

# //////////////////////////////////////////////
def ConvertToGrayScale(data):	
	Assert(isinstance(data,numpy.ndarray))
	R,G,B=data[:,:,:,0],data[:,:,:,1] ,data[:,:,:,2]
	return (0.299 * R + 0.587 * G + 0.114 * B).astype(data.dtype)
	
# ////////////////////////////////////////////////////////////////////////////////
def ConvertImageToGrayScale(img):
	if len(img.shape)>=3 and img.shape[2]==3:
		return cv2.cvtColor(img[:,:,0:3], cv2.COLOR_RGB2GRAY)
	else:
		return img[:,:,0] 

# ////////////////////////////////////////////////////////////////////////////////
def ResizeImage(src,width,height=0):
	return cv2.resize(src, (width,height if height>0 else int(src.shape[0] / float(src.shape[1]) * width)), interpolation=cv2.INTER_CUBIC)


# //////////////////////////////////////////////
def AddAlphaChannel(data):
	Assert(isinstance(data,numpy.ndarray))
	dtype=data.dtype
	channels = [data[:,:,:,C] for C in range(data.shape[-1])]
	alpha=numpy.zeros(channels[0].shape,dtype=dtype)
	return channels + [alpha,]

# //////////////////////////////////////////////
def ShowImage(img,max_preview_size=1024,win_name="img"):
	
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
	cv2.waitKey(1) # wait 1 msec just to allow the image to appear


		
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
def SaveImage(filename,img):
	os.makedirs(os.path.dirname(filename), exist_ok=True)
	if os.path.isfile(filename):
		os.remove(filename)
	cv2.imwrite(filename, img)

# ////////////////////////////////////////////////////////////////////////////////
def SaveUint8Image(filename,img):
	img=ConvertImageToUint8(img)
	if len(img.shape)>3 and img.shape[2]>3:
		img=img[:,:,0:3]
	SaveImage(filename, img)	


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
