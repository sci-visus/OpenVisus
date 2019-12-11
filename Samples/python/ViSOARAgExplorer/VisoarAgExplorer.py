import sys, os

import xml.etree.ElementTree as ET
import xml.dom.minidom

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

from slam2dWidget 				import *

from analysis_scripts			import *
from lookAndFeel  				import *



# IMPORTANT for WIndows
# Mixing C++ Qt5 and PyQt5 won't work in Windows/DEBUG mode
# because forcing the use of PyQt5 means to use only release libraries (example: Qt5Core.dll)
# but I'm in need of the missing debug version (example: Qt5Cored.dll)
# as you know, python (release) does not work with debugging versions, unless you recompile all from scratch

# on windows rememeber to INSTALL and CONFIGURE


class StartWindow(QMainWindow):
	def __init__(self):
		super().__init__()
		self.setWindowTitle('ViSOAR Ag Explorer Prototype')
		
		self.setMinimumSize(QSize(600, 600))  
		self.setStyleSheet(LOOK_AND_FEEL)

		
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



# //////////////////////////////////////////////////////////////////////////////

class MyTabWidget(QWidget):
	def __init__(self, parent):
		super(QWidget, self).__init__(parent)
		self.layout = QVBoxLayout(self)
		self.app_dir = os.getcwd()
		self.cellsAcross = 6
		self.inputMode =  "R G B"
		self.projDir = ''
		self.projName = ''
		self.userFileHistory = os.getcwd()+'/userFileHistory.xml'
		self.viewer=Viewer()
		#self.viewer.hide()
		self.viewer.setMinimal()

		self.slam_widget = Slam2DWidget(self)
		
		class Buttons : pass
		self.buttons=Buttons

		self.logo = QPushButton('', self)
		self.logo.setStyleSheet("QPushButton {border-style: outset; border-width: 0px;color:#ffffff;}");
		self.logo.setIcon(QIcon(self.app_dir+'/icons/visoar_logo.png') )
		self.logo.setIconSize(QtCore.QSize(480, 214))

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
		self.tabs.addTab(self.tabViewer,"Analytics")
		self.tabs.currentChanged.connect(self.onTabChange) #changed!
		
		self.tabs.setTabEnabled(2,False)
		self.tabs.setTabEnabled(3,False)

		# Add layout of tabs to self
		self.layout.addWidget(self.tabs)  

		self.tabs.setCurrentIndex(0) 


	def mySetTabStyle(self):
		self.tabs.setStyleSheet  ( TAB_LOOK);

	def tabViewerUI(self):
		self.sublayoutTabViewer= QVBoxLayout(self)

		#Toolbar
		toolbar=QHBoxLayout()

		self.inputModeLabel = QLabel("Image Mode:", self)

		self.comboBox = QComboBox(self)
		self.comboBox.addItem("R G B")
		self.comboBox.addItem("R G NIR")
		self.comboBox.setStyleSheet(MY_COMBOX)
		self.comboBox.currentIndexChanged.connect(self.inputModeChanged)

		self.buttons.show_ndvi=GuiUtils.createPushButton("NDVI",
			lambda: self.showNDVI())

		self.buttons.show_tgi=GuiUtils.createPushButton("TGI",
			lambda: self.showTGI())
				
		self.buttons.show_rgb=GuiUtils.createPushButton("Original",
			lambda: self.showRGB())
				
		toolbar.addWidget(self.inputModeLabel)
		toolbar.addWidget(self.comboBox)
		toolbar.addWidget(self.buttons.show_ndvi)
		toolbar.addWidget(self.buttons.show_tgi)
		toolbar.addWidget(self.buttons.show_rgb)
		toolbar.addStretch(1)

		self.sublayoutTabViewer.addLayout(toolbar)

		self.inputModeChanged()

		#Viewer
		viewer_subwin = sip.wrapinstance(FromCppQtWidget(self.viewer.c_ptr()), QtWidgets.QMainWindow)	
		self.sublayoutTabViewer.addWidget(viewer_subwin )
		self.tabViewer.setLayout( self.sublayoutTabViewer)

	def inputModeChanged(self):
		self.inputMode = self.comboBox.currentText()
		print(self.inputMode) 
		if (self.inputMode == "R G B"):
			self.buttons.show_ndvi.setEnabled(False)
			self.buttons.show_ndvi.setStyleSheet(DISABLED_PUSH_BUTTON)
			self.buttons.show_tgi.setEnabled(True)
			self.buttons.show_tgi.setStyleSheet(GREEN_PUSH_BUTTON)
			self.buttons.show_rgb.setEnabled(True)
			self.buttons.show_rgb.setStyleSheet(GREEN_PUSH_BUTTON)
		if (self.inputMode == "R G NIR"):
			self.buttons.show_ndvi.setEnabled(True)
			self.buttons.show_ndvi.setStyleSheet(GREEN_PUSH_BUTTON)
			self.buttons.show_tgi.setEnabled(False)
			self.buttons.show_tgi.setStyleSheet(DISABLED_PUSH_BUTTON)
			self.buttons.show_rgb.setEnabled(True)
			self.buttons.show_rgb.setStyleSheet(GREEN_PUSH_BUTTON)


	# showNDVI
	def showNDVI(self):
		fieldname="output=ArrayUtils.interleave(ArrayUtils.split(voronoi())[0:3])"
		print("Showing NDVI for Red and IR channels")
		print(self.projDir)
		#self.viewer.open(self.projDir + '/VisusSlamFiles/visus.midx' ) 
		# make sure the RenderNode get almost RGB components
		self.viewer.setFieldName(fieldname)		
		self.viewer.setScriptingCode(NDVI_SCRIPT);

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
		self.viewer.setScriptingCode(TGI_script)


	def showRGB(self):
		fieldname="output=ArrayUtils.interleave(ArrayUtils.split(voronoi())[0:3])"
		print("Showing img src")
		#self.viewer.open(self.projDir + '/VisusSlamFiles/visus.midx' ) 
		# make sure the RenderNode get almost RGB components
		self.viewer.setFieldName(fieldname)		
		self.viewer.setScriptingCode(
"""
output=input

""");

	def tabStitcherUI(self):
		self.sublayoutTabStitcher= QVBoxLayout(self)

		# toolbar
		toolbar=QHBoxLayout()
		# self.buttons.load_midx=GuiUtils.createPushButton("Load Prev Solution",
		# 	lambda: self.loadPrevSolution())

		self.buttons.run_slam=GuiUtils.createPushButton("Stitch it!",
			lambda: self.run())
		self.buttons.goToAnalytics=GuiUtils.createPushButton("Analytics",
			lambda: self.goToAnalyticsTab())
		
		self.buttons.run_slam.setStyleSheet(GREEN_PUSH_BUTTON)
		self.buttons.goToAnalytics.setStyleSheet(DISABLED_PUSH_BUTTON)
		self.buttons.goToAnalytics.setEnabled(False)

		# self.buttons.show_ndvi=GuiUtils.createPushButton("NDVI",
		# 	lambda: self.showNDVI())

		# self.buttons.show_tgi=GuiUtils.createPushButton("TGI",
		# 	lambda: self.showTGI())
				
		# self.buttons.show_rgb=GuiUtils.createPushButton("RGB",
		# 	lambda: self.showRGB())
				
#		toolbar.addWidget(self.buttons.load_midx)
		toolbar.addWidget(self.buttons.run_slam)
		toolbar.addWidget(self.buttons.goToAnalytics)
		# toolbar.addWidget(self.buttons.show_ndvi)
		# toolbar.addWidget(self.buttons.show_tgi)
		# toolbar.addWidget(self.buttons.show_rgb)
 

		toolbar.addStretch(1)
		self.sublayoutTabStitcher.addLayout(toolbar)
		
		self.sublayoutTabStitcher.addWidget(self.slam_widget )
		self.tabStitcher.setLayout( self.sublayoutTabStitcher)

		_stdout = sys.stdout
		_stderr = sys.stderr

		logger=Logger(terminal=sys.stdout, filename="~visusslam.log", qt_callback=self.slam_widget.printLog)

		sys.stdout = logger
		sys.stderr = logger

	def goToAnalyticsTab(self):
		print('---->Loading midx from: '+self.projDir)
		try:
			self.loadMIDX(self.projDir)
		except IOError as e:
			print("I/O error({0}): {1}".format(e.errno, e.strerror))
		except ValueError:
			print("Could not convert data to an integer.")
		except AttributeError as error:
			print('AttributError: '+ error+ sys.exc_info()[0])
			raise
		except:
			print("Unexpected error:", sys.exc_info()[0])
			raise

		self.tabs.setCurrentIndex(3) 

	def run(self):
		self.slam_widget.run()
		self.buttons.goToAnalytics.setEnabled(True)
		self.buttons.goToAnalytics.setStyleSheet(GREEN_PUSH_BUTTON)
		self.buttons.run_slam.setStyleSheet(GRAY_PUSH_BUTTON)

	def tabNewUI(self):
		#Create New Tab:
		self.sublayoutTabNew= QVBoxLayout(self)
		self.sublayoutTabNew.addWidget(self.logo)  #,row,0)
	
		self.sublayoutForm = QFormLayout()
		self.sublayoutTabNew.addLayout(self.sublayoutForm) #, row, 1,4)

		self.createErrorLabel = QLabel('')
		self.sublayoutTabNew.addWidget(  self.createErrorLabel, alignment=Qt.AlignCenter    )
		self.createErrorLabel.setStyleSheet( 'color: ##59040c')

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
		self.buttonAddImages.setStyleSheet(GREEN_PUSH_BUTTON)

		#Ability to change location
		self.projDir= '' #os.getcwd()
		self.srcDir = '' #os.getcwd()
		self.curDir = QLabel('Image Directory: ')
		self.curDir2 = QLabel(self.projDir)
		self.curDir2.setStyleSheet("""font-family: Roboto;font-style: normal;font-size: 14pt; """)
		self.curDir.resize(280,40)
		# self.buttonChangeDir = QPushButton('Change Project Location', self)
		# self.buttonChangeDir.resize(180,40)
		# self.buttonChangeDir.clicked.connect( self.getDirectoryLocation)
		# self.buttonChangeDir.setStyleSheet("""QPushButton {
		# 	max-width:300px;
		# 	border-radius: 7;
		# 	border-style: outset; 
		# 	border-width: 0px;
		# 	color: #045951;
		# 	background-color: #e6e6e6;
		# 	padding: 10px;
		# }
		# QPushButton:pressed { 
		# 	background-color:  #e6e6e6;

		# }""")
		# self.spaceLabel = QLabel('')
		# self.spaceLabel.resize(380,40)
		self.sublayoutForm.addRow(self.curDir,self.curDir2)
		#self.sublayoutForm.addRow(self.spaceLabel, self.buttonChangeDir )

		#Button that says: "Create Project"
		self.buttons.create_project = QPushButton('Create Project', self)
		self.buttons.create_project.move(20,80)
		self.buttons.create_project.resize(180,80) 
		self.buttons.create_project.setStyleSheet(GREEN_PUSH_BUTTON)
		self.spaceLabel2 = QLabel('')
		self.spaceLabel2.resize(380,40)
		
		self.sublayoutTabNew.addWidget(  self.buttonAddImages, alignment=Qt.AlignCenter    )

		self.sublayoutTabNew.addWidget(  self.buttons.create_project, alignment=Qt.AlignCenter    )
		self.buttons.create_project.hide()
		# connect button to function on_click
		self.buttons.create_project.clicked.connect(  self.createProject)

		self.tabNew.setLayout( self.sublayoutTabNew)

		
	def tabLoadUI(self):
		self.sublayoutTabLoad= QVBoxLayout(self)
		self.sublayoutGrid = QGridLayout()
		self.sublayoutGrid.setSpacing(10)
		self.sublayoutGrid.setRowStretch(0, self.cellsAcross)
		self.sublayoutGrid.setRowStretch(1, 4) 
		self.LoadFromFile()
		self.sublayoutTabLoad.addLayout(self.sublayoutGrid)

		self.tabLoad.setLayout( self.sublayoutTabLoad)
		self.tabLoad.setStyleSheet("""background-color: #045951""")

	def indent(elem, level=0, more_sibs=False):
		i = "\n"
		if level:
			i += (level-1) * '  '
		num_kids = len(elem)
		if num_kids:
			if not elem.text or not elem.text.strip():
				elem.text = i + "  "
				if level:
					elem.text += '  '
			count = 0
			for kid in elem:
				indent(kid, level+1, count < num_kids - 1)
				count += 1
			if not elem.tail or not elem.tail.strip():
				elem.tail = i
				if more_sibs:
					elem.tail += '  '
		else:
			if level and (not elem.tail or not elem.tail.strip()):
				elem.tail = i
				if more_sibs:
					elem.tail += '  '

	#User has specified location for data and the project name, launch ViSUS SLAM
	def createProject(self):
		self.createErrorLabel.setText('')
		self.projName = self.projNametextbox.text()
		self.projDir = self.curDir2.text()
		print(self.projName)
		print(self.projDir)
		if ((not self.projDir.strip()== "") and (not self.projName.strip()=="")): 
			print('Create Proj')
			print(self.projName)
			print(self.projDir)

			tree = ET.parse(self.userFileHistory)
			print (tree.getroot())
			root = tree.getroot()

			#etree.SubElement(item, 'Date').text = '2014-01-01'
			element = ET.Element('project')
			ET.SubElement(element, 'projName').text = self.projName
			ET.SubElement(element, 'projDir').text =  self.projDir
			ET.SubElement(element, 'srcDir').text =  self.srcDir
			root.append(element)
			#print(ET.tostring(element ))

			#indent(tree)
			tree.write(self.userFileHistory)
			print('Wrote new project to history')
			self.tabs.setTabEnabled(2,True)		
			#self.tabs.setTabEnabled(3,True)
			self.tabs.setCurrentIndex(2) 
			print('Change tabs')
			self.startViSUSSLAM()
			print('started slam')
		else:
			errorStr = ''
			if not self.projDir.strip():
				errorStr = 'Please Provide a directory of images or click on the load tab to load a dataset you\'ve already stitched\n'
			if not self.projName.strip():
				errorStr = errorStr + 'Please provide a unique name for your project'
			self.createErrorLabel.setText(errorStr)

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
		tree = ET.ElementTree(file=self.userFileHistory)
		print (tree.getroot())
		root = tree.getroot()
		x = 0
		y = 0
		width = self.cellsAcross -1

		for project in root.iterfind('project'):

			projName = project.find('projName').text
			projDir = project.find('projDir').text
			print(projName + " "+ projDir)
			
			sublayoutProj = QVBoxLayout()

			projMapButton = QToolButton(self)
			projMapButton.setToolButtonStyle(Qt.ToolButtonTextUnderIcon);
			projMapButton.setStyleSheet("background-color: #045951; QPushButton {background-color: #045951; border-style: outset; border-width: 0px;color:#ffffff;}");
			projMapButton.setIcon(QIcon(self.app_dir + '/icons/genericmap.png') )
			projMapButton.setIconSize(QtCore.QSize(100, 100))
			projMapButton.setText(projName)
			projMapButton.setStyleSheet("QToolButton{font-size: 20px;font-family: Roboto;color: rgb(38,56,76);background-color: rgb(255, 255, 255);}");
			projMapButton.setFixedSize(180,180)
			#projMapButton.setSizePolicy( QSizePolicy.Preferred, QSizePolicy.Expanding)

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
		tree = ET.ElementTree(file=self.userFileHistory)
		print (tree.getroot())
		root = tree.getroot()

		tree = ET.ElementTree(root)
		with open("updated.xml", "w") as f:
			indent(root)
			tree.write(f)

	def getDirectoryLocation(self): 
		self.projDir = str(QFileDialog.getExistingDirectory(self, "Select Directory"))
		self.curDir2.setText(self.projDir)

	def addImages(self):
		self.projName = self.projNametextbox.text()
		self.srcDir = str(QFileDialog.getExistingDirectory(self, "Select Directory containing Images"))
		self.projDir = self.srcDir
		self.curDir2.setText(self.projDir) 
		if ((not (self.projDir.strip()== "")) and (not (self.projName.strip()==""))): 
			self.tabs.setTabEnabled(2,True)		
			#self.tabs.setTabEnabled(3,True)
			self.tabs.setCurrentIndex(2) 
			self.createProject()
		else:
			errorStr = ''
			if not self.projDir.strip():
				print(self.projDir)
				errorStr = 'Please Provide a directory of images or click on the load tab to load a dataset you\'ve already stitched\n'
			if not self.projName.strip():
				print(self.projName)
				errorStr = errorStr + 'Please provide a unique name for your project'
			self.createErrorLabel.setText(errorStr)

		self.buttons.create_project.show()
		self.buttonAddImages.setStyleSheet(GRAY_PUSH_BUTTON)
		

	def triggerButton(self, projName):
		tree = ET.ElementTree(file=self.userFileHistory)
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
			
		self.tabs.setTabEnabled(3,True)
		self.tabs.setCurrentIndex(3) 

	#projectDir is where to save the files
	#srcDir is the location of initial images
	def startViSUSSLAM(self ):
		print("NYI")
		print('Need to run visusslam with projDir and srcDir')
		
		self.slam_widget.setCurrentDir(self.srcDir)
		
		#self.showFullScreen()
		#os.system('cd ~/GIT/ViSUS/SLAM/Giorgio_SLAM_Nov212019/OpenVisus')
		#print('cd ~/GIT/ViSUS/SLAM/Giorgio_SLAM_Nov212019/OpenVisus; python -m Slam '+srcDir)
		#os.system('python -m Slam '+srcDir)


# //////////////////////////////////////////////
if __name__ == '__main__':

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



