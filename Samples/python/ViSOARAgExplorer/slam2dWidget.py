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
from Slam.Slam2D                   	  import Slam2D


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
