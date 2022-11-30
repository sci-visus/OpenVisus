import os,sys,time,datetime,threading,queue,random,copy,math
from urllib.parse import urlparse

import numpy as np
import pandas as pd

import numpy as np,random

import bokeh.io
import bokeh.events
import bokeh.models
import bokeh.plotting 

import OpenVisus as ov

# ///////////////////////////////////////////////////////////////////
def ReadSlice(db, logic_box=None, dir=0, offset=0, time=None, field=None, access=None, num_refinements=1, max_pixels=None, aborted=ov.Aborted()):

	assert ov

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
			if np.prod(num_pixels)<=max_pixels*1.10:
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

	# print("Begin query",logic_box)
	db.beginBoxQuery(query)
	while query.isRunning():
		if not db.executeBoxQuery(access, query):   break
		data=ov.Array.toNumPy(query.buffer, bShareMem=False) 
		# print("*** Got Data","shape","#pixels",data.shape,np.prod(data.shape),"#maxpixels",max_pixels,"min",np.min(data),"max",np.max(data))
		h,w=[value for value in data.shape if value>1]
		data=data.reshape([h,w]) # from 3d to 2d
		yield logic_box, data
		db.nextBoxQuery(query)

PALETTES=[
"Greys256",
"Inferno256",
"Magma256",
"Plasma256",
"Viridis256",
"Cividis256",
"Turbo256"
]


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
		self.num_refinements = 4
		self.cache_dir       = cache_dir=os.environ["VISUS_CACHE_DIR"]
  
		self.last_job      = None
		self.current_img   = None
		self.next_img      = None
		self.last_size     = None

		self.ABORTED=ov.Aborted()
		self.ABORTED.setTrue() 
  
		self.direction = bokeh.models.Select(title='Direction', options=[('0','X'),('1','Y'),('2','Z')],value='2',width=80,sizing_mode="fixed")
		self.direction.on_change ("value",lambda attr, old, new: self.setDirection(int(new)))  
  
		self.offset    = bokeh.models.Slider(title='Offset', value=0, start=0, end=1024, sizing_mode="stretch_width")
		self.offset   .on_change ("value",lambda attr, old, new: self.setOffset(int(new))) 
	
		self.callback = bokeh.io.curdoc().add_periodic_callback(self.onTimer, 100)  
  
		self.color_mapper = bokeh.models.LinearColorMapper() # LogColorMapper
		self.color_mapper.palette="Viridis256"
		self.color_mapper.low=0
		self.color_mapper.high=65535.0 # TODO numbers are hardcoded 
		
		self.plot = bokeh.plotting.figure(active_scroll = "wheel_zoom")
		# self.plot.title=
		self.plot.x_range=bokeh.models.Range1d(0,1024)
		self.plot.y_range=bokeh.models.Range1d(0, 768)
		self.plot.toolbar_location="below"
		self.plot.sizing_mode = 'stretch_both' 

		# events
		self.plot.on_event(bokeh.events.MouseWheel, self.onMouseWheel)
		self.plot.on_event(bokeh.events.DoubleTap, self.onDoubleTap)
  
		data={"image": [np.random.random((300,300))*255], "x":[0], "y":[0], "dw":[256], "dh":[256]}
		self.source_image = bokeh.models.ColumnDataSource(data=data)
		self.image = self.plot.image("image", source=self.source_image, x="x", y="y", dw="dw", dh="dh", color_mapper=self.color_mapper)  
  
		self.color_bar = bokeh.models.ColorBar(color_mapper=self.color_mapper)
		self.plot.add_layout(self.color_bar, 'right') 

		self.layout=bokeh.layouts.column(
   		self.plot,
			bokeh.layouts.row(self.direction, self.offset,sizing_mode='stretch_width'),
     	sizing_mode='stretch_both')
  
		self.thread.start()

	# onMouseWheel
	def onMouseWheel(self,evt):
		print("onMouseWheel",evt)

	# onDoubleTap
	def onDoubleTap(self,evt):
		print("onDoubleTap",evt)

	# hasJobAborted
	def hasJobAborted(self):
		return self.aborted.__call__()==self.ABORTED.__call__()

	# project
	def project(self,value):
		p1,p2=(list(value[0]),list(value[1]))
		dir=self.getDirection()
		del p1[dir]
		del p2[dir]
		return (p1,p2)

	# unproject
	def unproject(self,value):
		p1,p2=(list(value[0]),list(value[1]))
		dir=self.getDirection()
		p1.insert(dir, self.offset.value+0)
		p2.insert(dir, self.offset.value+1)
		return (p1,p2)
		
	# getLogicBox
	def getLogicBox(self):
		x1,x2=self.plot.x_range.start, self.plot.x_range.end
		y1,y2=self.plot.y_range.start, self.plot.y_range.end
		return self.unproject(((x1,y1),(x2,y2)))

	# setLogicBox
	def setLogicBox(self,value):
		proj=self.project(value)
		self.plot.x_range=bokeh.models.Range1d(*[x for x,y in proj])
		self.plot.y_range=bokeh.models.Range1d(*[y for x,y in proj])  
		self.refresh()

	# renderData
	def renderData(self, data, logic_box):
		(x1,y1),(x2,y2)=self.project(logic_box)
		self.source_image.data = {"image":[data], "x":[x1], "y":[y1], "dw":[x2-x1], "dh":[y2-y1]}
		
	# drawLines
	def renderLines(self, points, marker='o'):
		self.plot.line(x=[x for x,y in points], y=[y for x,y in points], marker = marker)

	# setTime
	def setTime(self, url):
		self.setJobAborted()
		self.waitForIdleWorker()
		print("setTime",url)
		self.db= ov.LoadDataset(url)
		assert self.db
		parsed=urlparse(url)
		filename_template=os.path.join(os.path.splitext(parsed.path)[0], "$(time)/$(field)/$(block:%016x:%04x).bin.zz") # problem with the final zz that is non-default pattern
		self.access=self.db.createAccessForBlockQuery(ov.StringTree.fromString(f"""
				<access type='multiplex'>
					<access type='DiskAccess' chmod='rw' compression="zip" filename_template="{self.cache_dir}{filename_template}" />	
					<access type="CloudStorageAccess" filename_template="{filename_template}" />
				</access>
			"""))
		self.refresh()

	# setDataset
	def setDataset(self,  url=None, title="Slicer", direction=2):
		self.title=title
		self.setTime(url)
		self.setDirection(direction) 

	# setPalette
	def setPalette(self,value):
		self.color_mapper.palette=value
		self.refresh()
  
	# setDirection
	def setDirection(self,value):
		dims=[int(it) for it in self.db.getLogicSize()]
		self.direction.value = str(value)
		self.offset.start = 0
		self.offset.end   = int(dims[value]-1)
		self.offset.value = int(dims[value]//2)
		self.setLogicBox(([0,0,0],dims))
		self.refresh()

	# getDirection
	def getDirection(self):
		return int(self.direction.value)

	# getOffset
	def getOffset(self):
		return self.offset.value

	# setOffset
	def setOffset(self,value):
		self.offset.value=value
		self.refresh()

	# setJobAborted
	def setJobAborted(self):
		self.aborted.setTrue()

	# refresh
	def refresh(self):
		self.setJobAborted()
		self.last_job=None

	# onTimer
	def onTimer(self):

		# new image to display?
		if True:
			data,logic_box=None,None
			with self.lock:
				if self.next_img:
					data,logic_box=self.next_img 
					self.next_img=None
			if data is not None:
	 			self.renderData(data, logic_box)
   
		logic_box=self.getLogicBox()
		dir=self.getDirection()
		offset=self.getOffset()
		max_pixels=(self.plot.inner_width,self.plot.inner_height)
  
		key=f"logic_box={logic_box} dir={dir} offset={offset} max_pixels{max_pixels}"
		if key==self.last_job:
			return
 
		self.setJobAborted()
		self.waitForIdleWorker()
		self.queue.put([logic_box, dir, offset, max_pixels,self.num_refinements])
		self.last_job=key

	# waitForIdleWorker
	def waitForIdleWorker(self):
		self.queue.join()  

	# runInBackground
	def workerLoop(self):

		while True:
			logic_box, direction,offset,max_pixels,num_refinements=self.queue.get()

			with self.lock:
				self.aborted=ov.Aborted()
				print("\nWorker::got nee job",logic_box)
   
			I=0
			for logic_box, data in ReadSlice(self.db, access=self.access, logic_box=logic_box, dir=direction, offset=offset,  num_refinements=num_refinements, max_pixels=int(np.prod(max_pixels)), aborted=self.aborted):
				
				# query failed
				if data is None:
					break

				if self.hasJobAborted():
						break

				self.next_img=(data,logic_box)
				print(f"Got data {I}/{num_refinements} {data.shape} {max_pixels}")
					
				I+=1
				time.sleep(0.001)

			# let the main task know I am done
			print("Worker::done_job")
			self.queue.task_done()


# //////////////////////////////////////////////////////////////////////////////////////
class PlotScans:

	# constructor
	def __init__(self,scans,df):

		self.plot = bokeh.plotting.figure(x_axis_label='time', y_axis_label='stress',tools="tap")
		self.plot.title.text = "Experiment"
		self.plot.title.align = "center"
		self.plot.title.text_font_size = "20px"

		self.plot.line(df.time, df.stress, color="blue")
		x = [scan["pos"][0] for scan in scans]
		y = [scan["pos"][1] for scan in scans]
		self.plot_source = bokeh.models.ColumnDataSource(dict(x=x, y=y, markers=["inverted_triangle"]*len(x)))
		self.glyph = bokeh.models.Scatter(x="x", y="y", size=20, fill_color="#74add1", marker="markers")
 
		self.markers=self.plot.add_glyph(self.plot_source, self.glyph)
  
		self.layout=self.plot
  
  # enableMarkerSelection
	def enableMarkerSelection(self,fn):
		self.markers.data_source.selected.on_change('indices', lambda attr,old,new: fn(int(new[0])))


# //////////////////////////////////////////////////////////////////////////////////////
class Slices:

	# constructor
	def __init__(self,scans,df):
		self.scans=scans
		self.slices=[]
  
		plot_scans=PlotScans(scans,df)
		plot_scans.enableMarkerSelection(lambda index: self.setTime(index))
  
		self.palette  = bokeh.models.Select(title='Palette',  options=PALETTES,value='Viridis256')
		self.palette .on_change ("value",lambda attr, old, new: self.setPalette(new))  
  
		self.time = bokeh.models.Slider(title='Time', value=0, start=0, end=len(scans)-1, sizing_mode="stretch_width")
		self.time.on_change ("value",lambda attr, old, new: self.setTime(int(new)))
  
		for target in range(min(3,len(scans))):
			slicer=Slicer()
			self.slices.append(slicer)
			scan=scans[target % len(scans)]
			slicer.setDataset(scan["url"], scan["id"], direction=2-(target % 3))

		slice_layouts=[slice.layout for slice in self.slices]
		if len(self.slices)==1:
			central=bokeh.layouts.grid( children=slice_layouts, nrows=1,ncols=1, sizing_mode='stretch_both')    
		else:
			central=bokeh.layouts.grid( children=[plot_scans.layout] + slice_layouts,nrows=2,ncols=2, sizing_mode='stretch_both')

		self.layout=bokeh.layouts.Column(
			bokeh.layouts.Row(self.palette, self.time, sizing_mode='stretch_width'),
    	central, 
     	sizing_mode='stretch_both')
  
		self.setTime(0)

	# setTime
	def setTime(self, index):
		scan=self.scans[index]
		print("Setting scan",scan)
		for slice in self.slices:
			slice.setTime(scan["url"])
   
	# setPalette
	def setPalette(self,value):
		self.palette.value=value
		for slice in self.slices:
			slice.setPalette(value)

# //////////////////////////////////////////////////////////////////////////////////////
if True:

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
		{"id":"112509", "pos":(530 ,1.0), "url": "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112509.h5/r/idx/1mb/visus.idx"},
		{"id":"112512", "pos":(1400,6.5), "url": "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112512.h5/r/idx/1mb/visus.idx"},
		{"id":"112515", "pos":(2100,7.0), "url": "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112515.h5/r/idx/1mb/visus.idx"},
		{"id":"112517", "pos":(2400,11),  "url": "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112517.h5/r/idx/1mb/visus.idx"},
		{"id":"112520", "pos":(3100,14),  "url": "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112520.h5/r/idx/1mb/visus.idx"},
		{"id":"112522", "pos":(3400,15),  "url": "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112522.h5/r/idx/1mb/visus.idx"},
		{"id":"112524", "pos":(3600,17),  "url": "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112524.h5/r/idx/1mb/visus.idx"},
		{"id":"112526", "pos":(3800,18),  "url": "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112526.h5/r/idx/1mb/visus.idx"},
		{"id":"112528", "pos":(4100,19),  "url": "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112528.h5/r/idx/1mb/visus.idx"},
		{"id":"112530", "pos":(4400,21),  "url": "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112530.h5/r/idx/1mb/visus.idx"},
		{"id":"112532", "pos":(4700,22),  "url": "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112532.h5/r/idx/1mb/visus.idx"},
	]
 
	print("Reading CSV file...")
	csv=pd.read_csv("https://raw.githubusercontent.com/sci-visus/OpenVisus/master/Samples/panel/materialscience/data.csv")
	print("DONE")
	slices=Slices(scans,csv)
	bokeh.io.curdoc().add_root(slices.layout)
 
 




