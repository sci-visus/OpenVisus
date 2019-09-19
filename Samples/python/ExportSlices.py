import os,sys

from OpenVisus	   import *
from VisusGuiPy	  import *
from VisusGuiNodesPy import *
from VisusAppKitPy   import *
	
import PyQt5
from   PyQt5.QtCore	import *
from   PyQt5.QtWidgets import *
from   PyQt5.QtGui	 import *
import PyQt5.sip  as  sip

import numpy
import cv2

# //////////////////////////////////////////////
def Assert(cond):
	if not cond:
		raise Exception("internal error")
		
# //////////////////////////////////////////////
class NumPyUtils:
	
	@staticmethod
	def swapRedBlue(img):
		Assert(len(img.shape)==3) #YXC
		ret=img.copy()
		ret[:,:,0],ret[:,:,2]=img[:,:,2],img[:,:,0]
		return ret

	@staticmethod
	def interleaveChannels(channels):
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

	@staticmethod
	def splitChannels(data):
		N=len(data.shape)
		if N==3: return [data[  :,:,C] for C in range(data.shape[-1])]
		if N==4: return [data[:,:,:,C] for C in range(data.shape[-1])]
		raise Exception("internal error")	

	@staticmethod
	def convertToGrayScale(data):	
		Assert(isinstance(data,numpy.ndarray))
		R,G,B=data[:,:,:,0],data[:,:,:,1] ,data[:,:,:,2]
		return (0.299 * R + 0.587 * G + 0.114 * B).astype(data.dtype)

	@staticmethod
	def addAlphaChannel(data):
		Assert(isinstance(data,numpy.ndarray))
		dtype=data.dtype
		channels = [data[:,:,:,C] for C in range(data.shape[-1])]
		alpha=numpy.zeros(channels[0].shape,dtype=dtype)
		return channels + [alpha,]

	@staticmethod
	def showImage(img,max_preview_size=1024):
		
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
			img=NumPyUtils.interleaveChannels([
				numpy.multiply(img[:,:,0],A).astype(R.dtype),
				numpy.multiply(img[:,:,1],A).astype(G.dtype),
				numpy.multiply(img[:,:,2],A).astype(B.dtype)])
		
		cv2.imshow("img",img)
		cv2.waitKey(1) # wait 1 msec just to allow the image to appear


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
		NumPyUtils.showImage(img)
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



# ////////////////////////////////////////////////////////////////
class PyViewer(Viewer):
	
	# constructor
	def __init__(self):	
		super(PyViewer, self).__init__()
		self.setMinimal()
		self.addGLCameraNode("lookat")	
		
	# run
	def run(self):
		bounds=self.getWorldBounds()
		self.getGLCamera().guessPosition(bounds)	
		GuiModule.execApplication()		
		
	# addVolumeRender
	def addVolumeRender(self, data, bounds):
		
		t1=Time.now()
		print("Adding Volume render...")	
		
		Assert(isinstance(data,numpy.ndarray))
		data=Array.fromNumPy(data,TargetDim=3,bounds=bounds)
		
		node=RenderArrayNode()
		node.setLightingEnabled(False)
		node.setPaletteEnabled(False)
		node.setData(data)
		self.addNode(self.getRoot(), node)
		
		print("done in ",t1.elapsedMsec(),"msec")
		
		
	# addIsoSurface
	def addIsoSurface(self, field=None, second_field=None, isovalue=0, bounds=None):

		Assert(isinstance(field,numpy.ndarray))
		
		field=Array.fromNumPy(field,TargetDim=3,bounds=bounds)

		print("Extracting isocontour...")
		t1=Time.now()
		isocontour=MarchingCube(field,isovalue).run()
		print("done in ",t1.elapsedMsec(),"msec")
		
		if second_field is not None:
			isocontour.second_field=Array.fromNumPy(second_field,TargetDim=3,bounds=bounds)
					

		node=IsoContourRenderNode()	
		node.setIsoContour(isocontour)
		self.addNode(self.getRoot(),node)
		
		print("Added IsoContourRenderNode")	
		
		

# //////////////////////////////////////////////
class PyDataset(object):
	
	# constructor
	def __init__(self,url):
		self.dataset = LoadDataset(url)
		
	# __getattr__
	def __getattr__(self,attr):
	    return getattr(self.dataset, attr)	
	    
	# readData
	def readData(self,logic_box):
		t1=Time.now()
		
		# in alpha coordinate
		if type(logic_box) in [list, tuple]: 
			alpha=logic_box
			logic_box=BoxNi(
					PointNi(int(alpha[0]*self.getLogicBox().size()[0]),int(alpha[2]*self.getLogicBox().size()[1]),int(alpha[4]*self.getLogicBox().size()[2])),
					PointNi(int(alpha[1]*self.getLogicBox().size()[0]),int(alpha[3]*self.getLogicBox().size()[1]),int(alpha[5]*self.getLogicBox().size()[2])))

		print("Extracting full resolution data...","logic_box",logic_box.toString())
		data=self.dataset.readFullResolutionData(self.createAccess(), self.getDefaultField(), self.getDefaultTime(),logic_box)
		
		# compact dimension
		if True:
			dims=[data.dims[I] for I in range(data.dims.getPointDim()) if data.dims[I]>1 ]
			data.resize(PointNi(dims),data.dtype,__file__,0)
			
		print("done in %dmsec dims(%s) bounds(%s)" % (t1.elapsedMsec(),data.dims.toString(),data.bounds.toString()))
		return Array.toNumPy(data),Position(data.bounds)	
		
	# readSlice
	def readSlice(self,axis,offset):
		logic_box=BoxNi(self.getLogicBox())
		logic_box.p1.set(axis,offset+0)
		logic_box.p2.set(axis,offset+1)	
		data, physic_bounds=self.readData(logic_box)
		return (data,logic_box,physic_bounds.toAxisAlignedBox())
		
	# readSlices
	def readSlices(self, axis, delta=1):
		offsets=range(self.getLogicBox()	.p1[axis],self.getLogicBox().p2[axis],delta)
		for I,offset in enumerate(offsets):
			print("Doing","I","of",len(offsets))
			yield self.readSlice(axis,offset)	
			
	# exportSlicesToMovie
	def exportSlicesToMovie(self,out_movie, axis,preserve_ratio=True):
		
		for img, logic_box, physic_box in self.readSlices(axis):
			
			# resize to preserve the right ratio
			if preserve_ratio:
				density=[float(logic_box.size()[I])/float(physic_box.size()[I]) for I in range(3)]
				max_density=max(density)
				num_pixels=[int(physic_box.size()[I] * max_density) for I in range(3)]
				perm=((1,2,0),(0,2,1),(0,1,2))
				X,Y=perm[axis][0],perm[axis][1]				
				new_size=(num_pixels[X],num_pixels[Y])
				img=cv2.resize(img,new_size)

			out_movie.writeFrame(NumPyUtils.swapRedBlue(img))
			
		out_movie.release()			
		
	

# //////////////////////////////////////////////
def Main(argv):

	# set PYTHONPATH=D:/projects/OpenVisus/build/RelWithDebInfo
	# c:\Python37\python.exe Samples\python\ExtractSlices.py
	
	SetCommandLine("__main__")
	GuiModule.createApplication()
	AppKitModule.attach()  	
		
	old_dataset=PyDataset(r"D:\GoogleSci\visus_dataset\male\visus.idx")
	new_dataset=PyDataset(r"D:\GoogleSci\visus_dataset\male\RAW\Fullcolor\fullbody\VisusSlamFiles\visus.idx")

	Assert(old_dataset)
	Assert(new_dataset)
	
	flag_tatoo=(0.04, 0.95, 0.05,0.73, 0.15,0.15+0.1)	
	head=(0.35,0.65, 0.18,0.82, 0.0,0.2)	
	left_arm=(0.0, 0.4, 0.1,0.8, 0.0,0.3)
	
	if False:
		
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
			
	if False:
		
		dataset=new_dataset
		viewer=PyViewer()
		RGB,bounds=dataset.readData(flag_tatoo)
		R,G,B=NumPyUtils.splitChannels(RGB)
		viewer.addIsoSurface(field=R, second_field=RGB, isovalue=100.0, bounds=bounds)
		viewer.run()
	
	if True:
		
		dataset=new_dataset
		debug_mode=False
		viewer=PyViewer()
		data, bounds=dataset.readData(left_arm)	
		
		# wherever the red component is predominant, keep it
		print("Setting up alpha channel")
		R,G,B,A=NumPyUtils.addAlphaChannel(data)
		A[R>100]=255
		A[G>R]=0
		A[B>R]=0				
		
		data=NumPyUtils.interleaveChannels((R,G,B,A))

		if debug_mode:
			for Z in range(data.shape[0]):
				NumPyUtils.showImage("Debug",NumPyUtils.swapRedBlue(data[Z,:,:,:])) 
				cv2.waitKey()		
			
		viewer.addVolumeRender(data, bounds)
		viewer.run()
	
	AppKitModule.detach()
	print("ALL DONE")
	sys.stdin.read(1)
	sys.exit(0)	


# //////////////////////////////////////////////
if __name__ == '__main__':
	Main(sys.argv)




