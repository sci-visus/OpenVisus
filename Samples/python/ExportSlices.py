import os,sys
from OpenVisus import *
import cv2
import numpy

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


# //////////////////////////////////////////////
def Main(argv):

	# set PYTHONPATH=D:/projects/OpenVisus/build/RelWithDebInfo
	# c:\Python37\python.exe Samples\convert\visiblemale.py

	#physic_box
	# 0 675.84 0  401.28 0 1871.00
	# 0 2047 0 1215 0 1877

	x1,y1,z1=0,0,0
	x2,y2,z2=2048,1216,1878
	vs=(1.0,1.0,3.019008)

	SetCommandLine("__main__")
	IdxModule.attach()

	old=LoadDataset(r"D:\GoogleSci\visus_dataset\male\visus.idx")
	new=LoadDataset(r"D:\GoogleSci\visus_dataset\male\RAW\Fullcolor\fullbody\VisusSlamFiles\visus.idx")

	if not old or not new:
		raise Exception("error")

	if False:
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
			

	IdxModule.detach()
	print("Done with conversion")
	sys.exit(0)

# //////////////////////////////////////////////
if __name__ == '__main__':
	Main(sys.argv)




