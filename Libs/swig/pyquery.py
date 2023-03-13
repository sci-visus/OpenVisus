import os,sys,time,threading,queue,math, types
import numpy as np
import logging
import copy

import OpenVisus as ov
from OpenVisus import Aborted,SetupLogger, cbool, HumanSize

logger = logging.getLogger(__name__)

if cbool(os.environ.get("VISUS_PYQUERY_VERBOSE","0")) == True:
	SetupLogger(logger)

# ///////////////////////////////////////////////////////////////////
def ResetStats():
	ov.File.global_stats().resetStats()
	ov.NetService.global_stats().resetStats()

# ///////////////////////////////////////////////////////////////////
def ReadStats():

	io=ov.File.global_stats()
	net=ov.NetService.global_stats()

	return {
		"io": {
			"r":io.getReadBytes(),
			"w":io.getWriteBytes(),
			"n":io.getNumOpen(),
		},
		"net":{
			"r":net.getReadBytes(), 
			"w":net.getWriteBytes(),
			"n":net.getNumRequests(),
		}
	}


# ///////////////////////////////////////////////////////////////////
def IsIterable(value):
	try:
		iter(value)
		return True
	except:
		return False


# ///////////////////////////////////////////////////////////////////
class PyStats:
	
	# constructor
	def __init__(self):
		self.lock = threading.Lock()
		self.num_running=0
		
	# isRunning
	def isRunning(self):
		with self.lock:
			return self.num_running>0

	# startQuery
	def startQuery(self):
		with self.lock:
			self.num_running+=1
			if self.num_running>1: return

		self.t1=time.time()
		ResetStats()
			
	# stopQuery
	def stopQuery(self):
		with self.lock:
			self.num_running-=1
			if self.num_running>0: return
		self.printStatistics()

	# printStatistics
	def printStatistics(self):
		sec=time.time()-self.t1
		stats=ReadStats()
		logger.info(f"PyStats::printStatistics enlapsed={sec} seconds" )
		try: # division by zero
			for k,v in stats.items():
				logger.info(f"   {k}  r={HumanSize(v['r'])} r_sec={HumanSize(v['r']/sec)}/sec w={HumanSize(v['w'])} w_sec={HumanSize(v['w']/sec)}/sec n={v.n:,} n_sec={int(v/sec):,}/sec")
		except:
			pass

		# read

# ////////////////////////////////////////////////////////////////////////////////////////////////////////////
def Clamp(value,a,b):
	assert a<=b
	if value<a: value=a
	if value>b: value=b
	return value

# ////////////////////////////////////////////////////////////////////////////////////////////////////////////
def Read(args):

	db=args["db"]
	assert(db)

	pdim=db.getPointDim()
	assert pdim==2 or pdim==3 # todo other cases?

	maxh=db.getMaxResolution()
	bitmask=db.getBitmask().toString()
	dims=db.getLogicSize()

	access=args["access"]
	timestep=args.get("timestep",db.getTime())
	field=args.get("field",db.getField())
	logic_box=args["logic_box"]
	max_pixels=args.get("max_pixels",None)
	endh=args.get("endh",maxh)
	num_refinements=args.get("num_refinements",1)
	aborted=args.get("aborted",Aborted())

	logger.info(f"pyquery.Read begin timestep={timestep} field={field} logic_box={logic_box} num_refinements={num_refinements} max_pixels={max_pixels} endh={endh}")

	if IsIterable(max_pixels):
		max_pixels=int(np.prod(max_pixels,dtype=np.int64))

	if isinstance(field,str):
		field=db.getField(field)
	 
	# if box is not specified get the all box
	if logic_box is None:
		W,H,D=[int(it) for it in db.getLogicSize()]
		logic_box=[[0,0,0],[W,H,D]]
	  
	# fix logic box by cropping
	if True:
		p1,p2=list(logic_box[0]),list(logic_box[1])
		slice_dir=None
		for I in range(pdim):
			# *************** is a slice? *******************
			if pdim==3 and (p2[I]-p1[I])==1:
				assert slice_dir is None 
				slice_dir=I
				p1[I]=Clamp(p1[I],0,dims[I])
				p2[I]=p1[I]+1
			else:
				p1[I]=Clamp(int(math.floor(p1[I])),     0,dims[I])
				p2[I]=Clamp(int(math.ceil (p2[I])) ,p1[I],dims[I])
			assert p1[I]<p2[I]
		logic_box=(p1,p2)
	 
	# is view dependent? if so guess max resolution 
	if max_pixels:
		original_box=logic_box
		for H in range(maxh,0,-1):
			aligned_box,delta,num_pixels=PyQuery.getAlignedBox(db,original_box,H, slice_dir=slice_dir)
			tot_pixels=np.prod(num_pixels,dtype=np.int64)
			if tot_pixels<=max_pixels*1.10:
				endh=H
				logger.info(f"Guess resolution H={H} original_box={original_box} aligned_box={aligned_box} delta={delta} num_pixels={repr(num_pixels)} tot_pixels={tot_pixels:,} max_pixels={max_pixels:,} end={endh}")
				logic_box=aligned_box
				break

	# this is the query I need
	logic_box,delta,num_pixels=PyQuery.getAlignedBox(db,logic_box, endh, slice_dir=slice_dir)


	box_ni=ov.BoxNi(
		ov.PointNi([int(it) for it in logic_box[0]]),  
		ov.PointNi([int(it) for it in logic_box[1]]))
  
	query = db.createBoxQuery(box_ni,  field ,  timestep, ord('r'), aborted)

	# compute intermediate resolutions
	end_resolutions=list(reversed([ endh-pdim*I for I in range(num_refinements) if endh-pdim*I>=0]))
	# print("pyquery.Read"."endh",endh,"end_resolutions",end_resolutions,"maxh",maxh)
	for H in end_resolutions:
		query.end_resolutions.push_back(H)

	# print("beginBoxQuery","box",box_ni.toString(),"field",field.name,"timestep",timestep)
	t1=time.time()
	I,N=0,len(end_resolutions)
	db.beginBoxQuery(query)
	while query.isRunning():
		if not db.executeBoxQuery(access, query): break
		data=ov.Array.toNumPy(query.buffer, bShareMem=False) 

		# is a slice? I need to reduce the size
		if slice_dir is not None:
			dims=list(reversed(data.shape))
			assert dims[slice_dir]==1
			del dims[slice_dir]
			while len(dims)>2 and dims[-1]==1: dims=dims[0:-1] # remove right `1`
			data=data.reshape(list(reversed(dims))) 
    
		H=query.getCurrentResolution()
		msec=int(1000*(time.time()-t1))
		logger.info(f"pyqquery.read got data {I}/{N} timestep={timestep} field={field.name} H={H} data.shape={data.shape} data.dtype={data.dtype} logic_box={logic_box} m={np.min(data)} M={np.max(data)} ms={msec}")
		yield {"I":I,"N":N,"timestep":timestep,"field":field.name,"logic_box":logic_box, "H":H, "data":data,"msec":msec}
		I+=1
		db.nextBoxQuery(query)

	logger.info(f"pyqquery.read read done")


# /////////////////////////////////////////////////////////////////////////////////////////////////
# PyQuery is the equivalent of C++ QUeryNode but in Python, useful for python dashboards
class PyQuery:

	stats=PyStats()

	# constructor
	def __init__(self):
		self.iqueue=queue.Queue()
		self.oqueue=queue.Queue()
		self.wait_for_oqueue=False

	# disableOutputQueue
	def disableOutputQueue(self):
		self.oqueue=None

	# startThread
	def startThread(self):
		self.thread = threading.Thread(target=lambda: self._threadLoop())
		self.thread.start()   

	# stopThread
	def stopThread(self):
		self.iqueue.join()
		self.iqueue.put(None)
		self.thread.join()
		self.thread=None   

	# waitIdle
	def waitIdle(self):
		self.iqueue.join()

	# pushJob
	def pushJob(self,args):
		self.iqueue.put(args)

	# popResult
	def popResult(self, last_only=True):
		assert self.oqueue is not None
		ret=None
		while not self.oqueue.empty():
			ret=self.oqueue.get()
			self.oqueue.task_done()
			if not last_only: break
		return ret
  
	# getAlignedBox
	@staticmethod
	def getAlignedBox(db,logic_box,H, slice_dir:int=None):
		ret=copy.deepcopy(logic_box)
		pdim=db.getPointDim()
		maxh=db.getMaxResolution()
		bitmask=db.getBitmask().toString()
		delta=[1,1,1]
		for B in range(maxh,H,-1):
			bit=ord(bitmask[B])-ord('0')
			A,B,D=ret[0][bit], ret[1][bit], delta[bit]
			D*=2
			A=int(math.floor(A/D))*D
			B=int(math.ceil (B/D))*D
			B=max(A+D,B)
			ret[0][bit] = A 
			ret[1][bit] = B
			delta[bit] = D
	  
		#  force to be a slice?
		if pdim==3 and slice_dir is not None:
			offset=ret[0][slice_dir]
			ret[1][slice_dir]=offset+0
			ret[1][slice_dir]=offset+1
			delta[slice_dir]=1
   
		num_pixels=[(ret[1][I]-ret[0][I])//delta[I] for I in range(pdim)]
		return ret, delta,num_pixels

	
	# _threadLoop
	def _threadLoop(self):
		ABORTED=Aborted()
		ABORTED.setTrue() 
		while True:
			args=self.iqueue.get()
			if args is None: return # need to exit
			self.stats.startQuery() # collect statistics
			for result in Read(args):
				aborted=args.get("aborted",Aborted())
				if result is None: break # no result, jumpt this lopp
				if aborted.__call__()==ABORTED.__call__(): break # query aborted

				# push the result to the output quue
				if self.oqueue:
					self.oqueue.put(result)
					if self.wait_for_oqueue:
						self.oqueue.join()
      
				if aborted.__call__()==ABORTED.__call__(): break # aborder
   
			# let the main task know I am done
			self.iqueue.task_done()
			self.stats.stopQuery() # print statistics

