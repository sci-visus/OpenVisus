import os, sys, importlib.util

_agent_llm_module = None


def _load_agent_llm_module():
	"""Load agent_llm from the OpenVisus package, or from this file's directory (dev builds often omit new .py files)."""
	global _agent_llm_module
	if _agent_llm_module is not None:
		return _agent_llm_module
	try:
		import OpenVisus.agent_llm as m
		_agent_llm_module = m
		return m
	except ImportError:
		pass
	here = os.path.dirname(os.path.abspath(__file__))
	for path in (
		os.path.join(here, "agent_llm.py"),
		os.path.normpath(os.path.join(here, "..", "..", "Libs", "swig", "agent_llm.py")),
	):
		if not os.path.isfile(path):
			continue
		try:
			spec = importlib.util.spec_from_file_location("openvisus_agent_llm", path)
			if spec is None or spec.loader is None:
				continue
			m = importlib.util.module_from_spec(spec)
			spec.loader.exec_module(m)
			_agent_llm_module = m
			return m
		except Exception:
			continue
	return None


def _agent_chat_widget_alive(w):
	if w is None:
		return False
	try:
		from PyQt5 import sip
		return not sip.isdeleted(w)
	except Exception:
		try:
			w.objectName()
			return True
		except Exception:
			return False


def ImportQt5Sip():
	try:
		 from PyQt5 import sip as  sip
	except ImportError:
		 import sip

import PyQt5
from   PyQt5.QtCore	import *
from   PyQt5.QtWidgets import *
from   PyQt5.QtGui	 import *

ImportQt5Sip()

from OpenVisus	    import *
from OpenVisus.gui import *

from html import escape as _html_escape


def _same_widget(a, b):
	"""SWIG/PyQt wrappers may break `is`; compare identity, equality, and winId()."""
	if a is None or b is None:
		return False
	if a is b:
		return True
	try:
		if a == b:
			return True
	except Exception:
		pass
	try:
		if int(a.winId()) == int(b.winId()):
			return True
	except Exception:
		pass
	return False


def _find_agent_chat_panel_under_viewer(viewer):
	"""Locate AgentChatPanel; SWIG Viewer does not expose QObject.findChild/findChildren."""
	app = QApplication.instance()
	if app is None:
		return None
	panels = [w for w in app.allWidgets() if w.objectName() == "AgentChatPanel"]
	if len(panels) == 1:
		return panels[0]
	for w in panels:
		try:
			top = w.window()
		except Exception:
			top = None
		if top is not None and _same_widget(top, viewer):
			return w
		try:
			if hasattr(viewer, "isAncestorOf") and viewer.isAncestorOf(w):
				return w
		except Exception:
			pass
		p = w
		while p is not None:
			if _same_widget(p, viewer):
				return w
			try:
				p = p.parentWidget()
			except Exception:
				break
	return None


# ////////////////////////////////////////////////////////////////
class _AgentChatInputFilter(QObject):
	"""Runs after the chat UI exists; last-installed filter sees Enter before C++ viewer filter."""

	def __init__(self, viewer, input_widget, output_widget):
		super().__init__(input_widget)
		self.viewer = viewer
		self.input_widget = input_widget
		self.output_widget = output_widget

	def eventFilter(self, obj, event):
		if event.type() == QEvent.KeyPress:
			ke = event
			if ke.key() in (Qt.Key_Return, Qt.Key_Enter) and not (ke.modifiers() & Qt.ShiftModifier):
				text = self.input_widget.toPlainText().strip()
				if text:
					self.input_widget.clear()
					self.viewer._append_agent_chat_user_bubble(self.output_widget, text)
					self.viewer._route_agent_chat_message(text)
				return True
		return False


# ////////////////////////////////////////////////////////////////
class PyViewer(Viewer):
	
	# constructor
	def __init__(self,title="PyViewer"):	
		super().__init__(title)
		self.render_palettes=False
		self._agent_llm_messages = []
		self._agent_chat_ui_wired = False
		self._agent_chat_wire_attempts = 0
		# Prefer Qt signal when the rebuilt bindings expose it (optional).
		self._agent_chat_via_signal = False
		try:
			self.agentChatUserSubmitted.disconnect()
			self.agentChatUserSubmitted.connect(self._route_agent_chat_message)
			self._agent_chat_via_signal = True
		except Exception:
			pass
		# Reliable path: hijack Send + Enter once Agent chat panel exists (no SWIG director / rebuild required).
		# Do not pass PyViewer as QTimer parent — SIP/SWIG Viewer is not accepted as QObject by PyQt.
		self._agent_chat_wire_timer = QTimer()
		self._agent_chat_wire_timer.setInterval(250)
		self._agent_chat_wire_timer.timeout.connect(self._try_wire_agent_chat_ui)
		self._agent_chat_wire_timer.start()
		# Chat dock is created lazily; the timer may expire before the user opens it — retry when the input is focused.
		app = QApplication.instance()
		if app is not None:
			app.focusChanged.connect(self._maybe_wire_agent_chat_on_focus)

	def _maybe_wire_agent_chat_on_focus(self, old, now):
		if self._agent_chat_ui_wired and self._agent_chat_widgets_alive():
			return
		if isinstance(now, QPlainTextEdit):
			self._agent_chat_wire_attempts = 0
			if not self._agent_chat_wire_timer.isActive():
				self._agent_chat_wire_timer.start()
			QTimer.singleShot(0, self._try_wire_agent_chat_ui)

	def _agent_chat_widgets_alive(self):
		return _agent_chat_widget_alive(getattr(self, "_agent_chat_input_widget", None))

	def _on_agent_chat_panel_destroyed(self, obj=None):
		# Viewer::clearAll() tears down the agent chat dock; C++ rebuilds it later — must re-wire Send/Enter.
		self._agent_chat_ui_wired = False
		self._agent_chat_input_widget = None
		self._agent_chat_output_widget = None
		self._agent_chat_input_filter = None
		if not self._agent_chat_wire_timer.isActive():
			self._agent_chat_wire_timer.start()

	def _try_wire_agent_chat_ui(self):
		if self._agent_chat_ui_wired and self._agent_chat_widgets_alive():
			self._agent_chat_wire_timer.stop()
			return
		if self._agent_chat_ui_wired and not self._agent_chat_widgets_alive():
			self._agent_chat_ui_wired = False
			self._agent_chat_input_widget = None
			self._agent_chat_output_widget = None
			self._agent_chat_input_filter = None
		self._agent_chat_wire_attempts += 1
		# Keep trying while the viewer runs (dock may appear only after user opens Agent chat).
		if self._agent_chat_wire_attempts > 200000:
			self._agent_chat_wire_timer.stop()
			return
		try:
			panel = _find_agent_chat_panel_under_viewer(self)
		except Exception:
			return
		if panel is None:
			return
		inputs = panel.findChildren(QPlainTextEdit)
		outputs = panel.findChildren(QTextEdit)
		if not inputs or not outputs:
			return
		inp, out = inputs[0], outputs[0]
		# Stop C++ Viewer::eventFilter from seeing Enter (would call submitAgentChatInput → builtin only).
		try:
			inp.removeEventFilter(self)
		except Exception:
			pass
		buttons = panel.findChildren(QPushButton)
		send_btns = [b for b in buttons if str(b.text()).strip().lower() in ("send", "ok", "submit")]
		if not send_btns and len(buttons) == 1:
			send_btns = buttons
		elif not send_btns and buttons:
			send_btns = [buttons[-1]]
		for b in send_btns:
			try:
				b.disconnect()
			except Exception:
				pass
			b.clicked.connect(lambda checked=False, inp=inp, out=out: self._agent_chat_send_clicked(inp, out))
		filt = _AgentChatInputFilter(self, inp, out)
		inp.installEventFilter(filt)
		filt.setParent(inp)
		self._agent_chat_input_filter = filt
		self._agent_chat_input_widget = inp
		self._agent_chat_output_widget = out
		try:
			panel.destroyed.connect(self._on_agent_chat_panel_destroyed)
		except Exception:
			pass
		self._agent_chat_ui_wired = True
		self._agent_chat_wire_timer.stop()

	def _append_agent_chat_user_bubble(self, out, text):
		if not out or not text:
			return
		esc = _html_escape(text).replace("\n", "<br/>")
		if out.toPlainText():
			c = QTextCursor(out.document())
			c.movePosition(QTextCursor.End)
			c.insertBlock()
		html = (
			'<table width="100%" cellspacing="0" cellpadding="0" border="0"'
			' style="margin:6px 0; border:none;"><tr><td align="right">'
			'<div style="display:inline-block; max-width:92%; text-align:left;'
			' background-color:#2563eb; color:#ffffff; padding:10px 14px; border-radius:16px;'
			' border:none; box-sizing:border-box;">'
			'<div style="font-size:12px; font-weight:600; color:#bfdbfe; margin:0 0 6px 0;">You</div>'
			'<div style="font-size:15px; line-height:1.5; word-break:normal; overflow-wrap:break-word;">'
			+ esc
			+ "</div></div></td></tr></table>"
		)
		c = QTextCursor(out.document())
		c.movePosition(QTextCursor.End)
		c.insertHtml(html)
		out.moveCursor(QTextCursor.End)

	def _append_agent_chat_assistant_bubble(self, out, text):
		"""Match Viewer.Gui.cpp agentChatAppendBubbleAgent (Agent bubble)."""
		if not out or not text:
			return
		esc = _html_escape(text).replace("\n", "<br/>")
		if out.toPlainText():
			c = QTextCursor(out.document())
			c.movePosition(QTextCursor.End)
			c.insertBlock()
		html = (
			'<table width="100%" cellspacing="0" cellpadding="0" border="0"'
			' style="margin:6px 0; border:none;"><tr><td align="left">'
			'<div style="display:inline-block; max-width:92%; text-align:left;'
			' background-color:#f8fafc; color:#334155; padding:10px 14px; border-radius:16px;'
			' border:1px solid #e2e8f0; box-sizing:border-box;">'
			'<div style="font-size:12px; font-weight:600; color:#64748b; margin:0 0 6px 0;">Agent</div>'
			'<div style="font-size:15px; line-height:1.5; word-break:normal; overflow-wrap:break-word;">'
			+ esc
			+ "</div></div></td></tr></table>"
		)
		c = QTextCursor(out.document())
		c.movePosition(QTextCursor.End)
		c.insertHtml(html)
		out.moveCursor(QTextCursor.End)

	def _agent_chat_send_clicked(self, inp, out):
		text = inp.toPlainText().strip()
		if not text:
			return
		inp.clear()
		self._append_agent_chat_user_bubble(out, text)
		self._route_agent_chat_message(text)

	def _append_agent_chat_line(self, text):
		"""Prefer the wired QTextEdit: C++ appendAgentChatLine can no-op if widgets.agent_chat_output is null; else log via printInfo."""
		out = getattr(self, "_agent_chat_output_widget", None)
		if _agent_chat_widget_alive(out):
			self._append_agent_chat_assistant_bubble(out, text)
			return
		fn = getattr(self, "appendAgentChatLine", None)
		if callable(fn):
			fn(text)
			return
		fn = getattr(Viewer, "appendAgentChatLine", None)
		if callable(fn):
			fn(self, text)
			return
		try:
			self.printInfo(text)
		except Exception:
			print(text, file=sys.stderr)

	def _call_viewer_builtin_chat(self, msg):
		"""Run C++ agentChatProcessBuiltinMessage (commands + XML). Prefer VisusGuiPy.i agentChatRunBuiltinMessage(std::string)."""
		fn = getattr(self, "agentChatRunBuiltinMessage", None)
		if callable(fn):
			fn(msg)
			return
		fn = getattr(Viewer, "agentChatRunBuiltinMessage", None)
		if callable(fn):
			fn(self, msg)
			return
		fn = getattr(self, "agentChatProcessBuiltinMessage", None)
		if callable(fn):
			fn(msg)
			return
		fn = getattr(Viewer, "agentChatProcessBuiltinMessage", None)
		if callable(fn):
			fn(self, msg)
			return
		base = super(PyViewer, self)
		fn = getattr(base, "agentChatMessageReceived", None)
		if callable(fn):
			fn(msg)
			return
		fn = getattr(Viewer, "agentChatMessageReceived", None)
		if callable(fn):
			fn(self, msg)
			return
		self._append_agent_chat_line(
			"Viewer builtin agent is not available: rebuild the VisusGuiPy target so "
			"agentChatRunBuiltinMessage is in the bindings (see Libs/swig/VisusGuiPy.i). "
			"Could not run: "
			+ ((msg if isinstance(msg, str) else str(msg))[:200])
		)

	def _route_agent_chat_message(self, text):
		"""Agent chat is LLM-first; C++ word parser only for /local, XML, or lines returned by the model."""
		msg = text if isinstance(text, str) else str(text)
		agent_llm = _load_agent_llm_module()
		if agent_llm is None:
			self._append_agent_chat_line(
				"OpenVisus agent_llm module is missing (copy Libs/swig/agent_llm.py next to viewer.py in the OpenVisus package folder, or rebuild)."
			)
			return
		try:
			if agent_llm.use_llm_for_agent_chat():
				agent_llm.handle_agent_chat(self, msg)
				return
		except Exception as ex:
			self._append_agent_chat_line("Agent LLM error: {}".format(ex))
			return
		agent_llm.prompt_configure_llm(self)

	def agentChatMessageReceived(self, message):
		if getattr(self, "_agent_chat_via_signal", False):
			return
		self._route_agent_chat_message(message)
	
	# run
	def run(self):
		bounds=self.getWorldBox()
		self.getGLCamera().guessPosition(bounds)	
		QApplication.exec()	
		
	# addVolumeRender
	def addVolumeRender(self, data, bounds):
		
		t1=Time.now()
		print("Adding Volume render...")	
		
		Assert(isinstance(data,numpy.ndarray))
		data=Array.fromNumPy(data,TargetDim=3)
		data.bounds=bounds
		
		node=RenderArrayNode()
		node.setName("RenderVolume")
		node.setLightingEnabled(False)
		node.setPaletteEnabled(False)
		node.setData(data)
		self.addNode(self.getRoot(), node)
		
		print("done in ",t1.elapsedMsec(),"msec")
		
		
	# addIsoSurface
	def addIsoSurface(self, field=None, second_field=None, isovalue=0, bounds=None):

		Assert(isinstance(field,numpy.ndarray))
		
		field=Array.fromNumPy(field,TargetDim=3)
		field.bounds=bounds

		print("Extracting isocontour...")
		t1=Time.now()
		mesh=MarchingCube(field,isovalue).run()
		print("done in ",t1.elapsedMsec(),"msec")
		
		if second_field is not None:
			mesh.second_field=Array.fromNumPy(second_field,TargetDim=3)
			mesh.second_field.bounds=bounds
					
		node=IsoContourRenderNode()	
		node.setName("RenderMesh")
		node.setMesh(mesh)
		self.addNode(self.getRoot(),node)
		
		print("Added IsoContourRenderNode")	

	# findQueryNodes
	def findQueryNodes(self):
		dataflow=self.getDataflow()
		return [QueryNode.castFrom(it) for it in dataflow.getNodesAsVector() if QueryNode.castFrom(it)]

	# findPaletteNodes
	def findPaletteNodes(self):
		dataflow=self.getDataflow()
		return [PaletteNode.castFrom(it) for it in dataflow.getNodesAsVector() if PaletteNode.castFrom(it)]

	# glRenderTexturedQuad
	def glRenderTexturedQuad(self, gl, texture=None, bounds=None):
		x1,y1,w,h=bounds
		x2,y2=x1+w,y1+h

		# draw a 2d quad
		GL_QUADS=0x0007
		mesh=GLMesh()
		mesh.begin(GL_QUADS);
		mesh.texcoord2(0,0);mesh.vertex(x1,y1)
		mesh.texcoord2(0,1);mesh.vertex(x2,y1)
		mesh.texcoord2(1,1);mesh.vertex(x2,y2)
		mesh.texcoord2(1,0);mesh.vertex(x1,y2)
		mesh.end()

		obj=GLPhongObject()
		obj.color=Color(255,255,0)
		obj.texture=GLTexture.createFromArray(Array.fromNumPy(texture,TargetDim=1))
		obj.mesh=mesh
		obj.glRender(gl)

	# pushHUD
	def pushHUD(self,gl):
		gl.pushFrustum()
		gl.setHud()
		gl.pushDepthTest(False)
		gl.pushBlend(True)

	# popHUD
	def popHUD(self,gl):
		gl.popBlend()
		gl.popDepthTest()
		gl.popFrustum()

	# glRenderPalette
	def glRenderPalette(self,gl, texture=None, stats=None,bounds=None, slot=0, default_w=32, default_h=600, default_border=30, disable_transparency=True):
		
		# automatic displacemente basing on slot
		if bounds is None:
			W=gl.getViewport().width
			H=gl.getViewport().height
			w,h,border=default_w,default_h,default_border
			bounds=[W-(slot+1)*(w+border),H-h-border,w,h]
			
		if disable_transparency:
			texture[:,3]=255 

		font_height=10
		x1,y1=bounds[0],bounds[1]
		x2,y2=x1+bounds[2],y1+bounds[3]
		self.glRenderTexturedQuad(gl, texture=texture, bounds=bounds)
		gl.glRenderScreenText(x1,y1-font_height,"{}".format(stats.computed_range.From if stats else 0.0),Color(255,255,255))
		gl.glRenderScreenText(x1,y2            ,"{}".format(stats.computed_range.To   if stats else 0.0),Color(255,255,255))

	# glRenderPalettes
	def glRenderPalettes(self,gl):
		for I, node in enumerate(self.findPaletteNodes()):
			node.setStatisticsEnabled(True) # I am assuming I want to see statistics
			stats=node.getLastStatistics()
			texture=Array.toNumPy(node.getPalette().toArray())
			self.glRenderPalette(gl,
								texture=texture, 
								stats=stats.components[0] if len(stats.components) else None, 
								slot=I)
		
	# glRenderNodes (overriding from Viewer)
	def glRenderNodes(self, gl):

		Viewer.glRenderNodes(self,gl)

		if self.render_palettes:
			self.pushHUD(gl)
			self.glRenderPalettes(gl)
			self.popHUD(gl)