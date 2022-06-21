
import sys, os
from OpenVisus import *
from OpenVisus.gui import *

from PyQt5 import QtCore 
from PyQt5.QtCore                     import QUrl
from PyQt5.QtGui                      import QIcon,QColor
from PyQt5.QtWidgets                  import QApplication, QHBoxLayout, QLineEdit
from PyQt5.QtWidgets                  import QMainWindow, QPushButton, QVBoxLayout,QSplashScreen
from PyQt5.QtWidgets                  import QWidget
from PyQt5.QtWidgets                  import QTableWidget,QTableWidgetItem

try:
    from PyQt5 import sip as  sip
except ImportError:
    import sip


# //////////////////////////////////////////////////////////////////////////////
def CreatePushButton(text,callback=None, img=None ):
	ret=QPushButton(text)
	ret.setAutoDefault(False)
	if callback: ret.clicked.connect(callback)
	if img: ret.setIcon(QIcon(img))
	return ret
	
# //////////////////////////////////////////////////////////////////////////////
class Buttons : 
	pass

# //////////////////////////////////////////////////////////////////////////////
class MyViewer(Viewer):
	
	# constructor
	def __init__(self,name="",url=""):
		super(MyViewer, self).__init__()
		super(MyViewer, self).setMinimal()
		self.name=name
		if url:
			self.open(url)
		self.on_camera_change=None
		
	# glCameraChangeEvent
	def glCameraChangeEvent(self):
		super(MyViewer, self).glCameraChangeEvent()
		if self.on_camera_change:
			self.on_camera_change()


# //////////////////////////////////////////////////////////////////////////////
class MyWindow(QMainWindow):
	
	# constructor
	def __init__(self):
		super(MyWindow, self).__init__()
		self.createGui()
		self.showMaximized()
		
	# destroy
	def destroy(self):
		self.viewer1=None
		self.viewer2=None
		
	# onCameraChange
	def onCameraChange(self,cam1,cam2):
		
		# avoid rehentrant calls
		if hasattr(self,"changing_camera") and self.changing_camera: 
			return
		
		self.changing_camera=True
		cam2.beginTransaction()
		cam2.setLookAt(cam1.getPos(),cam1.getCenter(),cam1.getVup())

		if isinstance(cam1,GLLookAtCamera):
			cam2.setFov(cam1.getFov())
			cam2.setCenterOfRotation(cam1.getCenterOfRotation())
			cam2.setRotation(cam1.getRotation())
		else:
			cam2.setOrthoParams(cam1.getOrthoParams())
			
		cam2.endTransaction()
		self.changing_camera=False


	# createGui
	def createGui(self):

		self.setWindowTitle("MyWindow")
		
		# create buttons
		if True:
			self.buttons=Buttons
			self.buttons.run_slam=CreatePushButton("Run",lambda: self.run())
		
		# create viewers
		if True:


			self.viewer1=MyViewer(name="viewer1",url="http://atlantis.sci.utah.edu/mod_visus?dataset=2kbit1")
			self.viewer2=MyViewer(name="viewer2",url="http://atlantis.sci.utah.edu/mod_visus?dataset=2kbit1")
			
			cam1=self.viewer1.getGLCamera()
			cam2=self.viewer2.getGLCamera()

			# disable smoothing
			if isinstance(cam1,GLOrthoCamera): cam1.toggleDefaultSmooth()
			if isinstance(cam2,GLOrthoCamera): cam2.toggleDefaultSmooth()
			
			self.viewer1.on_camera_change=lambda : self.onCameraChange(cam1,cam2)
			self.viewer2.on_camera_change=lambda : self.onCameraChange(cam2,cam1)
		
		# create log
		if True:
			self.log = QTextEdit()
			self.log.setLineWrapMode(QTextEdit.NoWrap)
			p = self.log.viewport().palette()
			p.setColor(QPalette.Base, QColor(200,200,200))
			p.setColor(QPalette.Text, QColor(0,0,0))
			self.log.viewport().setPalette(p)
			
		# create toolbar
		if True:
			toolbar=QHBoxLayout()
			toolbar.addWidget(self.buttons.run_slam)
			toolbar.addStretch(1)			
		
		# create central panel
		if True:
			center = QSplitter(QtCore.Qt.Horizontal)
			center.addWidget(sip.wrapinstance(FromCppQtWidget(self.viewer1.c_ptr()), QMainWindow))
			center.addWidget(sip.wrapinstance(FromCppQtWidget(self.viewer2.c_ptr()), QMainWindow))
			center.setSizes([100,100])
		
		# window layout
		if True:
			main_layout=QVBoxLayout()
			main_layout.addLayout(toolbar)	
			main_layout.addWidget(center,1)
			main_layout.addWidget(self.log)

			central_widget = QFrame()
			central_widget.setLayout(main_layout)
			central_widget.setFrameShape(QFrame.NoFrame)
			self.setCentralWidget(central_widget)


# //////////////////////////////////////////////
def Main(argv):
	win=MyWindow(); 
	win.show()
	QApplication.exec()
	win.destroy()
	print("All done")
	sys.exit(0)	
	

# //////////////////////////////////////////////
if __name__ == '__main__':
	Main(sys.argv)

