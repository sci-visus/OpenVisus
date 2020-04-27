import os,sys,glob,threading,platform,sysconfig,re,time,subprocess, errno, fnmatch, shutil

# not fundamental
try:
	import numpy
except:
	pass

# not fundamental
try:
	import cv2
except:
	pass

WIN32=platform.system()=="Windows" or platform.system()=="win32"
APPLE=platform.system()=="Darwin"
LINUX=not APPLE and not WIN32

# ////////////////////////////////////////////////////////////////////////////////
def ThisDir(file):
	return os.path.dirname(os.path.abspath(file))

# ////////////////////////////////////////////////////////////////////////////////
def Assert(condition):
	if not condition:
		raise Exception("Assert failed")


# /////////////////////////////////////////////////////////////////////////
def GetCommandOutput(cmd):
	output=subprocess.check_output(cmd)
	if sys.version_info >= (3, 0): output=output.decode("utf-8")
	return output.strip()

# /////////////////////////////////////////////////////////////////////////
def CreateDirectory(value):
	try: 
		os.makedirs(value)
	except OSError:
		if not os.path.isdir(value):
			raise
	
# /////////////////////////////////////////////////////////////////////////
def GetFilenameWithoutExtension(filename):
	return os.path.splitext(os.path.basename(filename))[0]

# /////////////////////////////////////////////////////////////////////////
def CopyFile(src,dst):
	
	src=os.path.realpath(src) 
	dst=os.path.realpath(dst)		
	
	if src==dst or not os.path.isfile(src):
		return		

	CreateDirectory(os.path.dirname(dst))
	shutil.copyfile(src, dst)	
	
# /////////////////////////////////////////////////////////////////////////
def CopyDirectory(src,dst):
	
	src=os.path.realpath(src)
	
	if not os.path.isdir(src):
		return
	
	CreateDirectory(dst)
	
	# problems with symbolic links so using shutil	
	dst=dst+"/" + os.path.basename(src)
	
	if os.path.isdir(dst):
		shutil.rmtree(dst,ignore_errors=True)
		
	shutil.copytree(src, dst, symlinks=True)				
	
# /////////////////////////////////////////////////////////////////////////
def ReadTextFile(filename):
	file = open(filename, "r") 
	ret=file.read().strip()
	file.close()
	return ret
	
# /////////////////////////////////////////////////////////////////////////
def WriteTextFile(filename,content):
	if not isinstance(content, str):
		content="\n".join(content)+"\n"
	CreateDirectory(os.path.dirname(os.path.realpath(filename)))
	file = open(filename,"wt") 
	file.write(content) 
	file.close() 		

# /////////////////////////////////////////////////////////////////////////
# glob(,recursive=True) is not supported in python 2.x
# see https://stackoverflow.com/questions/2186525/use-a-glob-to-find-files-recursively-in-python
def RecursiveFindFiles(rootdir='.', pattern='*'):
  return [os.path.join(looproot, filename)
          for looproot, _, filenames in os.walk(rootdir)
          for filename in filenames
          if fnmatch.fnmatch(filename, pattern)]

# /////////////////////////////////////////////////////////////////////////
def PipInstall(packagename,extra_args=[]):
	cmd=[sys.executable,"-m","pip","install","--user",packagename]
	if extra_args: cmd+=extra_args
	print("# Executing",cmd)
	return_code=subprocess.call(cmd)
	return return_code==0

# ////////////////////////////////////////////////////////////////////////////////
def ParseDouble(value,default_value=0.0):

	if isinstance(value,str) and len(value)==0:
		return default_value
	try:
		return float(value)
	except:
		return default_value

# ////////////////////////////////////////////////////////////////////////////////
def ParseInt(value,default_value=0):

	if isinstance(value,str) and len(value)==0:
		return default_value
	try:
		return int(value)
	except:
		return default_value			
	
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
	
# /////////////////////////////////////////////////////////////////////////
def ExecuteCommand(cmd):	
	"""
	note: shell=False does not support wildcard but better to use this version
	because quoting the argument is not easy
	"""
	print("# Executing command: ",cmd)
	return subprocess.call(cmd, shell=False)


# ///////////////////////////////////////////////////////////////////////
def RunJobsInParallel(jobs, advance_callback=None, nthreads=8):

	class MyThread(threading.Thread):

		# constructor
		def __init__(self):
			super(MyThread, self).__init__()

		# run
		def run(self):
			for job in self.jobs:
				self.jobDone(job())

	nthreads=min(nthreads,len(jobs))
	threads,results=[],[]
	for WorkerId in range(nthreads):
		thread=MyThread()
		threads.append(thread)
		thread.jobs=[job for I,job in enumerate(jobs)  if (I % nthreads)==WorkerId]
		thread.jobDone=lambda result: results.append(result)
		thread.start()
			
	while True:

		time.sleep(0.01)

		if len(results)==len(jobs):
			[thread.join() for thread in threads]
			return results

		if advance_callback:
				advance_callback(len(results))

# /////////////////////////////////////////////////////////////////////////
def RemoveDirectory(dir):
	shutil.rmtree(os.path.abspath(dir),ignore_errors=True)		


# /////////////////////////////////////////////////////////////////////////
def RemoveFiles(pattern):
	files=glob.glob(pattern)
	print("Removing files",files)
	for it in files:
		if os.path.isfile(it):
			os.remove(it)
		else:
			shutil.rmtree(os.path.abspath(it),ignore_errors=True)		
			

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
