import io,sys,logging, threading, queue, time

from .backend import ReadStats, Aborted
from .utils import HumanSize
from .query import ExecuteQuery

logger = logging.getLogger(__name__ )

# ///////////////////////////////////////////////////////////////////
class Stats:
	
	# constructor
	def __init__(self):
		self.lock = threading.Lock()
		self.num_running=0
		
	# isRunning
	def isRunning(self):
		with self.lock:
			return self.num_running>0

	# startCollecting
	def startCollecting(self):
		with self.lock:
			self.num_running+=1
			if self.num_running>1: return
		self.t1=time.time()
		ReadStats()
			
	# stopCollecting
	def stopCollecting(self):
		with self.lock:
			self.num_running-=1
			if self.num_running>0: return
		self.printStatistics()

	# printStatistics
	def printStatistics(self):
		sec=time.time()-self.t1
		stats=ReadStats()
		logger.info(f"Stats::printStatistics enlapsed={sec} seconds" )
		try: # division by zero
			for k,v in stats.items():
				logger.info(f"   {k}  r={HumanSize(v['r'])} r_sec={HumanSize(v['r']/sec)}/sec w={HumanSize(v['w'])} w_sec={HumanSize(v['w']/sec)}/sec n={v.n:,} n_sec={int(v/sec):,}/sec")
		except:
			pass

# /////////////////////////////////////////////////////////////////////////////////////////////////
class QueryNode:

	stats=Stats()

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
  
	# _threadLoop
	def _threadLoop(self):

		while True:
			args=self.iqueue.get()

			# need to exit
			if args is None: 
				return 

			# collect statistics
			self.stats.startCollecting() 
   
			aborted=args.get("aborted", Aborted())
			
			for result in ExecuteQuery(args):
       
				if result is None: 
					break 
 
				if aborted.isTrue():
					break
 
				# push the result to the output quue
				if self.oqueue:
					self.oqueue.put(result)
					if self.wait_for_oqueue:
						self.oqueue.join()
      
				if aborted.isTrue():
					break 
      
			# let the main task know I am done
			self.iqueue.task_done()
			self.stats.stopCollecting() 

