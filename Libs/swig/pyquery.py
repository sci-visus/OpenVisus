import os,sys,time,threading,queue,math, types
import numpy as np
import logging

import OpenVisus as ov

logger = logging.getLogger("pyquery")

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
		ov.File.global_stats().resetStats()
		ov.NetService.global_stats().resetStats()
			
	# stopQuery
	def stopQuery(self):
		with self.lock:
			self.num_running-=1
			if self.num_running>0: return
		self.printStatistics()

	# printStatistics
	def printStatistics(self):
		sec=time.time()-self.t1
		stats=types.SimpleNamespace()
		io=ov.File.global_stats()
		stats.io=types.SimpleNamespace()
		stats.io.r,stats.io.w, stats.io.n=io.getReadBytes(), io.getWriteBytes(), io.getNumOpen()
		net=ov.NetService.global_stats()
		stats.net=types.SimpleNamespace()
		stats.net.r, stats.net.w, stats.net.n=net.getReadBytes(), net.getWriteBytes(), net.getNumRequests()
		logger.info(f"Statistics enlapsed={sec} seconds" )
		logger.info(f"   IO  r={ov.HumanSize(stats.io .r)} r_sec={ov.HumanSize(stats.io .r/sec)}/sec w={ov.HumanSize(stats.io .w)} w_sec={ov.HumanSize(stats.io .w/sec)}/sec n={stats.io .n:,} n_sec={int(stats.io .n/sec):,}/sec")
		logger.info(f"   NET r={ov.HumanSize(stats.net.r)} r_sec={ov.HumanSize(stats.net.r/sec)}/sec w={ov.HumanSize(stats.net.w)} w_sec={ov.HumanSize(stats.net.w/sec)}/sec n={stats.net.n:,} n_sec={int(stats.net.n/sec):,}/sec")



# ////////////////////////////////////////////////////////////////////////
# PyQuery is the equivalent of C++ QUeryNode but in Python, useful for python dashboards

"""
Example:

db=ov.LoadDataset(...)
query=PyQuery()
query.startThread()
access=db.createAccessForBlockQuery()
logic_box=...
query.pushJob(db, access=access, timestep=db.getTimestep(), field=db.getField(), logic_box=logic_box, max_pixels=1024*768, num_refinements=3, aborted=ov.Aborted())
data,logic_box=query.popResult()
query.stopThread()

"""
class PyQuery:


	query_id=0
	stats=PyStats()


	# constructor
	def __init__(self):
		self.query_id=PyQuery.query_id
		PyQuery.query_id+=1

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
	def pushJob(self,db=None, access=None,timestep=None,field=None,logic_box=None,max_pixels=None,num_refinements=None,aborted=ov.Aborted()):
		self.iqueue.put([db, access, timestep,field, logic_box, max_pixels, num_refinements,aborted])

	# popResult
	def popResult(self, last_only=True):
		assert self.oqueue is not None
		data,logic_box=None,None
		while not self.oqueue.empty():
			data,logic_box=self.oqueue.get()
			self.oqueue.task_done()
			if not last_only: break
		return data,logic_box 
  
	# getAlignedBox
	@staticmethod
	def getAlignedBox(db,logic_box,H, slice_dir:int=None):
		pdim=db.getPointDim()
		maxh=db.getMaxResolution()
		bitmask=db.getBitmask().toString()
		delta=[1,1,1]
		for B in range(maxh,H,-1):
			bit=ord(bitmask[B])-ord('0')
			A,B,D=logic_box[0][bit], logic_box[1][bit], delta[bit]
			D*=2
			A=int(math.floor(A/D))*D
			B=int(math.ceil (B/D))*D
			B=max(A+D,B)
			logic_box[0][bit] = A 
			logic_box[1][bit] = B
			delta[bit] = D
	  
		#  force to be a slice?
		if pdim==3 and slice_dir is not None:
			offset=logic_box[0][slice_dir]
			logic_box[1][slice_dir]=offset+0
			logic_box[1][slice_dir]=offset+1
			delta[slice_dir]=1

		num_pixels=[(logic_box[1][I]-logic_box[0][I])//delta[I] for I in range(pdim)]
		return logic_box, delta,num_pixels

	# read
	@staticmethod
	def read(db,  access=None, timestep=None, field=None, logic_box=None, num_refinements=1, max_pixels=None, aborted=ov.Aborted(), verbose=False):

		def Clamp(value,a,b):
			assert a<=b
			if value<a: value=a
			if value>b: value=b
			return value

		# logger.info(f"read logic_box={logic_box}")

		pdim=db.getPointDim()
		assert pdim==2 or pdim==3 # todo other cases?
	 
		maxh=db.getMaxResolution()
		bitmask=db.getBitmask().toString()
		dims=db.getLogicSize()

		# if timestep is not specified get the default one
		if timestep is None:
			timestep=db.getTimestep()

		# if field is not specified get the default one
		if field is None:
			field=db.getField()
	  
		if isinstance(field,str):
			field=db.getField(field)
	 
		# if box is not specified get the all box
		if logic_box is None:
			W,H,D=[int(it) for it in db.getLogicSize()]
			logic_box=[[0,0,0],[W,H,D]]
	  
		# fix logic box
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

		# default is to use max resolution
		endh=maxh
	 
		# is view dependent? if so guess max resolution 
		if max_pixels:
			for H in range(maxh,0,-1):
				logic_box,delta,num_pixels=PyQuery.getAlignedBox(db,logic_box,H, slice_dir=slice_dir)

				if verbose:
					logger.info(f"read Guess resolution H={H} logic_box={logic_box} delta={delta} num_pixels={num_pixels} tot_pixels={np.prod(num_pixels):,} max_pixels={max_pixels:,}")

				if np.prod(num_pixels)<=max_pixels*1.10:
					endh=H
					break

		# compute intermediate resolutions
		if num_refinements==1:
			end_resolutions=[endh]
		else:
			end_resolutions=list(reversed([ endh-pdim*I for I in range(num_refinements) if endh-pdim*I>=0]))

		# this is the query I need
		logic_box,delta,num_pixels=PyQuery.getAlignedBox(db,logic_box, end_resolutions[0], slice_dir=slice_dir)
	 
		box_ni=ov.BoxNi(
			ov.PointNi([int(it) for it in logic_box[0]]),  
			ov.PointNi([int(it) for it in logic_box[1]]))
	 
		query = db.createBoxQuery(box_ni,  field ,  timestep, ord('r'), aborted)

		for H in end_resolutions:
			query.end_resolutions.push_back(H)

		db.beginBoxQuery(query)
		I,N=0,len(end_resolutions)
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
	  
			yield logic_box, data
			db.nextBoxQuery(query)

	# _threadLoop
	def _threadLoop(self):

		ABORTED=ov.Aborted()
		ABORTED.setTrue() 

		while True:

			args=self.iqueue.get()
			
			# need to exit
			if args is None: 
				return

			self.stats.startQuery()
			db,access, timestep, field, logic_box, max_pixels,num_refinements, aborted = args

			logger.info(f"[{self.query_id}] Got job {timestep} {field} {logic_box} {num_refinements} {max_pixels}")
			I=0
			for logic_box, data in PyQuery.read(db, access=access, timestep=timestep, field=field, logic_box=logic_box, num_refinements=num_refinements, max_pixels=int(np.prod(max_pixels)), aborted=aborted):
				
				# query failed
				if data is None:
					break

				if aborted.__call__()==ABORTED.__call__():
					break

				logger.info(f"[{self.query_id}] {I}/{num_refinements} {data.shape} {data.dtype} m={np.min(data)} M={np.max(data)}")
				I+=1
    
				if self.oqueue:
					self.oqueue.put((data,logic_box))
					if self.wait_for_oqueue:
						self.oqueue.join()
      
				if aborted.__call__()==ABORTED.__call__():
					break   
   
				

			# let the main task know I am done
			logger.info(f"[{self.query_id}] job done")
			self.iqueue.task_done()
			self.stats.stopQuery()

