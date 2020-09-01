import numpy, datetime

from OpenVisus      import *
from OpenVisus.gui  import *

from PyQt5.QtGui     import *
from PyQt5.QtWidgets import *
from PyQt5.QtCore    import *

# //////////////////////////////////////////////////////////////////////////
class MyJob(NodeJob):
	
	# constructor
	def __init__(self):
		super().__init__()

	# printMessage
	def printMessage(self,*args):
		
		msg=""
		for it in args:
			msg+=str(it)+ " "
		
		if self.node.editor:
			self.node.editor.appendOutput(msg)
		else:
			print(msg)	
		
	# runJob
	def runJob(self):
		
		if self.aborted():
			return		
		
		pdim=self.input.dims.getPointDim()
		
		input=Array.toNumPy(self.input,bShareMem=True)
		
		self.printMessage(datetime.datetime.now(),"Got in input",input.shape,input.dtype)

		g=globals()
		g['input']=input
		g['aborted']=self.aborted

		try:
			exec(self.code,g)

			if not 'output' in g:
				raise Exception('output empty. Did you forget to set it?')

			output=g['output']
			
			if not type(output) is numpy.ndarray:
				raise Exception('output is not a numpy array')
			
		except Exception as e:
			if not self.aborted(): 
				import traceback
				self.printMessage(datetime.datetime.now(),'Python error\n',traceback.format_exc())
			return	
		
		if self.aborted():
			return				
			
		self.printMessage(datetime.datetime.now(),"Output is ",output.shape, output.dtype)
		output=Array.fromNumPy(output,TargetDim=pdim,bShareMem=False)
		output.shareProperties(self.input)
		
		self.msg.writeArray("array", output)
		self.node.publish(self.msg) 	
		

# ///////////////////////////////////////////////////////////////////////////
class MyView(QMainWindow):
	
	append_output_signal=pyqtSignal(str)

	# constructor
	def __init__(self, node):
		super().__init__()
			
		self.gui_thread = QThread.currentThread()
		self.append_output_signal.connect(self.appendOutput)
			
		self.node=node
		self.node.editor=self

		font = QFontDatabase.systemFont(QFontDatabase.FixedFont)
		font.setPointSize(9)
			
		self.presets = QComboBox()
		self.presets.currentIndexChanged.connect(self.onPresetIndexChanged)
			
		self.editor = QPlainTextEdit()	
		self.editor.setFont(font)
			
		self.output = QPlainTextEdit()	
		self.output.setFont(font)
		self.output.setStyleSheet("background-color: rgb(150, 150, 150);")

		self.run_button = QPushButton('Run')	
		self.run_button.clicked.connect(self.onRunButtonClick)				

		self.layout = QVBoxLayout()
		self.layout.addWidget(self.presets)
		self.layout.addWidget(self.editor)
		self.layout.addWidget(self.output)
		self.layout.addWidget(self.run_button)	

		container = QWidget()
		container.setLayout(self.layout)
		self.setCentralWidget(container)
			
		self.guessPresets()

		code=node.getCode()
		if code: 
			self.setText(code)
			
	# getText
	def getText(self):
		return self.editor.toPlainText()
		
	# setText
	def setText(self,value):
		self.editor.setPlainText(value)		
					
	# onRunButtonClick
	def onRunButtonClick(self):
		bForce=True
		self.node.setCode(self.getText(), bForce)
		
	# onPresetIndexChanged
	def onPresetIndexChanged(self,index):
		descr=self.presets.currentText()
		code=str(self.presets.itemData(index))
		self.setText(code)
	
	# addPreset
	def addPreset(self,descr=None,code=""):
		code= code if isinstance(code, str) else "\n".join([it for it in code])
		if descr is None: descr=code
		self.presets.addItem(descr, code)
			
	# guessPresets
	def guessPresets(self, dtype=None):
		self.presets.clear()
		self.addPreset("identity","output=input")
		self.addPreset("cast to float32","output=input.astype(numpy.float32)")
		self.addPreset("cast to float64","output=input.astype(numpy.float32)")
		
		if dtype is not None:
			N = dtype.ncomponents()
			for I in range(N):
				self.addPreset("output=input[%d]" % (I,))		
				
		self.addPreset("Laplacian",[
			"import cv2",
			"output=cv2.Laplacian(input,cv2.CV_64F)"
		])	

		self.addPreset("Canny",[
			"import cv2",
			"output=cv2.Canny(input,100,200)"
		])	

	# appendOutput
	def appendOutput(self,msg):
		
		if QThread.currentThread()!=self.gui_thread:
			self.append_output_signal.emit(msg)
			return
		
		self.output.moveCursor(QTextCursor.End)
		self.output.insertPlainText(msg)
		self.output.insertPlainText("\n")
		self.output.moveCursor(QTextCursor.End)

	# showError
	def showError(self, s):
		dlg = QMessageBox(self)
		dlg.setText(s)
		dlg.setIcon(QMessageBox.Critical)
		dlg.show()


# //////////////////////////////////////////////////////////////////////////
class PyScriptingNode(ScriptingNode):
	
	# __init__
	def __init__(self):
		super().__init__()
		self.editor=None

	# getTypeName
	def getTypeName(self):
		return "ScriptingNode" # i'm returning the same string anyway
		
	# setCode
	def setCode(self,value,bForce=False):
		super().setCode(value,bForce)
		if not self.editor: return
		self.editor.setText(value)

	# processInput
	def processInput(self):
	
		self.abortProcessing()
		self.joinProcessing()

		return_receipt = self.createPassThroughtReceipt()

		input = self.readArray("array")
		
		if input is None:
			return False

		self.setBounds(input.bounds)

		job=MyJob()
		job.node=self
		job.code=self.getCode()
		job.input=input
		job.msg=DataflowMessage()
		job.msg.setReturnReceipt(return_receipt)

		# python won't use it anymore (this is to force deallocation)
		del return_receipt 

		self.addNodeJob(job)
		
		return True 
		
	# createEditor
	def createEditor(self):
		MyView(self).show()
