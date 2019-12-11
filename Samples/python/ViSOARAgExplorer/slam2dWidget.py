import sys, os

import xml.etree.ElementTree as ET
from functools import partial

from OpenVisus       import *

from VisusGuiPy                       import *
from VisusAppKitPy                    import *
from OpenVisus.PyUtils                import *

from Slam.GuiUtils                    import *
from Slam.GoogleMaps                  import *
from Slam.ImageProvider               import *
from Slam.ExtractKeyPoints            import *
from Slam.FindMatches                 import *
from Slam.GuiUtils                    import *
#from Slam.Slam2D                   	  import Slam2D


from PyQt5.QtGui 					  import QFont
from PyQt5.QtCore                     import QUrl, Qt, QSize, QDir, QRect
from PyQt5.QtWidgets                  import QApplication, QHBoxLayout, QLineEdit,QLabel, QLineEdit, QTextEdit, QGridLayout
from PyQt5.QtWidgets                  import QMainWindow, QPushButton, QVBoxLayout,QSplashScreen,QProxyStyle, QStyle, QAbstractButton
from PyQt5.QtWidgets                  import QWidget
from PyQt5.QtWebEngineWidgets         import QWebEngineView	
from PyQt5.QtWidgets                  import QTableWidget,QTableWidgetItem

from OpenVisus.VisusGuiPy      import *
from OpenVisus.VisusGuiNodesPy import *
from OpenVisus.VisusAppKitPy   import *

from OpenVisus.PyViewer        import *


from analysis_scripts			import *
from lookAndFeel  				import *



#/////////////// COPY SLAM for editing ////////////////////////



# ///////////////////////////////////////////////////////////////////
def ComposeImage(v, axis):
		H = [single.shape[0] for single in v]
		W = [single.shape[1] for single in v]
		W,H=[(sum(W),max(H)),(max(W), sum(H))][axis]
		shape=list(v[0].shape)
		shape[0],shape[1]=H,W
		ret=numpy.zeros(shape=shape,dtype=v[0].dtype)
		cur=[0,0]
		for single in v:
			H,W=single.shape[0],single.shape[1]
			ret[cur[1]:cur[1]+H,cur[0]:cur[0]+W,:]=single
			cur[axis]+=[W,H][axis]
		return ret

# ///////////////////////////////////////////////////////////////////
class Slam2D(Slam):

	# constructor
	def __init__(self,width,height,dtype,calibration,cache_dir):
		super(Slam2D,self).__init__()

		self.width              = width
		self.height             = height
		self.dtype              = dtype
		self.calibration        = calibration
		self.cache_dir          = cache_dir

		self.debug_mode         = False # True
		self.energy_size        = 1280 
		self.min_num_keypoints  = 3000
		self.max_num_keypoints  = 6000
		self.anms               = 1300
		self.max_reproj_error   = 0.01 
		self.ratio_check        = 0.8
		self.calibration.bFixed = False 
		self.ba_tolerance       = 0.005

		self.images             = []
		self.extractor          = None

	# addCamera
	def addCamera(self,img):
		self.images.append(img)
		camera=Camera()
		camera.id=len(self.cameras)
		camera.color = Color.random()
		for filename in img.filenames:
			camera.filenames.append(filename)
		super().addCamera(camera)
		return camera

	# saveIdx
	def saveIdx(self,camera):
		idxfile = IdxFile()
		field=Field("myfield", self.dtype)
		field.default_layout = "rowmajor"
		idxfile.fields.push_back(field)
		idxfile.logic_box = BoxNi(PointNi(0,0), PointNi(self.width, self.height))
		idxfile.blocksperfile = -1 # one file per dataset
		idxfile.filename_template="./%04d.bin" % (camera.id,)
		camera.idx_filename="./idx/%04d.idx" % (camera.id,)
		idxfile.save(self.cache_dir + "/" + camera.idx_filename)

	# generateImage
	def generateImage(self,img):
		raise Exception("to implement")

	# startAction
	def startAction(self,N,message):
		pass

	# advanceAction
	def advanceAction(self,I):
		pass

	# endAction
	def endAction(self):
		pass

	# showEnergy
	def showEnergy(self,camera,energy):
		pass

	# guessLocalCameras
	def guessLocalCameras(self):

		box = self.getQuadsBox()

		x1i = math.floor(box.p1[0]); x2i = math.ceil(box.p2[0])
		y1i = math.floor(box.p1[1]); y2i = math.ceil(box.p2[1])
		rect=(x1i, y1i, (x2i - x1i), (y2i - y1i))

		subdiv=cv2.Subdiv2D(rect)

		find_camera=dict()
		for camera in self.cameras:
			center = camera.quad.centroid()
			center = (numpy.float32(center.x),numpy.float32(center.y))
			if center in find_camera:
				print("The following cameras seems to be in the same position: ",find_camera[center].id,camera.id)
			else:
				find_camera[center] = camera
				subdiv.insert(center)

		cells, centers=subdiv.getVoronoiFacetList([])
		assert(len(cells) == len(centers))

		# find edges
		edges=dict()
		for Cell in range(len(cells)):
			cell = cells[Cell]
			center = (numpy.float32(centers[Cell][0]), numpy.float32(centers[Cell][1]))

			camera = find_camera[center]

			for I in range(len(cell)):
				pt0 = cell[(I + 0) % len(cell)]
				pt1 = cell[(I + 1) % len(cell)]
				k0=(pt0[0], pt0[1], pt1[0], pt1[1]) 
				k1=(pt1[0], pt1[1], pt0[0], pt0[1]) 

				if not k0 in edges: edges[k0]=set()
				if not k1 in edges: edges[k1]=set()

				edges[k0].add(camera)
				edges[k1].add(camera)
				
		for k in edges:
			adjacent = tuple(edges[k])
			for A in range(0,len(adjacent)-1):
				for B in range(A+1,len(adjacent)):
					camera1 = adjacent[A]
					camera2 = adjacent[B]
					camera1.addLocalCamera(camera2)

		# insert prev and next
		N=self.cameras.size()
		for I in range(N):
			camera2 = self.cameras[I]

			# insert prev and next
			if (I-1) >= 0:
				camera2.addLocalCamera(self.cameras[I - 1])

			if (I+1) < N:
				camera2.addLocalCamera(self.cameras[I + 1])

		#enlarge a little 
		if True:

			new_local_cameras={}

			for camera1 in self.cameras:

				new_local_cameras[camera1]=set()

				for camera2 in camera1.getAllLocalCameras():

					# euristic to say: do not take cameras on the same drone flight "row"
					prev2=self.previousCamera(camera2)
					next2=self.nextCamera(camera2)
					if prev2!=camera1 and next2!=camera1:
						if prev2: new_local_cameras[camera1].add(prev2)
						if next2: new_local_cameras[camera1].add(next2)

			for camera1 in new_local_cameras:
				for camera3 in new_local_cameras[camera1]:
					camera1.addLocalCamera(camera3)
					
		# draw the image
		if True:
			w = float(box.size()[0])
			h = float(box.size()[1])

			W = int(4096)
			H = int(h * (W/w))
			out = numpy.zeros((H, W, 3), dtype="uint8")
			out.fill(255)

			def toScreen(p):
				return [
					int(0+(p[0]-box.p1[0])*(W/w)),
					int(H-(p[1]-box.p1[1])*(H/h))]

			for I in range(len(cells)):
				cell=cells[I]
				center=(numpy.float32(centers[I][0]), numpy.float32(centers[I][1]))
				camera2 = find_camera[center]
				center=toScreen(center)
				cell=numpy.array([toScreen(it) for it in cell],dtype=numpy.int32)
				cv2.fillConvexPoly(out, cell, [random.randint(0,255) ,random.randint(0,255) ,random.randint(0,255) ])
				cv2.polylines(out, [cell], True, [0,0,0], 3)
				cv2.putText(out, str(camera2.id), (int(center[0]),int(center[1])), cv2.FONT_HERSHEY_COMPLEX_SMALL, 1.0, [0,0,0])

			SaveImage(self.cache_dir+"/~local_cameras.png", out)

	# guessInitialPoses
	def guessInitialPoses(self):
		lat0,lon0=self.images[0].lat, self.images[0].lon
		for I,camera in enumerate(self.cameras):
			lat,lon,alt=self.images[I].lat, self.images[I].lon,self.images[I].alt
			x,y=GPSUtils.gpsToLocalCartesian(lat,lon,lat0,lon0)
			world_center=Point3d(x,y,alt)
			img=self.images[I]
			q = Quaternion(Point3d(0, 0, 1), -img.yaw) *  Quaternion(Point3d(1, 0, 0), math.pi)
			camera.pose = Pose(q, world_center).inverse()

	# saveMidx
	def saveMidx(self):

		url=self.cache_dir+"/visus.midx"

		lat0,lon0=self.images[0].lat,self.images[0].lon

		logic_box = self.getQuadsBox()

		# instead of working in range -180,+180 -90,+90 (worldwise ref frame) I normalize the range in 0,1 0,1
		physic_box=BoxNd.invalid()
		for I, camera in enumerate(self.cameras):
			quad=self.computeWorldQuad(camera)
			for point in quad.points:
				lat,lon=GPSUtils.localCartesianToGps(point.x, point.y,lat0, lon0)
				alpha,beta=GPSUtils.gpsToUnit(lat,lon)
				physic_box.addPoint(PointNd(Point2d(alpha,beta)))

		logic_centers=[]
		for I, camera in enumerate(self.cameras):
			p=camera.getWorldCenter()
			lat,lon,alt=*GPSUtils.localCartesianToGps(p.x, p.y, lat0, lon0),p.z
			alpha,beta=GPSUtils.gpsToUnit(lat,lon)
			alpha=(alpha-physic_box.p1[0])/float(physic_box.size()[0])
			beta =(beta -physic_box.p1[1])/float(physic_box.size()[1])
			logic_x=logic_box.p1[0]+alpha*logic_box.size()[0]
			logic_y=logic_box.p1[1]+beta *logic_box.size()[1]
			logic_centers.append((logic_x,logic_y))

		lines=[]

		lines.append("<dataset typename='IdxMultipleDataset' logic_box='%s %s %s %s' physic_box='%s %s %s %s'>" % (
			cstring(int(logic_box.p1[0])),cstring(int(logic_box.p2[0])),cstring(int(logic_box.p1[1])),cstring(int(logic_box.p2[1])),
			cstring10(physic_box.p1[0]),cstring10(physic_box.p2[0]),cstring10(physic_box.p1[1]),cstring10(physic_box.p2[1])))
		lines.append("")
		lines.append("<slam width='%s' height='%s' dtype='%s' calibration='%s %s %s' />" % (
			cstring(self.width),cstring(self.height),self.dtype.toString(),
			cstring(self.calibration.f),cstring(self.calibration.cx),cstring(self.calibration.cy)))
		lines.append("")
		lines.append("<field name='voronoi'><code>output=voronoi()</code></field>")
		lines.append("")
		lines.append("<translate x='%s' y='%s'>" % (cstring10(physic_box.p1[0]),cstring10(physic_box.p1[1])))
		lines.append("<scale     x='%s' y='%s'>" % (cstring10(physic_box.size()[0]/logic_box.size()[0]),cstring10(physic_box.size()[1]/logic_box.size()[1])))
		lines.append("<translate x='%s' y='%s'>" % (cstring10(-logic_box.p1[0]),cstring10(-logic_box.p1[1])))
		lines.append("")

		#Amy: Comment out extras, turn this if to True if you want them drawn
		if False:
			W=int(1024)
			H=int(W*(logic_box.size()[1]/float(logic_box.size()[0])))

			lines.append("<svg width='%s' height='%s' viewBox='%s %s %s %s' >" % (
				cstring(W),
				cstring(H),
				cstring(int(logic_box.p1[0])),
				cstring(int(logic_box.p1[1])),
				cstring(int(logic_box.p2[0])),
				cstring(int(logic_box.p2[1]))))

			lines.append("\t<g stroke='#000000' stroke-width='1' fill='#ffff00' fill-opacity='0.3'>")
			for I, camera in enumerate(self.cameras):
				lines.append("\t\t<poi point='%s,%s' />" % (cstring(logic_centers[I][0]),cstring(logic_centers[I][1])))
			lines.append("\t</g>")

			lines.append("\t<g fill-opacity='0.0' stroke-opacity='0.5' stroke-width='2'>")
			for I, camera in enumerate(self.cameras):
				lines.append("\t\t<polygon points='%s' stroke='%s' />" % (camera.quad.toString(","," "),camera.color.toString()[0:7]))
			lines.append("\t</g>")

			lines.append("</svg>")
			lines.append("")

		for I, camera in enumerate(self.cameras):
			p=camera.getWorldCenter()
			lat,lon,alt=*GPSUtils.localCartesianToGps(p.x, p.y, lat0, lon0),p.z
			lines.append("\t<dataset url='%s' color='%s' quad='%s' filenames='%s' q='%s' t='%s' lat='%s' lon='%s' alt='%s' />" %(
				camera.idx_filename,
				camera.color.toString(),
				camera.quad.toString(),
				";".join(camera.filenames),
				camera.pose.q.toString(),
				camera.pose.t.toString(),
				cstring10(lat),cstring10(lon),cstring10(alt)))
			
		lines.append("")
		lines.append("</translate>")
		lines.append("</scale>")
		lines.append("</translate>")
		lines.append("")
		lines.append("</dataset>")

		SaveTextDocument(url,"\n".join(lines) )

	# debugMatchesGraph
	def debugMatchesGraph(self):

		box = self.getQuadsBox()

		w = float(box.size()[0])
		h = float(box.size()[1])

		W = int(4096)
		H = int(h * (W/w))
		out = numpy.zeros((H, W, 4), dtype="uint8")
		out.fill(255)

		def getImageCenter(camera):
			p=camera.quad.centroid()
			return (
				int(0+(p[0]-box.p1[0])*(W/w)),
				int(H-(p[1]-box.p1[1])*(H/h)))

		for bGoodMatches in [False,True]:
			for A in self.cameras :
				local_cameras=A.getAllLocalCameras()
				for J in range(local_cameras.size()):
					B=local_cameras[J]
					edge=A.getEdge(B)
					if A.id < B.id and bGoodMatches == (True if edge.isGood() else False):
						p0 = getImageCenter(A)
						p1 = getImageCenter(B)
						color = [255,0,0, 255] if edge.isGood() else [211,211,211, 255]
						cv2.line(out, p0, p1, color, 1)
						num_matches = edge.matches.size()
						if num_matches>0:
							cx=int(0.5*(p0[0]+p1[0]))
							cy=int(0.5*(p0[1]+p1[1]))
							cv2.putText(out, str(num_matches), (cx,cy), cv2.FONT_HERSHEY_COMPLEX_SMALL, 1.0, color)

		for camera in self.cameras :
			center=getImageCenter(camera)
			cv2.putText(out, str(camera.id), (center[0],center[1]), cv2.FONT_HERSHEY_COMPLEX_SMALL, 1.0, [0,0,0,255])
			
		SaveImage(GuessUniqueFilename(self.cache_dir+"/~matches%d.png"), out)

	# debugSolution
	def debugSolution(self):

		box = self.getQuadsBox()

		w = float(box.size()[0])
		h = float(box.size()[1])

		W = int(4096)
		H = int(h * (W/w))
		out = numpy.zeros((H, W, 4), dtype="uint8")
		out.fill(255)

		def toScreen(p):
			return (
				int(0+(p[0]-box.p1[0])*(W/w)),
				int(H-(p[1]-box.p1[1])*(H/h)))

		for camera2 in self.cameras:
			color=(int(255*camera2.color.getRed()),int(255*camera2.color.getGreen()),int(255*camera2.color.getBlue()),255)
			points=numpy.array([toScreen(it) for it in camera2.quad.points],dtype=numpy.int32)
			cv2.polylines(out, [points], True, color, 3)
			cv2.putText(out, str(camera2.id), toScreen(camera2.quad.points[0]), cv2.FONT_HERSHEY_COMPLEX_SMALL, 1.0, color)

		SaveImage(GuessUniqueFilename(self.cache_dir+"/~solution%d.png"), out)

	# doPostIterationAction
	def doPostIterationAction(self):
		self.debugSolution()
		self.debugMatchesGraph()

	# extractKeyPoints
	def extractKeyPoints(self):

		t1=Time.now()

		# convert to idx and find keypoints (don't use threads for IO ! it will slow down things)
		# NOTE I'm disabling write-locks
		self.startAction(len(self.cameras),"Converting idx and extracting keypoints...")

		if not self.extractor:
			self.extractor=ExtractKeyPoints(self.min_num_keypoints,self.max_num_keypoints,self.anms)

		for I,(img,camera) in enumerate(zip(self.images,self.cameras)):
			self.advanceAction(I)

			# create idx and extract keypoints
			keypoint_filename = self.cache_dir+"/keypoints/%04d" % (camera.id,)
			idx_filename      = self.cache_dir+"/" + camera.idx_filename

			if not self.loadKeyPoints(camera,keypoint_filename) or not os.path.isfile(idx_filename):
				full = self.generateImage(img)
				Assert(isinstance(full, numpy.ndarray))

				dataset = LoadDataset(idx_filename)
				dataset.writeFullResolutionData(dataset.createAccess(), dataset.getDefaultField(), dataset.getDefaultTime(), Array.fromNumPy(full,TargetDim=2), dataset.getLogicBox())
				dataset.compressDataset("lz4") # lz4 creates files too big (ratio 0.90 vs 0.75) but zip seems a little slow (need to do some speed test)

				energy=ConvertImageToGrayScale(full)
				energy=ResizeImage(energy, self.energy_size)
				(keypoints,descriptors)=self.extractor.doExtract(energy)

				vs=self.width  / float(energy.shape[1])
				if keypoints:
					camera.keypoints.clear()
					camera.keypoints.reserve(len(keypoints))
					for keypoint in keypoints:
						camera.keypoints.push_back(KeyPoint(vs*keypoint.pt[0], vs*keypoint.pt[1], keypoint.size, keypoint.angle, keypoint.response, keypoint.octave, keypoint.class_id))
					camera.descriptors=Array.fromNumPy(descriptors,TargetDim=2) 

				self.saveKeyPoints(camera,keypoint_filename)

				energy=cv2.cvtColor(energy, cv2.COLOR_GRAY2RGB)
				for keypoint in keypoints:
					cv2.drawMarker(energy, (int(keypoint.pt[0]), int(keypoint.pt[1])), (0, 255, 255), cv2.MARKER_CROSS, 5)
				energy=cv2.flip(energy, 0)
				energy=ConvertImageToUint8(energy)

				if False:
					quad_box=camera.quad.getBoundingBox()
					VS = self.energy_size / max(quad_box.size()[0],quad_box.size()[1])
					T=Matrix.scale(2,VS) * camera.homography * Matrix.scale(2,vs)
					quad_box=Quad(T,Quad(energy.shape[1],energy.shape[0])).getBoundingBox()
					warped=cv2.warpPerspective(energy,  MatrixToNumPy(Matrix.translate(-quad_box.p1) * T),  (int(quad_box.size()[0]),int(quad_box.size()[1])))
					energy=ComposeImage([warped,energy],1)

				self.showEnergy(camera,energy)
					

			print("Done",camera.filenames[0],I,"of",len(self.cameras))

		print("done in",t1.elapsedMsec(),"msec")

	# findMatches
	def findMatches(self,camera1,camera2):

		if camera1.keypoints.empty() or camera2.keypoints.empty():
			camera2.getEdge(camera1).setMatches([],"No keypoints")
			return 0

		matches,H21,err=FindMatches(self.width,self.height,
			camera1.id,[(k.x, k.y) for k in camera1.keypoints],Array.toNumPy(camera1.descriptors), 
			camera2.id,[(k.x, k.y) for k in camera2.keypoints],Array.toNumPy(camera2.descriptors),
			self.max_reproj_error * self.width, self.ratio_check)

		if self.debug_mode and H21 is not None and len(matches)>0:
			points1=[(k.x, k.y) for k in camera1.keypoints]
			points2=[(k.x, k.y) for k in camera2.keypoints]
			DebugMatches(self.cache_dir+"/debug_matches/%s/%04d.%04d.%d.png" %(err if err else "good",camera1.id,camera2.id,len(matches)), 
				self.width, self.height, 
				Array.toNumPy(ArrayUtils.loadImage(self.cache_dir+"/energy/~%04d.tif" % (camera1.id,))), [points1[match.queryIdx] for match in matches], H21, 
				Array.toNumPy(ArrayUtils.loadImage(self.cache_dir+"/energy/~%04d.tif" % (camera2.id,))), [points2[match.trainIdx] for match in matches], numpy.identity(3,dtype='float32'))

		if err:
			camera2.getEdge(camera1).setMatches([],err)
			return 0

		matches=[Match(match.queryIdx,match.trainIdx, match.imgIdx, match.distance) for match in matches]
		camera2.getEdge(camera1).setMatches(matches,str(len(matches)))
		return len(matches)

	# findAllMatches
	def findAllMatches(self,nthreads=8):
		t1 = Time.now()
		jobs=[]
		for camera2 in self.cameras:
			for camera1 in camera2.getAllLocalCameras():
				if camera1.id < camera2.id:
					jobs.append(lambda pair=(camera1,camera2): self.findMatches(pair[0],pair[1]))
		self.startAction(len(jobs),"Finding all matches")
		results=RunJobsInParallel(jobs,advance_callback=lambda ndone: self.advanceAction(ndone))
		num_matches=sum(results)
		print("Found num_matches(", num_matches, ") matches in ", t1.elapsedMsec() ,"msec")

	# initialSetup
	def initialSetup(self):
		self.guessInitialPoses()
		self.refreshQuads()
		self.saveMidx()
		self.guessLocalCameras()
		self.debugMatchesGraph()
		self.debugSolution()

	# refineSolution
	def refineSolution(self):

		if self.cameras[0].keypoints.size()==0:
			self.extractKeyPoints()
			self.findAllMatches()
			self.removeDisconnectedCameras()
			self.debugMatchesGraph()

		tolerances=(10.0*self.ba_tolerance,1.0*self.ba_tolerance)
		self.startAction(len(tolerances),"Refining solution...")
		for I,tolerance in enumerate(tolerances):
			self.advanceAction(I)
			self.bundleAdjustment(tolerance)
			self.removeOutlierMatches(self.max_reproj_error * self.width)
			self.removeDisconnectedCameras()
			self.removeCamerasWithTooMuchSkew()
		self.endAction()

		self.saveMidx()
		print("Finished")



# //////////////////////////////////////////////////////////////////////////////

class Logger(QtCore.QObject):

        """Redirects console output to text widget."""
        my_signal = QtCore.pyqtSignal(str)

        # constructor
        def __init__(self, terminal=None, filename="", qt_callback=None):
                super().__init__()
                self.terminal=terminal
                self.log=open(filename,'w')
                self.my_signal.connect(qt_callback)

        # write
        def write(self, message):
                message=message.replace("\n", "\n" + str(datetime.datetime.now())[0:-7] + " ")
                self.terminal.write(message)
                self.log.write(message)
                self.my_signal.emit(str(message))

        # flush
        def flush(self):
                self.terminal.flush()
                self.log.flush()


# ////////////////////////////////////////////////////////////////////////////////////////////
class ExceptionHandler(QtCore.QObject):

        # __init__
        def __init__(self):
                super(ExceptionHandler, self).__init__()
                sys.__excepthook__ = sys.excepthook
                sys.excepthook = self.handler

        # handler
        def handler(self, exctype, value, traceback):
                sys.stdout=sys.__stdout__
                sys.stderr=sys.__stderr__
                sys.excepthook=sys.__excepthook__
                sys.excepthook(exctype, value, traceback)


# //////////////////////////////////////////////////////////////////////////////

		 
class Slam2DWidget(QWidget):
	
	# constructor
	def __init__(self, parent):
		super(QWidget, self).__init__(parent)
		self.image_directory=""
		self.cache_dir=""
		self.createGui()

		self.setLookAndFeel()

	def setLookAndFeel(self):
		self.setStyleSheet(LOOK_AND_FEEL)

	# createGui
	def createGui(self):

		#self.setWindowTitle("Visus SLAM") #AAG: REMOVED
		self.layout = QVBoxLayout(self)    #AAG: Added
		class Buttons : pass
		self.buttons=Buttons
		
		# create widgets
		self.viewer=Viewer()
		self.viewer.setMinimal()
		viewer_subwin = sip.wrapinstance(FromCppQtWidget(self.viewer.c_ptr()), QtWidgets.QMainWindow)	
		
		self.google_maps = QWebEngineView()
		self.progress_bar=ProgressLine()
		self.preview=PreviewImage()

		self.log = QTextEdit()
		self.log.setLineWrapMode(QTextEdit.NoWrap)
		
		p = self.log.viewport().palette()
		p.setColor(QPalette.Base, QtGui.QColor(200,200,200))
		p.setColor(QPalette.Text, QtGui.QColor(0,0,0))
		self.log.viewport().setPalette(p)
		
		main_layout=QVBoxLayout()
		
		# toolbar
		toolbar=QHBoxLayout()
		# self.buttons.load_midx=GuiUtils.createPushButton("Load Prev Solution",
		# 	lambda: self.loadPrevSolution())

		# self.buttons.run_slam=GuiUtils.createPushButton("Stitch it!",
		# 	lambda: self.run())
		# self.buttons.goToAnalytics=GuiUtils.createPushButton("Analytics",
		# 	lambda: self.goToAnalyticsTab())
		
		# self.buttons.run_slam.setStyleSheet(GREEN_PUSH_BUTTON)
		# self.buttons.goToAnalytics.setStyleSheet(DISABLED_PUSH_BUTTON)
		# self.buttons.goToAnalytics.setEnabled(False)

		# self.buttons.show_ndvi=GuiUtils.createPushButton("NDVI",
		# 	lambda: self.showNDVI())

		# self.buttons.show_tgi=GuiUtils.createPushButton("TGI",
		# 	lambda: self.showTGI())
				
		# self.buttons.show_rgb=GuiUtils.createPushButton("RGB",
		# 	lambda: self.showRGB())
				
#		toolbar.addWidget(self.buttons.load_midx)
		# toolbar.addWidget(self.buttons.run_slam)
		# toolbar.addWidget(self.buttons.goToAnalytics)
		# toolbar.addWidget(self.buttons.show_ndvi)
		# toolbar.addWidget(self.buttons.show_tgi)
		# toolbar.addWidget(self.buttons.show_rgb)
		toolbar.addLayout(self.progress_bar)

		toolbar.addStretch(1)
		main_layout.addLayout(toolbar)
		
		center = QSplitter(QtCore.Qt.Horizontal)
		center.addWidget(self.google_maps)
		center.addWidget(viewer_subwin)
		center.setSizes([100,200])
		
		main_layout.addWidget(center,1)
		main_layout.addWidget(self.log)

		self.layout.addLayout(main_layout)   #AAG: Added

		#AAG: REMOVED
		# central_widget = QFrame()
		# central_widget.setLayout(main_layout)
		# central_widget.setFrameShape(QFrame.NoFrame)
		# self.setCentralWidget(central_widget)



	# processEvents
	def processEvents(self):
		QApplication.processEvents()
		time.sleep(0.00001)

	# printLog
	def printLog(self,text):
		self.log.moveCursor(QtGui.QTextCursor.End)
		self.log.insertPlainText(text)
		self.log.moveCursor(QtGui.QTextCursor.End)	
		if hasattr(self,"__print_log__") and self.__print_log__.elapsedMsec()<200: return
		self.__print_log__=Time.now()
		self.processEvents()

	# startAction
	def startAction(self,N,message):
		print(message)
		self.progress_bar.setRange(0,N)
		self.progress_bar.setMessage(message)
		self.progress_bar.setValue(0)
		self.progress_bar.show()
		self.processEvents()

	# advanceAction
	def advanceAction(self,I):
		self.progress_bar.setValue(max(I,self.progress_bar.value()))
		self.processEvents()

	# endAction
	def endAction(self):
		self.progress_bar.hide()
		self.processEvents()

	# showMessageBox
	def showMessageBox(self,msg):
		print(msg)
		QMessageBox.information(self, 'Information', msg)

	# chooseDirectory
	def chooseDirectory(self):
		print("Showing choose directory dialog")
		value = QFileDialog.getExistingDirectory(self, "Choose directory...","",QFileDialog.ShowDirsOnly) 
		if not value: return
		self.setCurrentDir(value)	

	# generateImage
	def generateImage(self,img):
		t1=Time.now()
		print("Generating image",img.filenames[0])	
		ret = InterleaveChannels(self.provider.generateImage(img))
		print("done",img.id,"range",ComputeImageRange(ret),"shape",ret.shape, "dtype",ret.dtype,"in",t1.elapsedMsec()/1000,"msec")
		return ret

	# showEnergy
	def showEnergy(self,camera,energy):

		if self.slam.debug_mode:
			SaveImage(self.cache_dir+"/generated/%04d.%d.tif" % (camera.id,camera.keypoints.size()), energy)

		self.preview.showPreview(energy,"Extracting keypoints image(%d/%d) #keypoints(%d)" % (camera.id,len(self.provider.images),camera.keypoints.size()))
		self.processEvents()

	# setCurrentDir
	def setCurrentDir(self,image_dir):
		
		# avoid recursions
		if self.image_directory==image_dir:
			return
			
		self.image_directory=image_dir
		
		self.log.clear()
		Assert(os.path.isdir(image_dir))
		os.chdir(image_dir)
		self.cache_dir=os.path.abspath("./VisusSlamFiles")
		self.provider=CreateProvider(self.cache_dir,self.progress_bar)
		
		os.makedirs(self.cache_dir,exist_ok=True)
		TryRemoveFiles(self.cache_dir+'/~*')

		full=self.generateImage(self.provider.images[0])
		array=Array.fromNumPy(full,TargetDim=2)
		width  = array.getWidth()
		height = array.getHeight()
		dtype  = array.dtype

		self.slam=Slam2D(width,height,dtype, self.provider.calibration,self.cache_dir)
		self.slam.debug_mode=False
		self.slam.generateImage=self.generateImage
		self.slam.startAction=self.startAction
		self.slam.advanceAction=self.advanceAction
		self.slam.endAction=self.endAction
		self.slam.showEnergy=self.showEnergy

		for img in self.provider.images:
			camera=self.slam.addCamera(img)
			self.slam.saveIdx(camera)

		self.slam.initialSetup()
		self.refreshGoogleMaps()
		self.refreshViewer()

		#AAG: removed
		#self.setWindowTitle("%s num_images(%d) width(%d) height(%d) dtype(%s) " % (
		#	self.image_directory, len(self.provider.images),self.slam.width, self.slam.height, self.slam.dtype.toString()))

	# refreshViewer
	def refreshViewer(self,fieldname="output=ArrayUtils.interleave(ArrayUtils.split(voronoi())[0:3])"):
		url=self.cache_dir+"/visus.midx"
		self.viewer.open(url)
		# make sure the RenderNode get almost RGB components
		self.viewer.setFieldName(fieldname)		

	# showNDVI
	def showNDVI(self):
		fieldname="output=ArrayUtils.interleave(ArrayUtils.split(voronoi())[0:3])"
		print("Showing NDVI for Red and IR channels")
		url=self.cache_dir+"/visus.midx"
		self.viewer.open(url)
		# make sure the RenderNode get almost RGB components
		self.viewer.setFieldName(fieldname)		

		# for Amy: example about processing
		#if False:
		self.viewer.setScriptingCode(NDVI_SCRIPT);

	# showTGI (for RGB datasets)
	def showTGI(self):
		fieldname="output=ArrayUtils.interleave(ArrayUtils.split(voronoi())[0:3])"
		print("Showing TGI for RGB images")
		print("Showing NDVI for Red and IR channels")
		url=self.cache_dir+"/visus.midx"
		self.viewer.open(url)
		# make sure the RenderNode get almost RGB components
		self.viewer.setFieldName(fieldname)		

		# for Amy: example about processing
		#if False:
		self.viewer.setScriptingCode(TGI_script)	


	# Old Show TGI
	def showTGI2(self):
		fieldname="output=ArrayUtils.interleave(ArrayUtils.split(voronoi())[0:3])"
		print("Showing TGI for RGB images")
		print("Showing NDVI for Red and IR channels")
		url=self.cache_dir+"/visus.midx"
		self.viewer.open(url)
		# make sure the RenderNode get almost RGB components
		self.viewer.setFieldName(fieldname)		

		# for Amy: example about processing
		#if False:
		self.viewer.setScriptingCode(TGI_OLD)	

	def loadPrevSolution(self):
		fieldname="output=ArrayUtils.interleave(ArrayUtils.split(voronoi())[0:3])"
		print("Showing img src")
		url=self.cache_dir+"/visus.midx"
		self.viewer.open(url)
		# make sure the RenderNode get almost RGB components
		self.viewer.setFieldName(fieldname)		

		# for Amy: example about processing
		#if False:
		self.viewer.setScriptingCode(
"""
output=input

""");

	def showRGB(self):
		fieldname="output=ArrayUtils.interleave(ArrayUtils.split(voronoi())[0:3])"
		print("Showing img src")
		url=self.cache_dir+"/visus.midx"
		self.viewer.open(url)
		# make sure the RenderNode get almost RGB components
		self.viewer.setFieldName(fieldname)		

		# for Amy: example about processing
		#if False:
		self.viewer.setScriptingCode(
"""
output=input

""");

	# refreshGoogleMaps
	def refreshGoogleMaps(self):
	
		images=self.slam.images
		if not images:
			return
			
		maps=GoogleMaps()
		maps.addPolyline([(img.lat,img.lon) for img in images],strokeColor="#FF0000")
		
		for I,img in enumerate(images):
			maps.addMarker(img.filenames[0], img.lat, img.lon, color="green" if I==0 else ("red" if I==len(images)-1 else "blue"))
			dx=math.cos(img.yaw)*0.00015
			dy=math.sin(img.yaw)*0.00015
			maps.addPolyline([(img.lat, img.lon),(img.lat + dx, img.lon + dy)],strokeColor="yellow")

		content=maps.generateHtml()
		
		filename=os.path.join(os.getcwd(),self.cache_dir+"/slam.html")
		SaveTextDocument(filename,content)
		self.google_maps.load(QUrl.fromLocalFile(filename))	

	# run
	def run(self):
		self.slam.refineSolution()
		self.preview.hide()
		self.refreshViewer()
		



#///////////////////////////////////////////////////////////////////////////////		
