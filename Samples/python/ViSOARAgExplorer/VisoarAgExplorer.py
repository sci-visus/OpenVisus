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
		#self.setCentralWidget(self.central_widget)
		#self.layout = QVBoxLayout(self.central_widget)

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
		self.tabs.resize(600,600)

		self.mySetTabStyle()
		self.tabNewUI()
		self.tabLoadUI()

		# Add tabs
		self.tabs.addTab(self.tabNew,"New Project")
		self.tabs.addTab(self.tabLoad,"Load Project")
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

		#Ability to change location
		self.projDir= os.getcwd()
		self.curDir = QLabel('Current Directory:')
		self.curDir2 = QLabel(self.projDir)
		self.curDir2.setStyleSheet("""
		font-family: Roboto;font-style: normal;font-size: 14pt; """)

		self.curDir.resize(280,40)
		self.buttonChangeDir = QPushButton('Change Location', self)
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
		self.sublayoutForm.addRow(self.spaceLabel, self.buttonChangeDir )

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
		# self.buttons.load_proj=GuiUtils.createPushButton("Load",
		# 	lambda: self.LoadFromFile())
		# self.buttons.load_proj.setIcon(QIcon('./icons/open_dir.png') )
		# self.buttons.load_proj.setIconSize(QtCore.QSize(180, 180))
		# self.sublayoutTabLoad.addWidget(self.buttons.load_proj)
		self.sublayoutGrid = QGridLayout()
		self.sublayoutGrid.setSpacing(10)

		self.LoadFromFile()
		self.sublayoutTabLoad.addLayout(self.sublayoutGrid)

		self.tabLoad.setLayout( self.sublayoutTabLoad)
		self.tabLoad.setStyleSheet("""background-color: #045951""")
		

	def startNew(self):
		print('NYI:')

		


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
		root.append(element)
		print(ET.tostring(element ))
		tree.write('userFileHistory.xml')

	def onTabChange(self):
		for i in reversed(range(self.sublayoutGrid.count())): 
			widgetToRemove = self.sublayoutGrid.itemAt(i).widget()
			if (widgetToRemove != None ):
				
				# remove it from the layout list
				self.sublayoutGrid.removeWidget(widgetToRemove)
				# remove it from the gui
			
				widgetToRemove.setParent(None)

		self.LoadFromFile()

	def LoadFromFile(self):

		#self.layout.setColumnStretch(1, 4)
		#self.layout.setColumnStretch(2, 4)
		#Parse users history file, contains files they have loaded before
		tree = ET.ElementTree(file="userFileHistory.xml")
		print (tree.getroot())
		root = tree.getroot()
		x = 0
		y = 0
		width = 4

		#self.loadBtn_group = QButtonGroup(self)
		#group.setStyleSheet("""background-color: #045951""")
		 

		for project in root.iterfind('project'):

			projName = project.find('projName').text
			projDir = project.find('projDir').text
			print(projName + " "+ projDir)
			
			#sublayoutProjFrame = QFrame()
			#sublayoutProjFrame.setFrameShape(QFrame.NoFrame)
			sublayoutProj = QVBoxLayout()
			#sublayoutProjFrame.setStyleSheet("background-color:white;");

			projMapButton = QToolButton(self)
			projMapButton.setToolButtonStyle(Qt.ToolButtonTextUnderIcon);
			projMapButton.setStyleSheet("background-color: #045951; QPushButton {background-color: #045951; border-style: outset; border-width: 0px;color:#ffffff;}");
			projMapButton.setIcon(QIcon('./icons/genericmap.png') )
			projMapButton.setIconSize(QtCore.QSize(180, 180))
			projMapButton.setText(projName)
			#projMapButton.setCheckable(True)

			font = QFont("Roboto");
			font.setStyleHint(QFont.Monospace);
			font.setPointSize(20)
			#projMapButton.setFont(font)
			projMapButton.setStyleSheet("QToolButton{font-size: 20px;font-family: Roboto;color: rgb(38,56,76);background-color: rgb(255, 255, 255);}");

			#projMapButton.move(20,80)
			projMapButton.resize(180,180)
			projMapButton.setSizePolicy( QSizePolicy.Preferred, QSizePolicy.Expanding)

			self.btnCallback = partial(self.triggerButton, projName)
			projMapButton.clicked.connect(self.btnCallback)

			#projMapButton.clicked.connect(lambda: self.triggerButton(projMapButton))

			#self.loadBtn_group.addButton(projMapButton)
			#self.loadBtn_group.buttonReleased['QAbstractButton *'].connect(self.button_clicked)
			 

			# projLabel = QPushButton(projName, self)
			# projLabel.resize(480,40)
			#projLabel.clicked.connect( lambda: self.openProject(projMapButton))

			sublayoutProj.addWidget( projMapButton)
			# sublayoutProj.addWidget( projLabel)

			#sublayoutProjFrame.addLayout( sublayoutProj)
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
		 

		# with open("userFileHistory.xml") as f:
		# 	content = f.read()

		# soup = BeautifulSoup(content, "xml")
		# f.write(soup.prettify())
		# f.close()

		tree = ET.ElementTree(root)
		with open("updated.xml", "w") as f:
			tree.write(f)

	def getDirectoryLocation(self):
	 
		self.projDir = str(QFileDialog.getExistingDirectory(self, "Select Directory"))
		self.curDir2.setText(self.projDir)

	def openProject(self):
		#Open project at dir with ViSUS SLAM

		tree = ET.ElementTree(file="userFileHistory.xml")
		print (tree.getroot())
		root = tree.getroot()
		print(b.text)
		for project in root.iterfind('project'):
			projName = project.find('projName').text
			projDir = project.find('projDir').text
			if (projDir == b.text()):
				print('NYI '+ projDir)
			else:
				print('NYI: didnot find button and database match?')


	def button_clicked(self, button_or_id):
		# btn_pressed = self.loadBtn_group.checkedButton()
		# print(btn_pressed.text())
		if isinstance(button_or_id, QAbstractButton):
			print('"{}" was clicked'.format(button_or_id.text()))
			self.triggerButton(button_or_id.text())
		# elif isinstance(button_or_id, int):
		# 	print('"Id {}" was clicked'.format(button_or_id))

	def triggerButton(self, projName):
		tree = ET.ElementTree(file="userFileHistory.xml")
		#print (tree.getroot())
		root = tree.getroot()
		for project in root.iterfind('project'):
			if ( project.find('projName').text == projName):
				projectDir = project.find('projDir').text
				print(projectDir)
				self.loadSLAM(projectDir)

	def loadSLAM(self, projectDir):
		print("NYI")


 
 
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

	viewer=PyViewer()
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



