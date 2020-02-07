from OpenVisus import *
import numpy

# //////////////////////////////////////////////////////////////////////////
class PyScriptingNodeJob(NodeJob):
	
	# constructor
	def __init__(self, node, input, return_receipt):
		super().__init__()
		self.node=node
		self.code=node.getCode()
		self.input=input
		self.msg=	DataflowMessage()
		
		if return_receipt:
			self.msg.setReturnReceipt(return_receipt)		

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
		
		self.printMessage("Got in input",input.shape,input.dtype)
		
		g=globals()
		l={'input':input,'aborted':self.aborted}
			
		try:
			exec(self.code,g,l)
			
			output=l['output']
			
			if not type(output) is numpy.ndarray:
				raise Exception('output is not a numpy array')
			
		except Exception as e:
			if not self.aborted(): 
				self.printMessage('Failed to exec code',self.code,e)
			return	
		
		if self.aborted():
			return				
			
		self.printMessage("Output is ",output.shape, output.dtype)
		output=Array.fromNumPy(output,TargetDim=pdim,bShareMem=False)
		output.shareProperties(self.input)
		
		self.msg.writeArray("array", output)
		self.node.publish(self.msg) 	
		

# //////////////////////////////////////////////////////////////////////////
class PyScriptingNode(ScriptingNode):
	
	# __init__
	def __init__(self):
		super().__init__()
		self.editor=None
		
	# getTypeName
	def getTypeName(self):		
		return "PyScriptingNode"
		
	# getOsDependentTypeName
	def getOsDependentTypeName(self):
		return "PyScriptingNode"
	
	# setCode
	def setCode(self,code):
		super().setCode(code)
		if self.editor:
			self.editor.setText(code)

	# processInput
	def processInput(self):
	
		self.abortProcessing()
		self.joinProcessing()

		return_receipt = self.createPassThroughtReceipt()
		input = self.readArray("array")
		
		if input is None:
			return False

		self.bounds = input.bounds
		job=PyScriptingNodeJob(self, input, return_receipt)
		self.addNodeJob(job)
		return True 
		

		
from PyQt5.QtGui import *
from PyQt5.QtWidgets import *
from PyQt5.QtCore import *


# ///////////////////////////////////////////////////////////////////////////
class PyScriptingNodeView(QMainWindow):

		# constructor
		def __init__(self, node):
				super().__init__()
				
				self.gui_thread = QThread.currentThread()
				self.append_output_signal=pyqtSignal(str)
				self.append_output_signal.connect(self.appendOuput)
				
				self.node=node
				self.node.editor=self

				font = QFontDatabase.systemFont(QFontDatabase.FixedFont)
				font.setPointSize(10)
				
				self.editor = QPlainTextEdit()	
				self.editor.setFont(font)
				
				self.output = QPlainTextEdit()	
				self.output.setFont(font)
				self.output.setStyleSheet("background-color: rgb(150, 150, 150);")

				self.run_button = QPushButton('Run')	
				self.run_button.clicked.connect(self.runCode)				

				self.layout = QVBoxLayout()
				self.layout.addWidget(self.editor)
				self.layout.addWidget(self.output)
				self.layout.addWidget(self.run_button)	

				container = QWidget()
				container.setLayout(self.layout)
				self.setCentralWidget(container)
				
				self.setText(node.getCode())
				
		# getText
		def getText(self):
			return self.toPlainText()
			
		# setText
		def setText(self,value):
			self.setPlainText(value)		
						
		# runCode
		def runCode(self):
			self.node.setCode(self.getText())
			
		# appendOuput
		def appendOuput(self,msg):
			
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



