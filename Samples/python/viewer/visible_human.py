import os,sys,pickle
import platform,subprocess,glob,datetime
import cv2
import numpy
import random
import threading
import time

from OpenVisus                        import *
from OpenVisus.gui                    import *
from OpenVisus.image_utils            import *

from slampy.extract_keypoints         import *
from slampy.gui_utils                 import *
from slampy.image_utils               import *
from slampy.image_provider            import *


# ///////////////////////////////////////////////////////////////////////////////////
class VisibleMale:
	
	#  constructor
	def __init__(self):
		
		self.w, self.h,self.d =2048, 1216, 1878
		self.filename_template="D:/GoogleSci/visus_dataset/male/RAW/Fullcolor/fullbody/a_vm%04d.raw"
		self.filenames=[self.filename_template % (z,) for z in range(self.d)]
		
		# https://www.nlm.nih.gov/research/visible/visible_human.html#targetText=These%20images%2C%20each%20approximately%2032,all%201%2C871%20male%20color%20cryosections.&targetText=The%20Visible%20Human%20Female%20data,obtained%20at%200.33%20mm%20intervals.
		self.density=[0.33, 0.33, 1.0]
		
		# problem of 12 black slices
		self.filenames[1423:1435]=[self.filenames[1422]]*12
			
		# if you want to speed up for debugging
		# self.filenames=self.filenames[0:50]

	# loadRawImage
	def loadRawImage(self,filename):
		print("Loading",filename)
		RGB=numpy.fromfile(filename, dtype=numpy.uint8)
		Assert(len(RGB)==self.w*self.h*3) 
		RGB=RGB.reshape((3, self.h, self.w)) #RRR...GGG...BBB
		R,G,B=RGB[0,:,:],RGB[1,:,:],RGB[2,:,:]
		return [R,G,B]
			
	# generateAlpha
	def generateAlpha(self, z):
		A=numpy.zeros((self.h,self.w),dtype=numpy.uint8)

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
		return A
	
	# generateNonAlignedImages
	def generateNonAlignedImages(self,bInteractive=False):
		z=0
		while z<self.d:
			R,G,B=self.loadRawImage(self.filenames[z])
			A=self.generateAlpha(z)
			R[A==0],G[A==0],B[A==0]=0,0,0
			RGBA=InterleaveChannels([R,G,B,A])

			if bInteractive:
				ShowImage(RGBA)
				key = cv2.waitKeyEx()	
				if key==27: sys.exit(0)
				elif key==2555904: z+=1
				elif key==2424832: z-=1
			else:
				ShowImage(RGBA)
				cv2.waitKey(1) # wait 1 msec just to allow the image to appear
				z+=1
					
			yield RGBA
			
	# createIdx
	def createIdx(self, filename):
		return CreateIdx(
			url=filename, 
			rmtree=False,
			dim=3,
			dims=(self.w,self.h,self.d),
			blockperfile=-1,
			filename_template="./visus.bin",
			fields=[Field("data","uint8[4]","row_major")],
			bounds=Position(BoxNd(PointNd(0,0,0),PointNd(self.w*self.density[0],self.h*self.density[1],self.d*self.density[2]))))


	# generateAlignedImages
	def generateAlignedImages(self, 
		cache_dir,
		energy_size = 1280,
		min_num_keypoints = 3000,
		max_num_keypoints = 6000,
		anms = 1300,
		max_reproj_error = 0.01, 
		ratio_check = 0.80,
		ba_tolerance = 0.005,
		fov=60.0):
		
		slam=Slam()
		slam.width   = self.w
		slam.height  = self.h
		slam.dtype   = DType.fromString("uint8[3]")
		
		# guessing calibration (NOTE: calibration is not used at all, at least for now)
		f=slam.width * (0.5 / math.tan(0.5* math.radians(fov)))
		cx,cy=slam.width*0.5,slam.height*0.5			
		slam.calibration        = Calibration(f,cx,cy)
		slam.calibration.bFixed = False 
		
		os.makedirs(cache_dir,exist_ok=True)

		# add cameras
		for id,filename in enumerate(self.filenames):
			camera=Camera()
			camera.id=id
			camera.color = Color.random()
			camera.filenames.append(filename)
			slam.addCamera(camera)
			
		# extract keypoints
		if True:
			
			extractor=ExtractKeyPoints(min_num_keypoints, max_num_keypoints, anms)

			for camera in slam.cameras:

				keypoint_filename = cache_dir+"/keypoints/%04d" % (camera.id,)

				if slam.loadKeyPoints(camera, keypoint_filename):
					print("Loading cached keypoints",camera.id)
					
				else:
					
					print("Generating keypoints",camera.id)
					R,G,B = self.loadRawImage(camera.filenames[0])
					A=self.generateAlpha(camera.id)
					R[A==0],G[A==0],B[A==0]=0,0,0
					RGB=InterleaveChannels([R,G,B])

					energy=(0.3 * R + 0.59 * G + 0.11 * B).astype(numpy.uint8)

					energy=ResizeImage(energy, energy_size)
					(keypoints,descriptors)=extractor.doExtract(energy)
					Assert(keypoints)

					vs=slam.width/float(energy.shape[1])

					camera.keypoints.clear()
					camera.keypoints.reserve(len(keypoints))
					for keypoint in keypoints:
						camera.keypoints.push_back(KeyPoint(vs*keypoint.pt[0], vs*keypoint.pt[1], keypoint.size, keypoint.angle, keypoint.response, keypoint.octave, keypoint.class_id))
					camera.descriptors=Array.fromNumPy(descriptors,TargetDim=2) 

					slam.saveKeyPoints(camera,keypoint_filename)

					# show keypoints
					if True:
						energy=cv2.cvtColor(energy, cv2.COLOR_GRAY2RGB)
						for keypoint in keypoints:
							cv2.drawMarker(energy, (int(keypoint.pt[0]), int(keypoint.pt[1])), (255, 255, 0), cv2.MARKER_CROSS, 5)
						energy=ConvertImageToUint8(energy)
						SaveImage(cache_dir+"/energy/%04d.tif" % (camera.id,), energy)
						ShowImage(energy)
						cv2.waitKey(1) # just to show the image
					

		# find all matches
		if True:
			
			# local cameras
			for I in range(len(slam.cameras)-1):
				camera1=slam.cameras[I+0]
				camera2=slam.cameras[I+1]
				camera1.addLocalCamera(camera2)				
			
			for camera2 in slam.cameras:
				for camera1 in camera2.getAllLocalCameras():
					if camera1.id < camera2.id:
						Assert(not camera1.keypoints.empty())
						Assert(not camera2.keypoints.empty())

						matches, H21, err=FindMatches(slam.width,slam.height,
							camera1.id,[(k.x, k.y) for k in camera1.keypoints],Array.toNumPy(camera1.descriptors), 
							camera2.id,[(k.x, k.y) for k in camera2.keypoints],Array.toNumPy(camera2.descriptors),
							max_reproj_error * slam.width, ratio_check)
							
						if err:
							print("matches",camera1.id,camera2.id,len(matches), err)
							camera2.getEdge(camera1).setMatches([],err)
							Assert(False) # should not happen
						else:
							print("matches",camera1.id,camera2.id,len(matches), "OK")
							matches=[Match(match.queryIdx,match.trainIdx, match.imgIdx, match.distance) for match in matches]
							camera2.getEdge(camera1).setMatches(matches,str(len(matches)))	
		
		# slam.removeDisconnectedCameras()
		
		# bundle adjustment
		if True:
			for I,tolerance in enumerate((10.0*ba_tolerance,1.0*ba_tolerance)):
				slam.bundleAdjustment(tolerance,"offset")
				slam.removeOutlierMatches(max_reproj_error)
				#slam.removeDisconnectedCameras()
				#slam.removeCamerasWithTooMuchSkew()

		# write aligned 
		for camera in slam.cameras:
			
			# note: there are few slices that are totally black, going to use them as they are
			offset=(round(camera.homography(0,2)),round(camera.homography(1,2)))
			print("offset",camera.id,offset)
			
			R,G,B=self.loadRawImage(camera.filenames[0])
			A=self.generateAlpha(camera.id)
			R[A==0],G[A==0],B[A==0]=0,0,0

			x1,x2 = (0,self.w)
			y1,y2 = (0,self.h)

			X1,X2 = (max([x1+offset[0],0]),min([x2+offset[0],self.w]))
			Y1,Y2 = (max([y1+offset[1],0]),min([y2+offset[1],self.h]))

			x1,x2=(X1-offset[0],X2-offset[0])
			y1,y2=(Y1-offset[1],Y2-offset[1])

			RGBA=numpy.zeros((self.h,self.w,4),dtype='uint8')
			RGBA[Y1:Y2, X1:X2, 0]=R[y1:y2,x1:x2]
			RGBA[Y1:Y2, X1:X2, 1]=G[y1:y2,x1:x2]
			RGBA[Y1:Y2, X1:X2, 2]=B[y1:y2,x1:x2]
			RGBA[Y1:Y2, X1:X2, 3]=A[y1:y2,x1:x2]

			filename=cache_dir+"/aligned/%04d.tif" % (camera.id,)
			# SaveImage(filename, RGBA)
			
			ShowImage(RGBA)
			cv2.waitKey(1) # wait 1 msec just to allow the image to appear

			yield RGBA


	# runViewer
	def runViewer(self,idx_filename):
	
		viewer=PyViewer()
		
		if False:
			viewer.open(idx_filename)
			viewer.setScriptingCode("""
import numpy
from OpenVisus.image_utils import *
R,G,B,A=[input[...,I] for I in range(4)]
A.fill(255)
A[(R<76) | (G>R) | (B>R)]=0
output=input
output[...,3]=A
""")
			
		else:
			
			viewer.addGLCamera("glcamera", viewer.getRoot(),"lookat")
			
			flag_tatoo=(0.04, 0.95, 0.05,0.73, 0.15,0.15+0.1)	
			head=(0.35,0.65, 0.18,0.82, 0.0,0.2)	
			left_arm=(0.0, 0.4, 0.1,0.8, 0.0,0.3)
			full=(0.00,1.00, 0.00,1.00, 0.00,1.00)
			region=full
			
			db=LoadDataset(idx_filename)
			logic_box=db.getLogicBox(x=region[0:2],y=region[2:4],z=region[4:6])
			RGBA=db.read(logic_box,quality=-6)	
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

# //////////////////////////////////////////////
def Main(argv):

	male=VisibleMale()

	non_aligned="D:/GoogleSci/visus_dataset/male/non_aligned"	
	aligned="D:/GoogleSci/visus_dataset/male/aligned"

	db=male.createIdx(non_aligned + "/visus.idx")
	db.writeSlabs(male.generateNonAlignedImages())
		
	#db=male.createIdx(aligned+"/visus.idx")
	#db.writeSlabs(male.generateAlignedImages(aligned))
		
	male.runViewer(non_aligned+"/visus.idx")
	
	print("ALL DONE, press a key")
	sys.stdin.read(1)
	sys.exit(0)	


# //////////////////////////////////////////////
if __name__ == '__main__':
	Main(sys.argv)




