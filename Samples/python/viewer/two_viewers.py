
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
class MyWindow(QMainWindow):
	
	# constructor
	def __init__(self):
		super(MyWindow, self).__init__()
		self.createGui()
		
	# destroy
	def destroy(self):
		self.viewer1=None
		self.viewer2=None

	# createGui
	def createGui(self):

		self.setWindowTitle("MyWindow")
		
		# create buttons
		if True:
			self.buttons=Buttons
			self.buttons.run_slam=CreatePushButton("Run",lambda: self.run())
		
		# create viewers
		if True:
			self.viewer1=Viewer(); self.viewer1.open("./datasets/cat/gray.idx"); self.viewer1.setMinimal()
			self.viewer2=Viewer(); self.viewer2.open("./datasets/cat/rgb.idx");  self.viewer2.setMinimal()
		
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
			center.addWidget(sip.wrapinstance(FromCppQtWidget(self.viewer2.c_ptr()), QMainWindow))
			center.addWidget(sip.wrapinstance(FromCppQtWidget(self.viewer1.c_ptr()), QMainWindow))
			center.setSizes([100,200])
		
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

