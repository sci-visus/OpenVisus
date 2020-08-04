import os,sys
import cv2

from OpenVisus             import *
from OpenVisus.gui         import *
from OpenVisus.image_utils import *

# ///////////////////////////////////////////////////////////////////////////////////
class VisibleMale:
	
	#  constructor
	def __init__(self):
		
		self.w, self.h,self.d =2048, 1216, 1878
		self.idx_filename=idx_filename="D:/GoogleSci/visus_dataset/male/non_aligned/visus.idx"
		self.filename_template="D:/GoogleSci/visus_dataset/male/RAW/Fullcolor/fullbody/a_vm%04d.raw"
		self.bInteractive=False
		self.filenames=[self.filename_template % (z,) for z in range(self.d)]
		
		# https://www.nlm.nih.gov/research/visible/visible_human.html#targetText=These%20images%2C%20each%20approximately%2032,all%201%2C871%20male%20color%20cryosections.&targetText=The%20Visible%20Human%20Female%20data,obtained%20at%200.33%20mm%20intervals.
		self.density=[0.33, 0.33, 1.0]
		
		# problem of 12 black slices
		self.filenames[1423:1435]=[self.filenames[1422]]*12


	# loadImage
	def loadImage(self,filename):
			print("Loading",filename)
			RGB=numpy.fromfile(filename, dtype=numpy.uint8)
			Assert(len(RGB)==self.w*self.h*3) 
			RGB=RGB.reshape((3, self.h, self.w)) #RRR...GGG...BBB
			R,G,B=RGB[0,:,:],RGB[1,:,:],RGB[2,:,:]
			A=numpy.zeros((self.h,self.w),dtype=numpy.uint8)
			return InterleaveChannels([R,G,B,A])
		
	# generateImage
	def generateImage(self, z=0):
		
		while z<self.d:
			RGBA=self.loadImage(self.filenames[z])
			R,G,B,A=[RGBA[:,:,I] for I in range(4)]

			#remove bands
			def keepArea(x1,x2,y1,y2):
				A[int(y1*self.h):int(y2*self.h),int(x1*self.w):int(x2*self.w)]=255
					
			if   z< 536: keepArea(0.05, 0.95, 0.03, 0.800)
			elif z< 612: keepArea(0.05, 0.95, 0.03, 0.835)
			elif z< 684: keepArea(0.05, 0.95, 0.03, 0.860)
			elif z< 775: keepArea(0.05, 0.95, 0.03, 0.840)
			elif z< 791: keepArea(0.05, 0.95, 0.03, 0.870)
			elif z< 811: keepArea(0.05, 0.95, 0.03, 0.885)
			elif z< 857: keepArea(0.05, 0.95, 0.03, 0.895)
			elif z< 884: keepArea(0.05, 0.95, 0.03, 0.860)
			elif z<1016: keepArea(0.05, 0.95, 0.03, 0.830)
			elif z<1435: keepArea(0.05, 0.95, 0.03, 0.720)
			elif z<1827: keepArea(0.05, 0.82, 0.03, 0.730)
			else:        keepArea(0.05, 0.84, 0.03, 0.800)
				
			R[A==0],G[A==0],B[A==0]=0,0,0

			if self.bInteractive:
				ShowImage(RGBA)
				key = cv2.waitKeyEx()	
				if key==27: sys.exit(0)
				elif key==2555904: z+=1
				elif key==2424832: z+=-1
			else:
				ShowImage(RGBA)
				cv2.waitKey(1) # wait 1 msec just to allow the image to appear
				z+=1
					
			yield RGBA
		
	# convertToIdx
	def convertToIdx(self):
		
		physic_box=BoxNd(PointNd(0,0,0),PointNd(self.w*self.density[0],self.h*self.density[1],self.d*self.density[2]))

		db=CreateIdx(
			url=self.idx_filename, 
			rmtree=True,
			dim=3,
			dims=(self.w,self.h,self.d),
			blockperfile=-1,
			filename_template="./visus.bin",
			fields=[Field("data","uint8[4]","row_major")],
			bounds=Position(physic_box))

		db.writeSlabs(self.generateImage(), max_memsize=4*(1024*1024*1024))

	# runViewer
	def runViewer(self):
	
		viewer=PyViewer()
		
		if False:
			viewer.open(self.idx_filename)
			viewer.setScriptingCode("""
from OpenVisus.image_utils import *
import numpy
R,G,B,A=SplitChannels(input)
Brighness=0.2126*R + 0.7152*G + 0.0722*B
A[Brighness<70]=0
A[R>76  ]=255
A[G-20>R]=0
A[B-20>R]=0	
output=InterleaveChannels([R,G,B,A])
""")
			
		else:
			
			viewer.addGLCamera("glcamera", viewer.getRoot(),"lookat")
			
			flag_tatoo=(0.04, 0.95, 0.05,0.73, 0.15,0.15+0.1)	
			head=(0.35,0.65, 0.18,0.82, 0.0,0.2)	
			left_arm=(0.0, 0.4, 0.1,0.8, 0.0,0.3)
			full=(0.00,1.00, 0.00,1.00, 0.00,1.00)
			region=head
			
			db=LoadDataset(self.idx_filename)
			logic_box=db.getLogicBox(x=region[0:2],y=region[2:4],z=region[4:6])
			RGBA=db.read(logic_box,quality=0)	
			bounds=db.getBounds(logic_box)

			# remove the blueish
			R,G,B,A=SplitChannels(RGBA)
			A[R<76]  =0
			A[R>76  ]=255
			A[G>R]=0
			A[B>R]=0			
			
			viewer.addVolumeRender(RGBA, bounds)
			#viewer.addIsoSurface(field=R, second_field=RGBA, isovalue=100.0, bounds=bounds)
		
		viewer.run()

	

# /////////////////////////////////////////////////////////////////
class VisibleFemale:
	
	# constructor
	def __init__(self):
		pass
	
	# runViewer
	def runViewer(self):
		
		db=LoadDataset(r"D:\GoogleSci\visus_dataset\female\visus.idx")
		Assert(db)

		full=(0,1, 0,1, 0,1.0)	
		head=(0,1, 0,1, 0,0.1)
		region=head

		logic_box=db.getLogicBox(x=region[0:2],y=region[2:4],z=region[4:6])

		RGBA =db.read(logic_box=logic_box,quality=G-6)	
		bounds=db.getBounds(logic_box)
		
		# remove the blueish
		R,G,B,A=[RGBA[:,:,I] for I in range(4)]
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
			

		viewer=PyViewer()
		viewer.addGLCamera("camera",viewer.getRoot(),"lookat")
		viewer.addIsoSurface(field=R, second_field=None, isovalue=100.0, bounds=bounds)
		#viewer.addVolumeRender(RGBA, bounds)	
		
		viewer.run()
	

# //////////////////////////////////////////////
def Main(argv):

	if True:
		male=VisibleMale()
		# male.convertToIdx()
		male.runViewer()
	

	print("ALL DONE, press a key")
	sys.stdin.read(1)
	sys.exit(0)	


# //////////////////////////////////////////////
if __name__ == '__main__':
	Main(sys.argv)




