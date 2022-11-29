import os,sys,time,datetime,threading,queue,random,copy,math

import panel as pn
import panel.pane.plot

import numpy as np
import ipywidgets
import pandas as pd
import matplotlib
import matplotlib.pyplot as plt
from matplotlib import cm
from matplotlib.figure import Figure
import ipympl.backend_nbagg

from PIL import Image

import OpenVisus as ov

# https://panel.holoviz.org/reference/panes/Matplotlib.html
pn.extension()
pn.config.css_files.append("https://cdnjs.cloudflare.com/ajax/libs/font-awesome/4.5.0/css/font-awesome.css")
pn.extension(sizing_mode="stretch_width")



# ///////////////////////////////////////////////////////////////////
def ReadSlice(db, logic_box=None, dir=0, offset=0, time=None, field=None, access=None, num_refinements=1, max_pixels=None, aborted=ov.Aborted()):

	def AlignSlice(db,box,H):
		maxh=db.getMaxResolution()
		bitmask=db.getBitmask().toString()
		delta=[1,1,1]
		for B in range(maxh,H,-1):
			bit=ord(bitmask[B])-ord('0')
			A,B,D=box[0][bit],box[1][bit],delta[bit]
			D*=2
			A=int(math.floor(A/D))*D
			B=int(math.ceil (B/D))*D
			B=max(A+D,B)
			box[0][bit],box[1][bit],delta[bit]=A,B,D
			box[1][dir]=box[0][dir]+1 # it is a slice 
			
		num_pixels=[max(1,(box[1][I]-box[0][I])//delta[I]) for I in range(3)]
		return box, delta, num_pixels
 
	pdim=db.getPointDim()
	maxh=db.getMaxResolution()
	bitmask=db.getBitmask().toString()
	dims=db.getLogicSize()

	if time is None:
		time=db.getTime()
 
	if field is None:
		field=db.getField()
 
	# guess box
	if logic_box is None:
		W,H,D=[int(it) for it in db.getLogicSize()]
		logic_box=[[0,0,0],[W,H,D]]
  
	p1,p2=logic_box
  
	# check logic box
	for I in range(3):
		if p1[I]<0      : p1[I]=0
		if p2[I]>dims[I]: p2[I]=dims[I]
		assert p1[I]<p2[I]
		p1[I],p2[I]=int(math.floor(p1[I])),int(math.ceil(p2[I]))
		if p2[I]>dims[I]: p2[I]=dims[I]
		
	assert offset>=0 and offset<dims[dir]
	p1[dir]=offset+0
	p2[dir]=offset+1
	logic_box=(p1,p2)
 
	# find end_resolution
	endh=maxh
	if max_pixels:
		for H in range(maxh,0,-1):
			logic_box,delta,num_pixels=AlignSlice(db,logic_box,H)
			# print("!!!",H,logic_box,delta,num_pixels,f"{np.prod(num_pixels):,}",f"{max_pixels:,}")
			assert num_pixels[dir]==1
			if np.prod(num_pixels)<=max_pixels*1.25:
				endh=H
				break
 
	end_resolutions=[]
	for I in range(num_refinements):
		H=endh-pdim*I
		if H>=0:
			end_resolutions.insert(0,H)

	logic_box,delta,num_pixels=AlignSlice(db,logic_box, end_resolutions[0])
	box_ni=ov.BoxNi(ov.PointNi([int(it) for it in logic_box[0]]),  ov.PointNi([int(it) for it in logic_box[1]]))
	query = db.createBoxQuery(box_ni,  field ,  time, ord('r'), aborted)

	for H in end_resolutions:
		query.end_resolutions.push_back(H)

	print("Begin query",logic_box)
	db.beginBoxQuery(query)
	while query.isRunning():
		if not db.executeBoxQuery(access, query):   break
		data=ov.Array.toNumPy(query.buffer, bShareMem=False) 
		# print("*** Got Data","shape","#pixels",data.shape,np.prod(data.shape),"#maxpixels",max_pixels,"min",np.min(data),"max",np.max(data))
		h,w=[value for value in data.shape if value>1]
		data=data.reshape([h,w]) # from 3d to 2d
		yield logic_box, data
		db.nextBoxQuery(query)


# ////////////////////////////////////////////////////////////////////////////////////
# https://gist.github.com/MarcSkovMadsen/756fe0cf392a35c3c826194b831ad1a3
# see # https://stackoverflow.com/questions/11551049/matplotlib-plot-zooming-with-scroll-wheel
class OpenVisusSlicer:

	COLORMAPS =  [
		'viridis', 'terrain', 'plasma', 'inferno','magma', 'cividis','ocean', 'gist_earth', 'gist_stern',
		'gnuplot', 'gnuplot2', 'CMRmap', 'cubehelix','brg','gist_rainbow', 'rainbow', 'jet', 'turbo', 
		'nipy_spectral','gist_ncar'
	]
  
	# constructor
	def __init__(self):
		self.db=None
		self.access=None
  
		self.lock          = threading.Lock()
		self.queue         = queue.Queue()
		self.aborted       = ov.Aborted()
		self.thread        = threading.Thread(target=self.workerLoop)
		self.get_new_data  = False
  
		# create the plot
		self.fig = Figure()
		W,H=256,256 # this are fake dimension that will be changed
		self.axes  = self.fig.add_subplot(111, xlim=(0,W), ylim=(0,H), autoscale_on=False) 
		self.axes.get_xaxis().set_visible(True)
		self.axes.get_yaxis().set_visible(True)
		self.img = self.axes.imshow(np.uint8(np.random.random((W,H))*255), extent=(0,W,0,H))
		self.fig.colorbar(self.img, ax=self.axes)
		self.plot=panel.pane.plot.Matplotlib(self.fig, dpi=144,sizing_mode='stretch_both', tight=True, interactive=True)
		self.plot._get_widget=self._get_widget
		self.fig.tight_layout() 
		self.mouse_limit = None
		self.mouse_down  = None
  
		self.colormap  = pn.widgets.Select(name='Colormap', options=self.COLORMAPS,value=self.COLORMAPS[0],width=100)
		self.direction = pn.widgets.Select(name='Direction', options={'X': 0, 'Y': 1, 'Z' : 2},value=2,width=50)
		self.offset    = pn.widgets.IntSlider(name='Offset', value=0, start=0, end=1024, sizing_mode="stretch_width")
		self.info      = pn.widgets.TextInput(name="Info",value="")

		self.colormap .param.watch(lambda evt: self.setColorMap(evt.new), 'value')
		self.direction.param.watch(lambda evt: self.setDirection(evt.new), 'value')  
		self.offset   .param.watch(lambda evt: self.setOffset(evt.new), 'value')  
		self.callback = pn.state.add_periodic_callback(self.onTimer, 100)
  
		self.main_layout=pn.Column(
			#pn.pane.Markdown("""![NSDF](https://nationalsciencedatafabric.org/assets/images/logo.png)""",height=200),
			#pn.pane.PNG('https://nationalsciencedatafabric.org/assets/images/logo.png', width=100),
   		self.plot,
			pn.layout.HSpacer(height=600),  # panel bug
			pn.Row(self.colormap,	self.direction, self.offset),
   		# self.info,
     	sizing_mode='stretch_both')
  
		self.thread.start()

	# repaint
	def repaint(self):
		self.axes.figure.canvas.draw_idle()

	# project
	def project(self,value):
		ret=(list(value[0]),list(value[1]))
		del ret[0][self.direction.value]
		del ret[1][self.direction.value]
		return ret

	# unproject
	def unproject(self,value):
		ret=(list(value[0]),list(value[1]))
		ret[0].insert(self.direction.value, self.offset.value+0)
		ret[1].insert(self.direction.value, self.offset.value+1)
		return ret
		
	# getLogicBox
	def getLogicBox(self):
		x,y=self.axes.get_xlim(),get_ylim()
		return self.unproject(((x[0],y[0]),(x[1],y[1])))

	# drawLines
	def renderLines(self, points, marker='o'):
		self.axes.lines.clear()
		self.plot=self.axes.plot([p[0] for p in points], [p[1] for p in points], marker = marker)
		self.repaint()

	# renderData
	def renderData(self, data, logic_box, range=(0,1), cmap='gray'):
		self.img.set_cmap(cmap)
		self.img.set_data(np.flip(data,axis=0))
		self.img.set_clim(vmin=range[0], vmax=range[1])
		(x1,y1),(x2,y2)=logic_box[0],logic_box[1]
		self.img.set_extent((x1,x2,y1,y2))
		self.plot.param.trigger('object') # very important
  
	# override _get_widget (to make it interactive)
	def _get_widget(self, fig):
		matplotlib.use(getattr(matplotlib.backends, "backend", "agg"))
		canvas = ipympl.backend_nbagg.Canvas(fig)
		fig.patch.set_alpha(0)
		manager = ipympl.backend_nbagg.FigureManager(canvas, 0)
		if ipympl.backend_nbagg.is_interactive():
			fig.canvas.draw_idle()
  
		def onClose():
			canvas.mpl_disconnect(cid)
			from matplotlib._pylab_helpers import Gcf
			Gcf.destroy(manager)    
   
		cid = canvas.mpl_connect("close_event",   onClose)
		fig.canvas.mpl_connect("button_press_event",   self.onMousePress)
		fig.canvas.mpl_connect("button_release_event", self.onMouseRelease)
		fig.canvas.mpl_connect("motion_notify_event",  self.onMouseMotion)
		fig.canvas.mpl_connect('scroll_event',         self.onMouseWheel)
		return manager

	# onMousePress
	def onMousePress(self,event):
		if event.inaxes != self.axes: return
		self.mouse_limit=[self.axes.get_xlim(),self.axes.get_ylim()]
		self.mouse_down=[event.xdata, event.ydata]

	# onMouseMotion
	def onMouseMotion(self,event):
		if self.mouse_down is None or self.mouse_limit is None: return
		if event.inaxes != self.axes: return
		self.mouse_limit[0] -= (event.xdata - self.mouse_down[0])
		self.mouse_limit[1] -= (event.ydata - self.mouse_down[1])
		self.setLogicBox(self.unproject((
    	(self.mouse_limit[0][0],self.mouse_limit[1][0]),
     	(self.mouse_limit[0][1],self.mouse_limit[1][1]))))

	# onMouseRelease
	def onMouseRelease(self,event):
		self.mouse_down = None
		self.mouse_limit = None

	# onWheel
	def onMouseWheel(self, event, base_scale = 1.1):
		if event.button != 'down' and event.button != 'up': return
		vs = base_scale if event.button=="down" else 1 / base_scale
		X,Y= self.axes.get_xlim(),self.axes.get_ylim()
		cx,cy = event.xdata,event.ydata
		w = X[1] - X[0]
		h = Y[1] - Y[0]
		relx = (X[1] - cx)/w
		rely = (Y[1] - cy)/h
		self.setLogicBox(self.unproject((
			[cx - w * vs * (1-relx), cy - h * vs * (1-rely)],
   		[cx + w * vs * (  relx), cy + h * vs * (  rely)])))

	# waitForIdleWorker
	def waitForIdleWorker(self):
		self.queue.join()  
  
	# setDataset
	def setDataset(self,db, access, direction=2,title=""):
		self.setJobAborted()
		self.waitForIdleWorker()
		self.min,self.max=None,None
		self.title=title
		self.db=db
		self.access=access 
		self.setDirection(direction)  

	# setJobAborted
	def setJobAborted(self):
		self.aborted.setTrue()

	# hasJobAborted
	def hasJobAborted(self):
		ABORTED=ov.Aborted()
		ABORTED.setTrue()   
		return self.aborted.__call__()==ABORTED.__call__()

	# runInBackground
	def workerLoop(self):

		while True:
			logic_box, direction,offset,max_pixels=self.queue.get()

			with self.lock:
				self.aborted=ov.Aborted()
				print("Worker::got_job",logic_box)
   
			I=0
			for logic_box, data in ReadSlice(self.db, access=self.access, logic_box=logic_box, dir=direction, offset=offset,  num_refinements=4, max_pixels=max_pixels, aborted=self.aborted):
				
				# query failed
				if data is None:
					break

				if self.hasJobAborted():
						print("JOB ABORTED")
						break

				m,M=np.amin(data),np.amax(data)
 
				# assign the new data to display
				with self.lock:
					self.min = min(m,self.min if self.min is not None else m)
					self.max = max(M,self.max if self.max is not None else M)
					print("renderData",self.project(logic_box), data.shape,f"current_pixels={np.prod(data.shape):,}",f"max_pixels={max_pixels:,}")
					self.renderData(data, logic_box=self.project(logic_box), range=(self.min,self.max), cmap=self.colormap.value)
					# print("Worker::display_data",I,data.shape)
					I+=1

				# break
				time.sleep(0.1)

			# let the main task know I am done
			# print("Worker::done_job")
			self.queue.task_done()

	# readData
	def readData(self):
		self.setJobAborted()
		self.get_new_data=True

	# onTimer
	def onTimer(self):

		# reschedule new job (but make sure the worker is in idle state)
		if self.get_new_data:
			max_pixels=self.plot.width * self.plot.height
			self.setJobAborted()
			self.waitForIdleWorker()
			self.queue.put([self.logic_box, self.direction.value,self.offset.value, max_pixels])
			self.get_new_data=False

	# setColorMap
	def setColorMap(self,value):
		self.colormap.value=value
		self.readData()
  
	# setDirection
	def setDirection(self,value):
		dims=[int(it) for it in self.db.getLogicSize()]
		self.direction.value = value
		self.offset.start = 0
		self.offset.end   = int(dims[value]-1)
		self.offset.value = int(dims[value]//2)
		self.setLogicBox(([0,0,0],dims))
		self.readData()

	# setOffset
	def setOffset(self,value):
		self.offset.value=value
		self.readData()

	# setLogicBox
	def setLogicBox(self,value):
		print("setLogicBox",value)
		self.logic_box=value
		proj1,proj2=self.project(value)
		self.axes.set_xlim((proj1[0],proj2[0]))
		self.axes.set_ylim((proj1[1],proj2[1]))
		self.readData()
		self.repaint()


# //////////////////////////////////////////////////////////////////////////////////////
# # https://stackoverflow.com/questions/69619157/matplotlib-clickable-plot-in-panel-row
class Experiment:

	# constructor
	def __init__(self,scans,df):
		self.scans=scans
		self.url=None
		self.slices=[]
		self.fig = Figure()
		self.ax  = self.fig.subplots() 

		# diagram
		self.ax.plot(df.time, df.stress)

		# display scans as markers
		for scan in scans:
			x,y=scan["pos"]
			artist = self.ax.plot(x, y, marker="v", picker=True, pickradius=13, color='red',  markersize=13)[0]
			self.ax.text(x, y, scan["id"],fontsize="x-small",horizontalalignment="center")
			artist.obj = scan

		self.fig.tight_layout()
		# https://github.com/holoviz/panel/blob/5f44b57dd1f1352822eb94342c0390527ce6c422/panel/pane/plot.py
		# https://github.com/matplotlib/ipympl/issues/239
		self.fig.set_size_inches(1, 1) 

		self.fig.canvas.callbacks.connect('pick_event', self.onPickEvent)

		self.slices=[]
		for target in range(min(3,len(scans))):
			slicer=OpenVisusSlicer()
			self.slices.append(slicer)
			self.setDataset(target, scan=scans[target % len(scans)], direction=2)
			break

		self.main_layout=pn.GridBox(
			None, #pn.pane.Matplotlib(self.fig, sizing_mode='stretch_both', interactive=True,tight=False ,high_dpi  =True),
			self.slices[0].main_layout if len(self.slices)>=1 else None,
			self.slices[1].main_layout if len(self.slices)>=2 else None,
			self.slices[2].main_layout if len(self.slices)>=3 else None,
			nrows=1, ncols=1,
			sizing_mode='stretch_both')

	# https://stackoverflow.com/questions/69619157/matplotlib-clickable-plot-in-panel-row
	def onPickEvent(self,evt):
		scan=evt.artist.obj
  
		# too fast, cannot be
		if getattr(self,"last_pick",None) is not None and (time.time()-self.last_pick)<1.0:
			return 

		self.target=getattr(self,"target",0) % len(self.slices)
		self.setDataset(self.target,scan)
		self.target+=1
		self.last_pick=time.time()

	# setDataset
	def setDataset(self, target, scan,direction=2):
		VISUS_CACHE_DIR=os.environ["VISUS_CACHE_DIR"]
		slicer=self.slices[target]
		print(f"setDataset target={target} scan={scan} direction={direction}")
		url=scan["url"]
		print("Loading dataset",url,"...")
		db= ov.LoadDataset(url)
		print("done")
		from urllib.parse import urlparse
		parsed=urlparse(url)
		filename_template=os.path.join(os.path.splitext(parsed.path)[0], "$(time)/$(field)/$(block:%016x:%04x).bin.zz") # problem with the final zz that is non-default pattern
		access=db.createAccessForBlockQuery(ov.StringTree.fromString(f"""
				<access type='multiplex'>
					<access type='DiskAccess' chmod='rw' compression="zip" filename_template="{VISUS_CACHE_DIR}{filename_template}" />	
					<access type="CloudStorageAccess" filename_template="{filename_template}" />
				</access>
			"""))
		slicer.setDataset(db, access,direction=direction,title=scan["id"])
		slicer.scan=scan # keep track
		

# //////////////////////////////////////////////////////////////////////////////////////
if True:
  
	if False:
		url="https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112603.h5/r/idx/1mb/visus.idx"
		db=ov.LoadDataset(url)
		print(db.getLogicSize())
		sys.exit(0)

	# # python3 -m OpenVisus fix-range --dataset /usr/sci/cedmav/data/Pania_2021Q3_in_situ_data/idx/fly_scan_id_112509.h5/reconstructions/modvisus/visus.idx
	"""
for it in 112509 112512 112515 112517 112520  112524 112526 112528 112530 112532; do
   mkdir -p "/mnt/h/My Drive/visus_dataset/Pania_2021Q3_in_situ_data/idx/fly_scan_id_${it}.h5/reconstructions/"
   rsync -azP \
      "scrgiorgio@atlantis.sci.utah.edu:/usr/sci/cedmav/data/Pania_2021Q3_in_situ_data/idx/fly_scan_id_${it}.h5/reconstructions/modvisus/*" \
      "/mnt/h/My Drive/visus_dataset/Pania_2021Q3_in_situ_data/idx/fly_scan_id_${it}.h5/reconstructions/modvisus/"
done
	"""
	# 
	scans=[
		{"id":"112509","pos":(530 ,1.0)},
		{"id":"112512","pos":(1400,6.5)},
		{"id":"112515","pos":(2100,7.0)},
		{"id":"112517","pos":(2400,11)},
		{"id":"112520","pos":(3100,14)},
		{"id":"112522","pos":(3400,15)},
		{"id":"112524","pos":(3600,17)},
		{"id":"112526","pos":(3800,18)},
		{"id":"112528","pos":(4100,19)},
		{"id":"112530","pos":(4400,21)},
		{"id":"112532","pos":(4700,22)},
	]
 
	#for k,v in os.environ.items():
	#	print(k,v)
 
	for it in scans:
		it["url"]=f"https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_{it['id']}.h5/r/idx/1mb/visus.idx" 

	print("Reading CSV file...")
	csv=pd.read_csv("https://raw.githubusercontent.com/sci-visus/OpenVisus/master/Samples/panel/materialscience/data.csv")
	print("DONE")
	exp=Experiment(scans,csv)
 
	pn.template.FastListTemplate(
			site = "OpenVisus", 
			title = "slicers",
			main = [exp.main_layout]
		).servable() 
