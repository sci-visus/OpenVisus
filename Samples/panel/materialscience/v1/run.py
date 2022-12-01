import os,sys,time,datetime,threading,queue
import panel as pn
import numpy as np
import ipywidgets
import pandas as pd
import matplotlib
import matplotlib.pyplot as plt
from matplotlib.figure import Figure

import OpenVisus as ov

# this is needed to show matplot plots inside panel
# https://panel.holoviz.org/reference/panes/Matplotlib.html
pn.extension('ipywidgets')

# ////////////////////////////////////////////////////////////////////////////////////
COLORMAPS =  [
	'terrain', 'viridis', 'plasma', 'inferno','magma', 'cividis','ocean', 'gist_earth', 'gist_stern',
	'gnuplot', 'gnuplot2', 'CMRmap', 'cubehelix','brg','gist_rainbow', 'rainbow', 'jet', 'turbo', 
	'nipy_spectral','gist_ncar'
]

# ///////////////////////////////////////////////////////////////////
def ReadSlice(db, dir=0, offset=0, time=None, field=None, access=None, num_refinements=1, max_pixels=None, aborted=ov.Aborted()):

	def AlignSlice(db,box,H):
		maxh=db.getMaxResolution()
		bitmask=db.getBitmask().toString()
		delta=[1,1,1]
		for B in range(maxh,H,-1):
			bit=ord(bitmask[B])-ord('0')
			A,B,D=box[0][bit],box[1][bit],delta[bit]
			D*=2
			A=(A//D)*D
			B=(B//D)*D
			B=max(A+D,B)
			box[0][bit],box[1][bit],delta[bit]=A,B,D
			box[1][dir]=box[0][dir]+1 # it is a slice 
			
		num_pixels=[max(1,(box[1][I]-box[0][I])//delta[I]) for I in range(3)]
		return box, delta, num_pixels
 
	pdim=db.getPointDim()
	maxh=db.getMaxResolution()
	bitmask=db.getBitmask().toString()

	if time is None:
		time=db.getTime()
 
	if field is None:
		field=db.getField()
 
	# guess box
	W,H,D=[int(it) for it in db.getLogicSize()]
 
	box=[[0,0,0],[W,H,D]]
	box[0][dir]=offset
	box[1][dir]=offset+1
 
	# find end_resolution
	endh=maxh
	if max_pixels:
		for H in range(maxh,0,-1):
			box,delta,num_pixels=AlignSlice(db,box,H)
			# print("!!!",H,box,delta,num_pixels,f"{np.prod(num_pixels):,}",f"{max_pixels:,}")
			assert num_pixels[dir]==1
			if np.prod(num_pixels)<=max_pixels:
				endh=H
				break
 
	end_resolutions=[]
	for I in range(num_refinements):
		H=endh-pdim*I
		if H>=0:
			end_resolutions.insert(0,H)

	box,delta,num_pixels=AlignSlice(db,box, end_resolutions[0])
	query = db.createBoxQuery(ov.BoxNi(ov.PointNi([int(it) for it in box[0]]),  ov.PointNi([int(it) for it in box[1]])),  field ,  time, ord('r'), aborted)

	for H in end_resolutions:
		query.end_resolutions.push_back(H)

	db.beginBoxQuery(query)
	while query.isRunning():
		if not db.executeBoxQuery(access, query):   break
		data=ov.Array.toNumPy(query.buffer, bShareMem=False) 
		# print("*** Got Data","shape","#pixels",data.shape,np.prod(data.shape),"#maxpixels",max_pixels,"min",np.min(data),"max",np.max(data))
		h,w=[value for value in data.shape if value>1]
		data=data.reshape([h,w]) # from 3d to 2d
		yield data
		db.nextBoxQuery(query)


# ////////////////////////////////////////////////////////////////////////////////////
class MatPlotFigure:
  
	# constructor
	def __init__(self,data,debug_mode=False):
		self.fig = Figure()
		self.ax  = self.fig.subplots() 
		self.ax.autoscale(tight=True)
		self.ax.get_xaxis().set_visible(debug_mode)
		self.ax.get_yaxis().set_visible(debug_mode)
		self.img = self.ax.imshow(data)
		self.fig.colorbar(self.img, ax=self.ax)
		self.widget=pn.pane.Matplotlib(self.fig, dpi=144,sizing_mode='stretch_both', tight=True) 
		self.fig.tight_layout()

	# getWidth
	def getWidth(self):
		return self.widget.width

	# getHeight
	def getHeight(self):
		return self.widget.height

# ////////////////////////////////////////////////////////////////////////////////////
class Slicer:
  
	# constructor
	def __init__(self):
		self.db=None
		self.access=None
  
		self.lock          = threading.Lock()
		self.queue         = queue.Queue()
		self.aborted       = ov.Aborted()
		self.thread        = threading.Thread(target=self.workerLoop)
		self.get_new_data  = False
  
		self.fig       = MatPlotFigure(np.uint8(np.random.random((300,300))*255))
		self.colormap  = pn.widgets.Select(name='Colormap', options=COLORMAPS,value=COLORMAPS[0],width=100)
		self.direction = pn.widgets.Select(name='Direction', options={'X': 0, 'Y': 1, 'Z' : 2},value=2,width=50)
		self.offset    = pn.widgets.IntSlider(name='Offset', value=0, start=0, end=1024, sizing_mode="stretch_width")
		self.info      = pn.widgets.TextInput(name="Info",value="")

		self.colormap .param.watch(lambda evt: self.setColorMap(evt.new), 'value')
		self.direction.param.watch(lambda evt: self.setDirection(evt.new), 'value')  
		self.offset   .param.watch(lambda evt: self.setOffset(evt.new), 'value')  
		self.callback = pn.state.add_periodic_callback(self.onTimer, 100)
  
		self.app=pn.Column(
   		self.fig.widget,
			pn.Row(self.colormap,	self.direction, self.offset),
     	sizing_mode='stretch_both')
  
		self.thread.start()

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
		self.offset.value=int(dims[value]//2)
		self.readData()

	# setOffset
	def setOffset(self,value):
		self.offset.value=value
		self.readData()

	# setJobAborted
	def setJobAborted(self):
		self.aborted.setTrue()

	# hasJobAborted
	def hasJobAborted(self):
		ABORTED=ov.Aborted()
		ABORTED.setTrue()   
		return self.aborted.__call__()==ABORTED.__call__()

	# readData
	def readData(self):
		self.setJobAborted()
		self.get_new_data=True

	# onTimer
	def onTimer(self):
		# reschedule new job (but make sure the worker is in idle state)
		if self.get_new_data:
			max_pixels=self.fig.getWidth() * self.fig.getHeight()
			self.setJobAborted()
			self.waitForIdleWorker()
			self.queue.put([self.direction.value,self.offset.value, max_pixels])
			self.get_new_data=False

	# runInBackground
	def workerLoop(self):
		while True:
			direction,offset,max_pixels=self.queue.get()

			with self.lock:
				self.aborted=ov.Aborted()
				# print("Worker::got_job")
   
			I=0
			for data in ReadSlice(self.db, access=self.access, dir=direction, offset=offset,  num_refinements=3, max_pixels=max_pixels, aborted=self.aborted):
				
				# query failed
				if data is None:
					break

				if self.hasJobAborted():
						# print("JOB ABORTED")
						break

				m,M=np.amin(data),np.amax(data)
 
				# assign the new data to display
				with self.lock:
					self.min = min(m,self.min if self.min is not None else m)
					self.max = max(M,self.max if self.max is not None else M)
					self.info.value = f"Displaying data {I} {data.shape} offset={offset}"
					print(self.info.value)
     
					self.fig.img.set_cmap(self.colormap.value)
					self.fig.img.set_data(data)
					# see https://stackoverflow.com/questions/10970492/matplotlib-no-effect-of-set-data-in-imshow-for-the-plot
					# self.fig.img.autoscale()
					self.fig.img.set_clim(vmin=self.min, vmax=self.max)
					self.fig.ax.set_title(f"{self.title} {data.shape} {self.min:.2f} {self.max:.2f}")
					self.fig.widget.param.trigger('object') # vert important
  
					# print("Worker::display_data",I,data.shape)
					I+=1

				# break
				time.sleep(0.1)

			# let the main task know I am done
			# print("Worker::done_job")
			self.queue.task_done()



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
			slicer=Slicer()
			self.slices.append(slicer)
			self.setDataset(target, scan=scans[target % len(scans)], direction=2)

		#self.type  = pn.widgets.Select(name='Type', options=["Reconstruction","Segmentation"],value="Reconstruction",width=100)
		#self.type .param.watch(lambda evt: self.setType, 'value')

		self.app=pn.GridBox(
			
			pn.pane.Matplotlib(self.fig, sizing_mode='stretch_both', interactive=True,tight=False ,high_dpi  =True),
			self.slices[0].app,
			self.slices[1].app,
			self.slices[2].app,
			nrows=2, ncols=2,
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
	exp.app.servable()
