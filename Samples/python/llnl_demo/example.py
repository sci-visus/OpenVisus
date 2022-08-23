
import sys
from PyQt5.QtWidgets import *
from PyQt5.QtCore import *
from PyQt5.QtGui import *

from OpenVisus import *
import pyvista as pv
from pyvistaqt import QtInteractor


# ////////////////////////////////////////////////////////////////////////////
class Explorer3d(QMainWindow):
	
	# construtor
	def __init__(self,url,parent=None):
		
		super().__init__(parent)
		
		self.db=LoadDataset(url)
		self.dims=self.db.getLogicBox()[1]
		print("dims",self.dims)

		self.createUI()
		self.showVolume()
		self.show()
		

	# createUI
	def createUI(self):
		
		self.x1=self.createSlider(0, 0,self.dims[0]);self.x2=self.createSlider(self.dims[0], 0,self.dims[0])
		self.y1=self.createSlider(0, 0,self.dims[1]);self.y2=self.createSlider(self.dims[1], 0,self.dims[1])
		self.z1=self.createSlider(0, 0,self.dims[2]);self.z2=self.createSlider(self.dims[2], 0,self.dims[2])
		
		self.field=QComboBox()
		for name in self.db.getFields():
			self.field.addItem(name)
		self.field.setCurrentText(self.db.getFields()[0])
		
		self.type=QComboBox()
		for name in ( 'vr','threshold','isovalue','slice'):
			self.type.addItem(name)
		self.type.setCurrentText('vr')
		
		self.opacity=QComboBox()
		for name in ( 'linear', 'geom', 'geom_r','sigmoid','sigmoid_3','sigmoid_4','sigmoid_5','sigmoid_6','sigmoid_7','sigmoid_8','sigmoid_9','sigmoid_10'):
			self.opacity.addItem(name)
		self.opacity.setCurrentText('sigmoid')
		
		self.cmap=QComboBox()
		for name in ( 'gist_ncar','bone', 'cool','viridis','coolwarm','magma'):
			self.cmap.addItem(name)
		self.cmap.setCurrentText('gist_ncar')		
		
		self.blending=QComboBox()
		for name in ( 'additive', 'maximum', 'minimum', 'composite',  'average'):
			self.blending.addItem(name)
		self.blending.setCurrentText('additive')	
		
		self.max_mb=QComboBox()
		for name in ('8','16','32', '64', '128', '256', '512',  '1024',  '2048'):
			self.max_mb.addItem(name)
		self.max_mb.setCurrentText('32')
		
		self.run=QPushButton("Run")
		self.run.clicked.connect(self.showVolume)
		
		main_layout=QVBoxLayout()
		
		# plotter
		if True:
			frame=QFrame()
			self.plotter = QtInteractor(frame)
			main_layout.addWidget(self.plotter)
			
		if True:
			layout=self.createVerticalLayout([
				self.createGridLayout((
					[QLabel("x1"),self.x1,QLabel("x2"),self.x2],
					[QLabel("y1"),self.y1,QLabel("y2"),self.y2],
					[QLabel("z1"),self.z1,QLabel("z2"),self.z2])),
				self.createHorizontalLayout([QLabel("field"),self.field]),
				self.createHorizontalLayout([QLabel("type"),self.type]),
				self.createHorizontalLayout([QLabel("opacity"),self.opacity]),
				self.createHorizontalLayout([QLabel("cmap"),self.cmap]),
				self.createHorizontalLayout([QLabel("blending"),self.blending]),
				self.createHorizontalLayout([QLabel("MBytes"),self.max_mb]),
				self.run
			])
			main_layout.addLayout(layout)

		central_widget=QFrame()
		central_widget.setLayout(main_layout)
		self.setCentralWidget(central_widget)
		self.showMaximized()
		
	# getVolumeBounds
	def getVolumeBounds(self):
		x1=self.x1.value();x2=self.x2.value();x1,x2=min(x1,x2),max(x1,x2)
		y1=self.x1.value();y2=self.x2.value();y1,y2=min(y1,y2),max(y1,y2)
		z1=self.x1.value();z2=self.x2.value();z1,z2=min(z1,z2),max(z1,z2)
		return x1,x2,y1,y2,z1,z2

	# extractVolume
	def extractVolume(self):
		field=self.db.getField(self.field.currentText())
		
		x1,x2,y1,y2,z1,z2=self.getVolumeBounds()

		# guess quality
		dtype=field.dtype
		sample_size=dtype.getByteSize()
		mem_size=int((x2-x1)*(y2-y1)*(z2-z1)*sample_size/ (1024*1024))
			
		max_mb=int(self.max_mb.currentText())
			
		quality=0
		while mem_size>max_mb:
			quality-=1; mem_size=int(mem_size/2)

		data=self.db.read(
			x=[x1,x2],
			y=[y1,y2],
			z=[z1,z2],
			field=self.field.currentText(), 
			quality=quality)
			
		print("Extracted volume","shape",data.shape,"dtype",data.dtype,"mb",int(sys.getsizeof(data)/(1024*1024)), "quality",quality)
		return data
		
	# showVolume
	def showVolume(self):
		data=self.extractVolume()
		self.plotter.clear()
		# self.plotter.show_bounds(grid=True, location='back')
		
		grid = pv.UniformGrid()
		grid.dimensions = numpy.array(data.shape) +1
		x1,x2,y1,y2,z1,z2=self.getVolumeBounds()
		grid.origin  = (0,0,0) # The bottom left corner of the data set
		w,h,d=data.shape
		grid.spacing = ((x2-x1)/w, (y2-y1)/h, (z2-z1)/d) # These are the cell sizes along each axis
		grid.cell_arrays["values"] = data.flatten(order="F")		
		
		# /////////////////////////////////////////////////////////
		# example https://docs.pyvista.org/examples/00-load/create-uniform-grid.html
		if self.type.currentText()=='vr':
			self.plotter.add_volume(grid, opacity=self.opacity.currentText(), cmap=self.cmap.currentText(),blending=self.blending.currentText())

		elif self.type.currentText()=='threshold':
			self.plotter.add_mesh_threshold(grid)
			
		elif self.type.currentText()=='isovalue':
			self.plotter.add_mesh_isovalue(pv.wrap(data)) # TODO, aspect ratio not working
			
		elif self.type.currentText()=='slice':
			self.plotter.add_mesh_slice(grid)
			
		else:
			raise Exception("internal error")
			
		self.plotter.reset_camera()

	# createSlider
	def createSlider(self,value,m,M):
		ret = QSlider(Qt.Horizontal, self)
		ret.setRange(m, M)
		ret.setValue(value)
		ret.setFocusPolicy(Qt.NoFocus)
		ret.setPageStep(5)
		# ret.valueChanged.connect(self.showVolume)
		return ret
		
	# createHorizontalLayout
	def createHorizontalLayout(self,widgets):
		ret=QHBoxLayout()
		for widget in widgets:
			try:
				ret.addWidget(widget)
			except:
				ret.addLayout(widget)
		return ret
		
	# createVerticalLayout
	def createVerticalLayout(self,widgets):
		ret=QVBoxLayout()
		for widget in widgets:
			try:
				ret.addWidget(widget)
			except:
				ret.addLayout(widget)
		return ret		
		
	# createGridLayout
	def createGridLayout(self,rows):
		ret = QGridLayout()
		nrow=0
		for row in rows:
			ncol=0
			for widget in row:
				if widget:
					ret.addWidget(widget,nrow,ncol)
				ncol+=1
			nrow+=1
		return ret
		
		
if __name__ == "__main__":
	app = QApplication(sys.argv)
	
	if len(sys.argv)>1:
		url=sys.argv[-1]
	else:
		url=r'http://atlantis.sci.utah.edu/mod_visus?dataset=2kbit1&cached=1'

	explorer=Explorer3d(url)
	explorer.show()
	# explorer.showVolume()
	sys.exit(app.exec_())


