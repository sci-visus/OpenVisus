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

# IMPORTANT for WIndows
# Mixing C++ Qt5 and PyQt5 won't work in Windows/DEBUG mode
# because forcing the use of PyQt5 means to use only release libraries (example: Qt5Core.dll)
# but I'm in need of the missing debug version (example: Qt5Cored.dll)
# as you know, python (release) does not work with debugging versions, unless you recompile all from scratch

# on windows rememeber to INSTALL and CONFIGURE


class StartWindow(QMainWindow):
	def __init__(self):
		super().__init__()
		self.setWindowTitle('ViSOAR Ag Explorer Analytics Prototype')
		
		self.setMinimumSize(QSize(600, 600))  
		self.setStyleSheet("""
		font-family: Roboto;font-style: normal;font-size: 20pt; 
		background-color: #ffffff;
		color: #7A7A7A;
		QTabBar::tab:selected {
		background: #045951;
		}
		QMainWindow {
			#background-color: #7A7A7A;
			#color: #ffffff;
			background-color: #ffffff;
			color: #7A7A7A;
			
			}
			QLabel {
			background-color: #7A7A7A;
			color: #ffffff;
			}
		QToolTip {
			border: 1px solid #76797C;
			background-color: rgb(90, 102, 117);
			color: white;
			padding: 5px;
			opacity: 200;
		}
		QLabel {
			font: 20pt Roboto
		}
		QPushButton {
			border-radius: 7;
			border-style: outset; 
			border-width: 0px;
			color: #ffffff;
			background-color: #045951;
			padding: 10px;
		}
		QPushButton:pressed { 
			background-color:  #e6e6e6;

		}
		QLineEdit { background-color: #e6e6e6; border-color: #045951 }

		""")

		
		self.central_widget = QFrame()
		self.central_widget.setFrameShape(QFrame.NoFrame)

		self.tab_widget = MyTabWidget(self)
		self.setCentralWidget(self.tab_widget)

	def on_click(self):
		print("\n")
		for currentQTableWidgetItem in self.tabWidget.selectedItems():
			print(currentQTableWidgetItem.row(), currentQTableWidgetItem.column(), currentQTableWidgetItem.text())

	def onChange(self):
		QMessageBox.information(self,
			"Tab Index Changed!",
			"Current Tab Index: ");
		 
class MyTabWidget(QWidget):
	def __init__(self, parent):
		super(QWidget, self).__init__(parent)
		self.layout = QVBoxLayout(self)
		

		self.viewer=Viewer()
		#self.viewer.hide()
		self.viewer.setMinimal()
		

		class Buttons : pass
		self.buttons=Buttons

		self.logo = QPushButton('', self)
		self.logo.setStyleSheet("QPushButton {border-style: outset; border-width: 0px;color:#ffffff;}");
		self.logo.setIcon(QIcon('./icons/visoar_logo.png') )
		self.logo.setIconSize(QtCore.QSize(480, 214))

		#self.button_analytics_label.setIconSize(pixmap.rect().size())
		#self.button_analytics_label.setFixedSize(pixmap.rect().size())
		self.logo.setText('')

		#Initialize tab screen
		self.tabs = QTabWidget()
		self.tabNew = QWidget()
		self.tabLoad = QWidget()
		self.tabStitcher = QWidget()
		self.tabViewer = QWidget()
		self.tabs.resize(600,600)

		self.mySetTabStyle()
		self.tabNewUI()
		self.tabLoadUI()
		self.tabStitcherUI()
		self.tabViewerUI()

		# Add tabs
		self.tabs.addTab(self.tabNew,"New Project")
		self.tabs.addTab(self.tabLoad,"Load Project")
		self.tabs.addTab(self.tabStitcher,"Stitcher")
		self.tabs.addTab(self.tabViewer,"Viewer")
		self.tabs.currentChanged.connect(self.onTabChange) #changed!
		#self.tabs.setTabShape(QTabWidget.Triangular.)
		# Add layout of tabs to self
		self.layout.addWidget(self.tabs) #, row, 1,4)

		self.tabs.setCurrentIndex(0) 


	def mySetTabStyle(self):
		self.tabs.setStyleSheet  ( """
			/* Style the tab using the tab sub-control. Note that
		it reads QTabBar _not_ QTabWidget */
		QTabBar::tab {
			background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
			stop: 0 #E1E1E1, stop: 0.4 #DDDDDD,
			stop: 0.5 #D8D8D8, stop: 1.0 #D3D3D3);
			border: 2px solid #C4C4C3;
			border-bottom-color: #C2C7CB; /* same as the pane color */
			border-top-left-radius: 4px;
			border-top-right-radius: 4px;
			min-width: 200px;
			padding: 2px;
		}

		QTabBar::tab:selected {
			background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
			stop: 0 #045951, stop: 0.4 #045951,
			stop: 0.5 #034640, stop: 1.0 #045951);
			color: #ffffff;
		}
		QTabBar::tab:hover  {
			background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
			stop: 0 #07a294, stop: 0.4 #07a294,
			stop: 0.5 #045951, stop: 1.0 #07a294);
			color: #ffffff;
		}

		QTabBar::tab:selected {
			border-color: #9B9B9B;
			border-bottom-color: #C2C7CB; /* same as pane color */
		}

		QTabBar::tab:!selected {
			margin-top: 2px; /* make non-selected tabs look smaller */
		}""");

	def tabViewerUI(self):
		self.sublayoutTabViewer= QVBoxLayout(self)

		#Toolbar
		toolbar=QHBoxLayout()
		self.buttons.show_ndvi=GuiUtils.createPushButton("NDVI",
			lambda: self.showNDVI())

		self.buttons.show_tgi=GuiUtils.createPushButton("TGI",
			lambda: self.showTGI())
				
		self.buttons.show_rgb=GuiUtils.createPushButton("RGB",
			lambda: self.showRGB())
				
		toolbar.addWidget(self.buttons.show_ndvi)
		toolbar.addWidget(self.buttons.show_tgi)
		toolbar.addWidget(self.buttons.show_rgb)
		toolbar.addStretch(1)

		self.sublayoutTabViewer.addLayout(toolbar)


		#Viewer
		viewer_subwin = sip.wrapinstance(FromCppQtWidget(self.viewer.c_ptr()), QtWidgets.QMainWindow)	
		self.sublayoutTabViewer.addWidget(viewer_subwin )
		self.tabViewer.setLayout( self.sublayoutTabViewer)


			# showNDVI
	def showNDVI(self):
		fieldname="output=ArrayUtils.interleave(ArrayUtils.split(voronoi())[0:3])"
		print("Showing NDVI for Red and IR channels")
		print(self.projDir)
		#self.viewer.open(self.projDir + '/VisusSlamFiles/visus.midx' ) 
		# make sure the RenderNode get almost RGB components
		self.viewer.setFieldName(fieldname)		

		# for Amy: example about processing
		#if False:
		self.viewer.setScriptingCode(
"""
import matplotlib as mpl
import matplotlib.pyplot as plt
import matplotlib.image as mpimg
import cv2,numpy

#Convert ViSUS array to numpy
pdim=input.dims.getPointDim()
img=Array.toNumPy(input,bShareMem=True)

RED = img[:,:,0]   
green = img[:,:,1]
NIR = img[:,:,2]
 
NDVI_u = (NIR - RED) 
NDVI_d = (NIR + RED)
NDVI = NDVI_u / NDVI_d

NDVI = cv2.normalize(NDVI, None, alpha=0, beta=1, norm_type=cv2.NORM_MINMAX, dtype=cv2.CV_32F) #  normalize data [0,1]

NDVI =numpy.uint8(NDVI * 255)  #color map requires 8bit.. ugh, convert again

gray = NDVI
 
cdict=[(.2, .4,0), (.2, .4,0), (.94, .83, 0), (.286,.14,.008), (.56,.019,.019)]
cmap = mpl.colors.LinearSegmentedColormap.from_list(name='my_colormap',colors=cdict,N=1000)

out = cmap(gray)

output=Array.fromNumPy(out,TargetDim=pdim)

""");

	# showTGI (for RGB datasets)
	def showTGI(self):
		fieldname="output=ArrayUtils.interleave(ArrayUtils.split(voronoi())[0:3])"
		print("Showing TGI for RGB images")
		print("Showing NDVI for Red and IR channels")
		print(self.projDir)
		#url=self.projDir+"/VisusSlamFiles/visus.midx"
		#self.viewer.open(self.projDir + '/VisusSlamFiles/visus.midx' ) 
		# make sure the RenderNode get almost RGB components
		self.viewer.setFieldName(fieldname)		

		# for Amy: example about processing
		#if False:
		self.viewer.setScriptingCode(
"""
import matplotlib as mpl
import matplotlib.pyplot as plt
import matplotlib.image as mpimg
import cv2,numpy

#COnvert ViSUS array to numpy
pdim=input.dims.getPointDim()
img=Array.toNumPy(input,bShareMem=True)

red = img[:,:,0]   
green = img[:,:,1]
blue = img[:,:,2]

# #TGI – Triangular Greenness Index - RGB index for chlorophyll sensitivity. TGI index relies on reflectance values at visible wavelengths. It #is a fairly good proxy for chlorophyll content in areas of high leaf cover.
# #TGI = −0.5 * ((190 * (redData − greeData)) − (120*(redData − blueData)))
scaleRed  = (0.39 * red)
scaleBlue = (.61 * blue)
TGI =  green - scaleRed - scaleBlue
TGI = cv2.normalize(TGI, None, alpha=0, beta=1, norm_type=cv2.NORM_MINMAX, dtype=cv2.CV_32F) #  normalize data [0,1]

gray = TGI
 
#cdict=[(.2, .4,0), (.2, .4,0), (.94, .83, 0), (.286,.14,.008), (.56,.019,.019)]
cdict=[ (.56,.019,.019),  (.286,.14,.008), (.94, .83, 0),(.2, .4,0), (.2, .4,0)]
cmap = mpl.colors.LinearSegmentedColormap.from_list(name='my_colormap',colors=cdict,N=1000)

out = cmap(gray)

#out =  numpy.float32(out) 

output=Array.fromNumPy(out,TargetDim=pdim) 

""".strip())


	def showRGB(self):
		fieldname="output=ArrayUtils.interleave(ArrayUtils.split(voronoi())[0:3])"
		print("Showing img src")
		#self.viewer.open(self.projDir + '/VisusSlamFiles/visus.midx' ) 
		# make sure the RenderNode get almost RGB components
		self.viewer.setFieldName(fieldname)		

		# for Amy: example about processing
		#if False:
		self.viewer.setScriptingCode(
"""
output=input

""");

	def tabStitcherUI(self):
		# self.sublayoutTabViewer= QVBoxLayout(self)
		# viewer_subwin = sip.wrapinstance(FromCppQtWidget(self.viewer.c_ptr()), QtWidgets.QMainWindow)	
		# self.sublayoutTabViewer.addWidget(viewer_subwin )
		# self.tabViewer.setLayout( self.sublayoutTabViewer)
		pass


	def tabNewUI(self):
		#Create New Tab:
		self.sublayoutTabNew= QVBoxLayout(self)
		self.sublayoutTabNew.addWidget(self.logo)  #,row,0)
	
		self.sublayoutForm = QFormLayout()
		self.sublayoutTabNew.addLayout(self.sublayoutForm) #, row, 1,4)

		#Ask for Project Name
		self.projNameLabel = QLabel('New Project Name:')
		self.projNametextbox = QLineEdit(self)
		self.projNametextbox.move(20, 20)
		self.projNametextbox.resize(180,40)
		self.projNametextbox.setStyleSheet("padding:10px; background-color: #e6e6e6; color: rgb(0, 0, 0);  border: 2px solid #09cab8;")
		self.sublayoutForm.addRow(self.projNameLabel,self.projNametextbox )


		self.buttonAddImages = QPushButton('Add Images', self)
		self.buttonAddImages.resize(180,40)
		self.buttonAddImages.clicked.connect( self.addImages)
		self.buttonAddImages.setStyleSheet("""QPushButton {
			max-width:300px;
			border-radius: 7;
			border-style: outset; 
			border-width: 0px;
			color: #045951;
			background-color: #e6e6e6;
			padding: 10px;
		}
		QPushButton:pressed { 
			background-color:  #e6e6e6;

		}""")

		#Ability to change location
		self.projDir= os.getcwd()
		self.srcDir = os.getcwd()
		self.curDir = QLabel('Save Project To:')
		self.curDir2 = QLabel(self.projDir)
		self.curDir2.setStyleSheet("""font-family: Roboto;font-style: normal;font-size: 14pt; """)
		self.curDir.resize(280,40)
		self.buttonChangeDir = QPushButton('Change Project Location', self)
		self.buttonChangeDir.resize(180,40)
		self.buttonChangeDir.clicked.connect( self.getDirectoryLocation)
		self.buttonChangeDir.setStyleSheet("""QPushButton {
			max-width:300px;
			border-radius: 7;
			border-style: outset; 
			border-width: 0px;
			color: #045951;
			background-color: #e6e6e6;
			padding: 10px;
		}
		QPushButton:pressed { 
			background-color:  #e6e6e6;

		}""")
		self.spaceLabel = QLabel('')
		self.spaceLabel.resize(380,40)
		self.sublayoutForm.addRow(self.curDir,self.curDir2)
		self.sublayoutForm.addRow(self.buttonAddImages, self.buttonChangeDir )

		#Button that says: "Create Project"
		self.buttons.create_project = QPushButton('Create Project', self)
		self.buttons.create_project.move(20,80)
		self.buttons.create_project.resize(180,80) 
		self.buttons.create_project.setStyleSheet("""QPushButton {
			max-width:300px;
			border-radius: 7;
			border-style: outset; 
			border-width: 0px;
			color: #ffffff;
			background-color: #045951;
			padding-left: 40px;
			padding-right: 40px;
			padding-top: 10px;
			padding-bottom: 10px;
		}
		QPushButton:pressed { 
			background-color:  #e6e6e6;

		}""")
		self.spaceLabel2 = QLabel('')
		self.spaceLabel2.resize(380,40)
		self.sublayoutTabNew.addWidget(  self.buttons.create_project, alignment=Qt.AlignCenter    )

		# connect button to function on_click
		self.buttons.create_project.clicked.connect(  self.createProject)

		self.tabNew.setLayout( self.sublayoutTabNew)

		
	def tabLoadUI(self):
		self.sublayoutTabLoad= QVBoxLayout(self)
		self.sublayoutGrid = QGridLayout()
		self.sublayoutGrid.setSpacing(10)
		self.sublayoutGrid.setRowStretch(0, 6)
		self.sublayoutGrid.setRowStretch(1, 4)
		self.LoadFromFile()
		self.sublayoutTabLoad.addLayout(self.sublayoutGrid)

		self.tabLoad.setLayout( self.sublayoutTabLoad)
		self.tabLoad.setStyleSheet("""background-color: #045951""")

	#User has specified location for data and the project name, launch ViSUS SLAM
	def createProject(self):
		projName = self.projNametextbox.text()
		projDir = self.curDir2.text()
		print('Create Proj')
		print(projName)
		print(projDir)

		tree = ET.parse('userFileHistory.xml')
		print (tree.getroot())
		root = tree.getroot()

		#etree.SubElement(item, 'Date').text = '2014-01-01'
		element = ET.Element('project')
		ET.SubElement(element, 'projName').text = projName
		ET.SubElement(element, 'projDir').text =  projDir
		ET.SubElement(element, 'srcDir').text =  self.srcDir
		root.append(element)
		print(ET.tostring(element ))
		tree.write('userFileHistory.xml')

		self.startViSUSSLAM(projDir, self.srcDir)

	#If user changes the tab (from New to Load), then refresh to have new project
	def onTabChange(self):
		for i in reversed(range(self.sublayoutGrid.count())): 
			widgetToRemove = self.sublayoutGrid.itemAt(i).widget()
			if (widgetToRemove != None ):
				
				# remove it from the layout list
				self.sublayoutGrid.removeWidget(widgetToRemove)
				# remove it from the gui
			
				widgetToRemove.setParent(None)

		self.LoadFromFile()

	#User wants to load a project that has already been stitched (this is a hope that the midx files exists)
	def LoadFromFile(self):

		#Parse users history file, contains files they have loaded before
		tree = ET.ElementTree(file="userFileHistory.xml")
		print (tree.getroot())
		root = tree.getroot()
		x = 0
		y = 0
		width = 4

		for project in root.iterfind('project'):

			projName = project.find('projName').text
			projDir = project.find('projDir').text
			print(projName + " "+ projDir)
			
			sublayoutProj = QVBoxLayout()

			projMapButton = QToolButton(self)
			projMapButton.setToolButtonStyle(Qt.ToolButtonTextUnderIcon);
			projMapButton.setStyleSheet("background-color: #045951; QPushButton {background-color: #045951; border-style: outset; border-width: 0px;color:#ffffff;}");
			projMapButton.setIcon(QIcon('./icons/genericmap.png') )
			projMapButton.setIconSize(QtCore.QSize(180, 180))
			projMapButton.setText(projName)
			projMapButton.setStyleSheet("QToolButton{font-size: 20px;font-family: Roboto;color: rgb(38,56,76);background-color: rgb(255, 255, 255);}");
			projMapButton.resize(180,180)
			projMapButton.setSizePolicy( QSizePolicy.Preferred, QSizePolicy.Expanding)

			self.btnCallback = partial(self.triggerButton, projName)
			projMapButton.clicked.connect(self.btnCallback)

			sublayoutProj.addWidget( projMapButton)
			
			self.sublayoutGrid.addLayout( sublayoutProj, x,y)
			if (y < width):
				y = y + 1
			else:
				y = 0
				x = x+1

	def saveUserFileHistory(self):
		tree = ET.ElementTree(file="userFileHistory.xml")
		print (tree.getroot())
		root = tree.getroot()

		tree = ET.ElementTree(root)
		with open("updated.xml", "w") as f:
			tree.write(f)

	def getDirectoryLocation(self): 
		self.projDir = str(QFileDialog.getExistingDirectory(self, "Select Directory"))
		self.curDir2.setText(self.projDir)

	def addImages(self):
		self.srcDir = str(QFileDialog.getExistingDirectory(self, "Select Directory containing Images"))
		#self.curDir2.setText(self.projDir)

	def triggerButton(self, projName):
		tree = ET.ElementTree(file="userFileHistory.xml")
		#print (tree.getroot())
		root = tree.getroot()
		for project in root.iterfind('project'):
			if ( project.find('projName').text == projName):
				projectDir = project.find('projDir').text
				print(projectDir)
				self.loadMIDX(projectDir)
				print('Need to run visus viewer with projDir + /VisusSlamFiles/visus.midx')

	def loadMIDX(self, projectDir):
		print("NYI")
		print('Run visus viewer with: '+ projectDir + '/VisusSlamFiles/visus.midx')
		
		self.viewer.open(projectDir + '/VisusSlamFiles/visus.midx' ) 
		#self.viewer.run()
		#self.viewer.hide()
		self.tabs.setCurrentIndex(3) 

	#projectDir is where to save the files
	#srcDir is the location of initial images
	def startViSUSSLAM(self, projectDir, srcDir):
		print("NYI")
		print('Need to run visusslam with projDir and srcDir')
		self.tabs.setCurrentIndex(2) 
		os.system('cd ~/GIT/ViSUS/SLAM/Giorgio_SLAM_Nov212019/OpenVisus')
		os.system('python -m Slam '+srcDir)

 
 
# //////////////////////////////////////////////
def MainOld(argv):
	
	# set PYTHONPATH=D:/projects/OpenVisus/build/RelWithDebInfo
	# c:\Python37\python.exe CMake/PyViewer.py	
	
	SetCommandLine("__main__")
	GuiModule.createApplication()
	AppKitModule.attach()  	
	
	"""
	allow some python code inside scripting node
	"""

	viewer=Viewer()
	#viewer.open(r".\datasets\cat\gray.idx")

	viewer.open("/Users/amygooch/GIT/ViSUS/SLAM/Giorgio_SLAM/OpenVisus/Samples/python/../../datasets/cat/rgb.idx") 

	#viewer.open("/Users/amygooch/GIT/SCI/DATA/TaylorGrant/VisusSlamFiles/visus.midx") 

	# ... with some little python scripting
# 	viewer.setScriptingCode("""
# import cv2,numpy
# pdim=input.dims.getPointDim()
# img=Array.toNumPy(input,bShareMem=True)
# img=cv2.Laplacian(img,cv2.CV_64F)
# output=Array.fromNumPy(img,TargetDim=pdim)
# """.strip())	



# 	viewer.setScriptingCode("""
# import cv2,numpy
# pdim=input.dims.getPointDim()
# red = input[0]
# green = input[1]
# blue = input[2]
# #TGI – Triangular Greenness Index - RGB index for chlorophyll sensitivity. TGI index relies on reflectance values at visible wavelengths. It #is a fairly good proxy for chlorophyll content in areas of high leaf cover.
# #TGI = −0.5 * ((190 * (redData − greeData)) − (120*(redData − blueData)))
# scaleRed  = (0.39 * red)
# scaleBlue = (.61 * blue)
# TGI =  green - scaleRed - scaleBlue
# output=Array.fromNumPy(TGI,TargetDim=pdim)
# """.strip())	


	viewer.setScriptingCode("""
import cv2,numpy
pdim=input.dims.getPointDim()
red = input[0]
green = input[1]
blue = input[2]
# #TGI – Triangular Greenness Index - RGB index for chlorophyll sensitivity. TGI index relies on reflectance values at visible wavelengths. It #is a fairly good proxy for chlorophyll content in areas of high leaf cover.
# #TGI = −0.5 * ((190 * (redData − greeData)) − (120*(redData − blueData)))
scaleRed  = (0.39 * red)
scaleBlue = (.61 * blue)
TGI =  green - scaleRed - scaleBlue
#output=Array.fromNumPy(scaleRed,TargetDim=pdim)
output= TGI
""".strip())	

	viewer.run()
	
	GuiModule.execApplication()
	viewer=None  
	AppKitModule.detach()
	print("All done")
	sys.exit(0)	
	

# //////////////////////////////////////////////
if __name__ == '__main__':
	#Main(sys.argv)



	GuiModule.createApplication()
	AppKitModule.attach()  	

	# Create and display the splash screen
	splash_pix = QPixmap('icons/visoar_logo.png')
	#print('Error with Qt.WindowStaysOnTopHint')
	splash = QSplashScreen(splash_pix, Qt.WindowStaysOnTopHint)
	splash.setMask(splash_pix.mask())
	splash.show()

	print('Setting Fonts.... '+ str(QDir("Roboto")))
	dir_ = QDir("Roboto")
	_id = QFontDatabase.addApplicationFont("./Roboto-Regular.ttf")
	print(QFontDatabase.applicationFontFamilies(_id))

	font = QFont("Roboto");
	font.setStyleHint(QFont.Monospace);
	font.setPointSize(20)
	print('ERROR: not sure how to set the font in ViSUS')
	#app.setFont(font);

	

	window = StartWindow()

	window.show()

	

	splash.finish(window)

	GuiModule.execApplication()
	viewer=None  
	AppKitModule.detach()
	print("All done")
	sys.exit(0)	
	



	# 	<<project>
	# 	<projName> "Project2" </projName>
	# 	<dir> "/Users/amygooch/GIT/SCI/DATA/FromDale/ag1" </dir>
	# </project>
	# <<project>
	# 	<projName> "Project3" </projName>
	# 	<dir> "/Users/amygooch/GIT/SCI/DATA/TaylorGrant/rgb/" </dir>
	# </project>



