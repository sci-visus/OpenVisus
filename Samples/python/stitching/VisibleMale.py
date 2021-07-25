import os,sys,pickle,random,threading,time
import platform,subprocess,glob,datetime
import numpy as np
import cv2

from OpenVisus                        import *
from OpenVisus.gui                    import *
from OpenVisus.image_utils            import *

from slampy.extract_keypoints         import *
from slampy.gui_utils                 import *
from slampy.image_utils               import *
from slampy.image_provider            import *

# this are the pixel dimension as from files
Width,Height= 2048, 1216
filenames=sorted(glob.glob("D:/GoogleSci/visus_dataset/male/RAW/Fullcolor/fullbody/*.raw"))
Depth=len(filenames)
print("Found",len(filenames),"files")
print("Width",Width,"Height",Height,"Depth",Depth)

# I have a problem of 12 black slices, need to re-download? or is it a problem of the dataset
filenames[1423:1435]=[filenames[1422]]*12

# density is the scaling factor to show the dataset
# https://www.nlm.nih.gov/research/visible/visible_human.html#targetText=These%20images%2C%20each%20approximately%2032,all%201%2C871%20male%20color%20cryosections.&targetText=The%20Visible%20Human%20Female%20data,obtained%20at%200.33%20mm%20intervals.		
Density=[0.33, 0.33, 1.0]

# utility to load an image
def LoadRawImage(filename):
	
	print("Loading raw image",filename,"...")
	RGB=np.fromfile(filename, dtype=np.uint8)
	Assert(len(RGB)==Width*Height*3) 
	
	# original images are RRR...GGG...BBB
	RGB=RGB.reshape((3, Height, Width)) 
	
	# split channels
	R,G,B=RGB[0,:,:],RGB[1,:,:],RGB[2,:,:]
		
	return [R,G,B]

#remove color bands at the boundaries (very disturbing for visualization)
def GenerateAlphaRemovingBands(z):
		
	ret=np.zeros((Height,Width),dtype=np.uint8)

	def keepArea(x1,x2,y1,y2):
		ret[int(y1*Height):int(y2*Height),int(x1*Width):int(x2*Width)]=255
		
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
	return ret
	

# ///////////////////////////////////////////////////////////////////////////////////
def GenerateNonAlignedImages():
	
	for z in range(Depth):
		
		print("Doing z",z)
		R,G,B,=LoadRawImage(filenames[z])
		A=GenerateAlphaRemovingBands(z)
		R[A==0],G[A==0],B[A==0]=0,0,0
		RGBA=InterleaveChannels([R,G,B,A])

		show_rgba=True
		if show_rgba:
			ShowImage(RGBA)
			cv2.waitKey(1) 
				
		yield RGBA


# ///////////////////////////////////////////////////////////////////////////////////
def GenerateAlignedImages():
		
	# where to store aligned images
	cache_dir="D:/tmp/datasets/male-aligned/aligned"
	os.makedirs(cache_dir,exist_ok=True)
		
	config={
		'energy_size': 1280,
		'min_num_keypoints':3000,
		'max_num_keypoints':6000,
		'anms':1300,
		'max_reproj_error':0.01, 
		'ratio_check':0.80,
		'ba_tolerance':0.005,
		'fov':60.0
	}		
	
	slam=Slam()
	slam.width   = Width
	slam.height  = Height
	slam.dtype   = DType.fromString("uint8[3]")
	
	# guessing calibration (NOTE: calibration is not used at all, at least for now)
	f=slam.width * (0.5 / math.tan(0.5* math.radians(config['fov'])))
	cx,cy=slam.width*0.5,slam.height*0.5
	slam.calibration = Calibration(f,cx,cy)
	slam.calibration.bFixed = False 
	

	# add cameras
	for id,filename in enumerate(filenames):
		camera=Camera()
		camera.id=id
		camera.color = Color.random()
		camera.filenames.append(filename)
		slam.addCamera(camera)
		
	# extract keypoints
	extractor=ExtractKeyPoints(config['min_num_keypoints'], config['max_num_keypoints'], config['anms'])
	
	for camera in slam.cameras:
		keypoint_filename = cache_dir+"/keypoints/%04d" % (camera.id,)
		
		# load cached kepoints
		if slam.loadKeyPoints(camera, keypoint_filename):
			print("Loading cached keypoints",camera.id)
		# generating keypoints
		else:
			print("Generating keypoints",camera.id)
			R,G,B = LoadRawImage(camera.filenames[0])
			A=GenerateAlphaRemovingBands(camera.id)
			R[A==0],G[A==0],B[A==0]=0,0,0
			RGB=InterleaveChannels([R,G,B])
			# a grayscaled image
			energy=(0.3 * R + 0.59 * G + 0.11 * B).astype(np.uint8)
			# detect keypoints on a downsampled energy
			energy=ResizeImage(energy, config['energy_size'])
			(keypoints,descriptors)=extractor.doExtract(energy)
			Assert(keypoints)
			# scaling factor from keypoints in the downscaled resolution to fullres
			vs=slam.width/float(energy.shape[1])

			# add keypoints to the camera
			camera.keypoints.clear()
			camera.keypoints.reserve(len(keypoints))
			for keypoint in keypoints:
				camera.keypoints.push_back(KeyPoint(vs*keypoint.pt[0], vs*keypoint.pt[1], keypoint.size, keypoint.angle, keypoint.response, keypoint.octave, keypoint.class_id))
			camera.descriptors=Array.fromNumPy(descriptors,TargetDim=2) 
			
			# for future use
			slam.saveKeyPoints(camera,keypoint_filename)

			# show keypoints
			show_kepoints=True
			if show_kepoints:
				energy=cv2.cvtColor(energy, cv2.COLOR_GRAY2RGB)
				for keypoint in keypoints:
					cv2.drawMarker(energy, (int(keypoint.pt[0]), int(keypoint.pt[1])), (255, 255, 0), cv2.MARKER_CROSS, 5)
				energy=ConvertImageToUint8(energy)
				SaveImage(cache_dir+"/energy/%04d.tif" % (camera.id,), energy)
				ShowImage(energy)
				cv2.waitKey(1) # just to show the image
				
	# add local images (two consecutive images are local)
	for I in range(len(slam.cameras)-1):
		camera1=slam.cameras[I+0]
		camera2=slam.cameras[I+1]
		camera1.addLocalCamera(camera2)
	
	# find image matches
	for camera2 in slam.cameras:
		for camera1 in camera2.getAllLocalCameras():
			if camera1.id < camera2.id:
				Assert(not camera1.keypoints.empty())
				Assert(not camera2.keypoints.empty())
				matches, H21, error_message=FindMatches(slam.width,slam.height,
					camera1.id,[(k.x, k.y) for k in camera1.keypoints],Array.toNumPy(camera1.descriptors), 
					camera2.id,[(k.x, k.y) for k in camera2.keypoints],Array.toNumPy(camera2.descriptors),
					config['max_reproj_error'] * slam.width, config['ratio_check'])
					
				if error_message:
					print("Error in finding matches",camera1.id,camera2.id,len(matches), error_message)
					camera2.getEdge(camera1).setMatches([],error_message)
					Assert(False) # should not happen
				else:
					print("OK matches",camera1.id,camera2.id,len(matches), "OK")
					matches=[Match(match.queryIdx,match.trainIdx, match.imgIdx, match.distance) for match in matches]
					camera2.getEdge(camera1).setMatches(matches,str(len(matches)))	
	
	# I should not have any disconnected camera
	# slam.removeDisconnectedCameras()
	
	# run bundle adjustment
	for I,tolerance in enumerate((10.0*config['ba_tolerance'],1.0*config['ba_tolerance'])):
		# offset is just moving the camera in XY plane. Do we need to add a rotation or even non-linear deformation?
		algorithm="offset"
		slam.bundleAdjustment(tolerance,algorithm)
		slam.removeOutlierMatches(config['max_reproj_error'])
		# I do not have disconnected cameras
		#slam.removeDisconnectedCameras()
		# skew should not happen
		#slam.removeCamerasWithTooMuchSkew()

	# produce aligned images 
	for camera in slam.cameras:
		
		# note: there are few slices that are totally black, going to use them as they are
		offset=(round(camera.homography(0,2)),round(camera.homography(1,2)))
		print("Bundle adjustment computed for", camera.id, "an offset of",offset)
		
		R,G,B=LoadRawImage(camera.filenames[0])
		A=GenerateAlphaRemovingBands(camera.id)
		R[A==0],G[A==0],B[A==0]=0,0,0
		x1,x2 = (0,Width)
		y1,y2 = (0,Height)
		X1,X2 = (max([x1+offset[0],0]),min([x2+offset[0],Width]))
		Y1,Y2 = (max([y1+offset[1],0]),min([y2+offset[1],Height]))
		x1,x2=(X1-offset[0],X2-offset[0])
		y1,y2=(Y1-offset[1],Y2-offset[1])
		RGBA=np.zeros((Height,Width,4),dtype='uint8')
		RGBA[Y1:Y2, X1:X2, 0]=R[y1:y2,x1:x2]
		RGBA[Y1:Y2, X1:X2, 1]=G[y1:y2,x1:x2]
		RGBA[Y1:Y2, X1:X2, 2]=B[y1:y2,x1:x2]
		RGBA[Y1:Y2, X1:X2, 3]=A[y1:y2,x1:x2]

		# save the aligned image
		filename=cache_dir+"/aligned/%04d.tif" % (camera.id,)
		# SaveImage(filename, RGBA)
		
		# wait 1 msec just to allow the image to appear
		show_rgba=True
		if show_rgba:
			ShowImage(RGBA)
			cv2.waitKey(1) 

		yield RGBA


# //////////////////////////////////////////////
if __name__ == '__main__':

	# filename="D:/tmp/datasets/male-non-aligned/visus.idx"	
	# generator=GenerateNonAlignedImages()

	filename="D:/tmp/datasets/male-aligned/visus.idx"
	generator=GenerateAlignedImages()
	
	# create dataset
	if False:
		bounds=BoxNd(PointNd(0,0,0),PointNd(Width*Density[0],Height*Density[1],Depth*Density[2]))
		db=CreateIdx(
				url=filename, 
				dim=3,
				dims=(Width,Height,Depth),
				blockperfile=-1,
				filename_template="./visus.bin",
				fields=[Field("data","uint8[4]","row_major")],
				bounds=Position(bounds))
		db.writeSlabs(generator)
		
	#example of full viewer
	if False:
		viewer=PyViewer()
		viewer.open(filename)
		viewer.setScriptingCode("""if True:
			import numpy as np
			from OpenVisus.image_utils import SplitChannels

			R,G,B,A=SplitChannels(input)
			A.fill(255)
			A[(R<76) | (G>R) | (B>R)]=0
			output=input
			output[...,3]=A
			""")
		viewer.run()
			
	# example of showing a region
	if True:
		
		regions={
			'flag_tatoo' : (0.04, 0.95, 0.05,0.73, 0.15,0.15+0.1)	,
			'head' : (0.35,0.65, 0.18,0.82, 0.0,0.2)	,
			'left_arm' : (0.0, 0.4, 0.1,0.8, 0.0,0.3),
			'full' : (0.00,1.00, 0.00,1.00, 0.00,1.00)
		}
		
		region=regions['flag_tatoo']
		db=LoadDataset(filename)
		logic_box=db.getLogicBox(x=region[0:2],y=region[2:4],z=region[4:6])
		data=db.read(logic_box,quality=-6)	
		bounds=db.getBounds(logic_box)

		# remove the bluish blob around the body
		R,G,B,A=SplitChannels(data)
		A[R<76]  =0
		A[R>76]=255
		A[G>R]=0
		A[B>R]=0

		viewer=PyViewer()
		viewer.addGLCamera("glcamera", viewer.getRoot(),"lookat")
		viewer.addVolumeRender(data, bounds)
		viewer.run()
			
	sys.exit(0)	







