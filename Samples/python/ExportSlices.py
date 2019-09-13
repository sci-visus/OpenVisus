import os,sys

from OpenVisus       import *
from VisusGuiPy      import *
from VisusGuiNodesPy import *
from VisusAppKitPy   import *
	
import PyQt5
from   PyQt5.QtCore    import *
from   PyQt5.QtWidgets import *
from   PyQt5.QtGui     import *
import PyQt5.sip  as  sip

import numpy
import cv2

# //////////////////////////////////////////////
def Assert(cond):
	if not cond:
		raise Exception("internal error")

# //////////////////////////////////////////////
def ShowPreview(img,m=1024):
		w,h=img.shape[1],img.shape[0]
		r=h/float(w)
		if w>h:
			w=min([w,m])
			h=int(w*r)
		else:
			h=min([h,m])
			w=int(h/r)
		cv2.imshow("img",cv2.resize(img,(w,h)))
		cv2.waitKey(1)

# //////////////////////////////////////////////
class Movie:

	# constructor
	def __init__(self,filename):
		self.filename=filename
		self.out=None
		self.input=None

	# release
	def release(self):
		if self.out:
			self.out.release()

	# write
	def write(self,img):

		if self.out is None:
			self.width,self.height=img.shape[1],img.shape[0]
			self.out = cv2.VideoWriter(self.filename,cv2.VideoWriter_fourcc(*'DIVX'), 15, (self.width,self.height))

		ShowPreview(img)
		self.out.write(img)

	# read
	def read(self):
		if self.input is None:
			self.input=cv2.VideoCapture(self.filename)
		success,img=self.input.read()
		if not success: return None
		self.width,self.height=img.shape[1],img.shape[0]
		return img

# //////////////////////////////////////////////
def ExportSlices(filename,dataset,logic_box,axis,vs=[1,1,1],delta=1):

	access=dataset.createAccess()
	field=dataset.getDefaultField()
	time=dataset.getDefaultTime()

	out=Movie(filename)

	offsets=range(logic_box.p1[axis],logic_box.p2[axis],delta)
	for I,offset in enumerate(offsets):

		print("Doing","I","of",len(offsets))

		query_box=BoxNi(logic_box)
		query_box.p1.set(axis,offset+0)
		query_box.p2.set(axis,offset+1)

		img=dataset.readFullResolutionData(access, field, time, query_box)

		# resize to respect the aspect ratio
		size=[int(vs[I]*img.dims[I]) for I in range(3)]

		if axis==0:
			img.resize(PointNi(img.dims[1],img.dims[2]),img.dtype,__file__,0)
			img=cv2.resize(img.toNumPy(img,bShareMem=True),(size[1],size[2]))

		elif axis==1:
			img.resize(PointNi(img.dims[0],img.dims[2]),img.dtype,__file__,0)
			img=cv2.resize(img.toNumPy(img,bShareMem=True),(size[0],size[2]))

		else:
			img.resize(PointNi(img.dims[0],img.dims[1]),img.dtype,__file__,0)
			img=cv2.resize(img.toNumPy(img,bShareMem=True),(size[0],size[1]))

		img=cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
		out.write(img)

	out.release()


# //////////////////////////////////////////////
def ComposeMovies(out,args,axis=1):

	while True:

		images=[]
		for input,x1,x2,y1,y2 in args:
			img=input.read()
			if img is None: continue 
			w,h=img.shape[1],img.shape[0]
			img=img[
				int(float(y1)*h):int(float(y2)*h),
				int(float(x1)*w):int(float(x2)*w),
				:] # crop
			images.append(img)
			
		if not images:
			out.release()
			return

		out.write(numpy.concatenate(images, axis=axis))



# ////////////////////////////////////////////////////////////////
def ShowIsoSurface(dataset,isovalue,physic_box,x1,x2,y1,y2,z1,z2):

    logic_box=dataset.getLogicBox()
    logic_to_physic=Position.computeTransformation(Position(physic_box),logic_box)

    W,H,D=logic_box.size()[0],logic_box.size()[1],logic_box.size()[2]
	
    logic_box=BoxNi(
        PointNi(int(x1*W),int(y1*H),int(z1*D)),
        PointNi(int(x2*W),int(y2*H),int(z2*D)))

    bounds=Position(logic_to_physic,Position(logic_box))

    print("Extracting full resolutin data...")
    data=dataset.readFullResolutionData(dataset.createAccess(), dataset.getDefaultField(), dataset.getDefaultTime(),logic_box)
    print("done")

    data=Array.toNumPy(data)
    Assert(data.dtype==numpy.uint8)

    # convert to grayscale
    Assert(data.shape[3]==3 and data.dtype==numpy.uint8) # RGB
    R,G,B=[data[:,:,:,I] for I in range(3)]
    data=(0.299 * R + 0.587 * G + 0.114 * B).astype('uint8')

    # finally show in OpenVisus
    viewer=Viewer()
    viewer.setMinimal()
    viewer.addGLCameraNode("lookat")
    node=viewer.addIsoContourNode(viewer.getRoot(),None)
    data=Array.fromNumPy(data)
    data.bounds=bounds	
    node.setData(data)
    node.setIsoValue(isovalue)
    glcamera=viewer.getGLCamera()
    glcamera.guessPosition(bounds)
    return viewer

# //////////////////////////////////////////////
def Main(argv):

	# set PYTHONPATH=D:/projects/OpenVisus/build/RelWithDebInfo
	# c:\Python37\python.exe Samples\python\ExtractSlices.py
	
	SetCommandLine("__main__")
	GuiModule.createApplication()
	AppKitModule.attach()  		

	old=LoadDataset(r"D:\GoogleSci\visus_dataset\male\visus.idx")
	new=LoadDataset(r"D:\GoogleSci\visus_dataset\male\RAW\Fullcolor\fullbody\VisusSlamFiles\visus.idx")

	Assert(old and new)
	
	logic_box =BoxNi(PointNi(3),PointNi(2048,1216,1878))
	physic_box=BoxNd(PointNd(3),PointNd(675.84,401.28,1871.00))	

	if False:
		
		vs=(1.0,1.0,3.019008)
		
		ExportSlices("All_z0.avi",old, old.getLogicBox(), axis=2, vs=vs, delta=1)
		ExportSlices("All_z1.avi",new, new.getLogicBox(), axis=2, vs=vs, delta=1)

		ExportSlices("All_x0.avi",old, old.getLogicBox(), axis=0, vs=vs, delta=1)
		ExportSlices("All_x1.avi",new, new.getLogicBox(), axis=0, vs=vs, delta=1)

		ExportSlices("All_y0.avi",old, old.getLogicBox(), axis=1, vs=vs, delta=1)
		ExportSlices("All_y1.avi",new, new.getLogicBox(), axis=1, vs=vs, delta=1)

	if False:

		ComposeMovies(Movie("Up_x0.avi"),[(Movie("All_x0.avi"),0.0,1.0, 0.0,0.4),(Movie("All_x1.avi"),0.0,1.0, 0.0,0.4) ])
		ComposeMovies(Movie("Up_y0.avi"),[(Movie("All_y0.avi"),0.0,1.0, 0.0,0.4),(Movie("All_y1.avi"),0.0,1.0, 0.0,0.4) ])
		
		ComposeMovies(Movie("Dw_x0.avi"),[(Movie("All_x0.avi"),0.0,1.0, 0.6,1.0),(Movie("All_x1.avi"),0.0,1.0, 0.6,1.0) ])
		ComposeMovies(Movie("Dw_y0.avi"),[(Movie("All_y0.avi"),0.0,1.0, 0.6,1.0),(Movie("All_y1.avi"),0.0,1.0, 0.6,1.0) ])
			
	if False:
		viewer=ShowIsoSurface(old,100.0,physic_box,0.35,0.65, 0.18,0.82, 0.0,0.2)
        viewer=ShowIsoSurface(new,100.0,physic_box,0.35,0.65, 0.15,0.85, 0.0,0.2)
		GuiModule.execApplication()
		viewer=None  
	
	AppKitModule.detach()

	print("ALL DONE")
	sys.stdin.read(1)
	sys.exit(0)	


# //////////////////////////////////////////////
if __name__ == '__main__':
	Main(sys.argv)




