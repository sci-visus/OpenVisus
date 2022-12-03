import os,sys,glob,threading,platform,sysconfig,re,time, errno, fnmatch, shutil, logging,functools
import traceback, time, subprocess, shutil, shlex, math
import io, queue, threading, multiprocessing, subprocess

logger = logging.getLogger("OpenVisus")

# ////////////////////////////////////////////////////////////////////////////////
def ThisDir(file):
	return os.path.dirname(os.path.abspath(file))

# ////////////////////////////////////////////////////////////////////////////////
def Assert(condition):
	if not condition:
		raise Exception("Assert failed")

# /////////////////////////////////////////////////////////////////////////
def GetCommandOutput(cmd):
	output=subprocess.check_output(cmd)
	if sys.version_info >= (3, 0): output=output.decode("utf-8")
	return output.strip()

# /////////////////////////////////////////////////////////////////////////
def CreateDirectory(value):
	try: 
		os.makedirs(value)
	except OSError:
		if not os.path.isdir(value):
			raise
	
# /////////////////////////////////////////////////////////////////////////
def MakeDirForFile(filename):
	try:
		os.makedirs(os.path.dirname(filename),exist_ok=True)
	except:
		pass

# /////////////////////////////////////////////////////////////////////////
def GetFilenameWithoutExtension(filename):
	return os.path.splitext(os.path.basename(filename))[0]

# /////////////////////////////////////////////////////////////////////////
def CopyFile(src,dst):
	
	src=os.path.realpath(src) 
	dst=os.path.realpath(dst)		
	
	if src==dst or not os.path.isfile(src):
		return		

	CreateDirectory(os.path.dirname(dst))
	shutil.copyfile(src, dst)	
	
# /////////////////////////////////////////////////////////////////////////
def CopyDirectory(src,dst):
	
	src=os.path.realpath(src)
	
	if not os.path.isdir(src):
		return
	
	CreateDirectory(dst)
	
	# problems with symbolic links so using shutil	
	dst=dst+"/" + os.path.basename(src)
	
	if os.path.isdir(dst):
		shutil.rmtree(dst,ignore_errors=True)
		
	shutil.copytree(src, dst, symlinks=True)				
	
# /////////////////////////////////////////////////////////////////////////
def ReadTextFile(filename):
	file = open(filename, "r") 
	ret=file.read().strip()
	file.close()
	return ret

# ////////////////////////////////////////////////////////////////////////////////
def LoadTextDocument(filename):
	if not os.path.isfile(filename): return []
	file=open(filename,"rt")
	content=file.read()
	file.close()	
	return content
	
# /////////////////////////////////////////////////////////////////////////
def WriteTextFile(filename,content):
	if not isinstance(content, str):
		content="\n".join(content)+"\n"
	CreateDirectory(os.path.dirname(os.path.realpath(filename)))
	file = open(filename,"wt") 
	file.write(content) 
	file.close() 		

# ////////////////////////////////////////////////////////////////////////////////
def SaveTextDocument(filename,content):
	try:
		os.makedirs(os.path.dirname(filename),exist_ok=True)
	except:
		pass

	file=open(filename,"wt")
	file.write(content)
	file.close()		

# /////////////////////////////////////////////////////////////////////////
# glob(,recursive=True) is not supported in python 2.x
# see https://stackoverflow.com/questions/2186525/use-a-glob-to-find-files-recursively-in-python
def RecursiveFindFiles(rootdir='.', pattern='*'):
  return [os.path.join(looproot, filename)
          for looproot, _, filenames in os.walk(rootdir)
          for filename in filenames
          if fnmatch.fnmatch(filename, pattern)]

# /////////////////////////////////////////////////////////////////////////
def PipInstall(packagename,extra_args=[]):
	cmd=[sys.executable,"-m","pip","install","--progress-bar","off","--user",packagename]
	if extra_args: cmd+=extra_args
	logger.info(f"# Executing {cmd}")
	return_code=subprocess.call(cmd)
	return return_code==0

# ////////////////////////////////////////////////////////////////////////////////
def ParseDouble(value,default_value=0.0):

	if isinstance(value,str) and len(value)==0:
		return default_value
	try:
		return float(value)
	except:
		return default_value

# ////////////////////////////////////////////////////////////////////////////////
def ParseInt(value,default_value=0):

	if isinstance(value,str) and len(value)==0:
		return default_value
	try:
		return int(value)
	except:
		return default_value			
	
	
# ////////////////////////////////////////////////////////////////////////////////
def GuessUniqueFilename(pattern):
	I=0
	while True:
		filename=pattern %(I,)
		if not os.path.isfile(filename): 
			return filename
		I+=1

# ////////////////////////////////////////////////////////////////////////////////
def KillProcess(process):
	if not process:
		return
	try:
		process.kill()
	except:
		pass	
	
# /////////////////////////////////////////////////////////////////////////
def ExecuteCommand(cmd):	
	"""
	note: shell=False does not support wildcard but better to use this version
	because quoting the argument is not easy
	"""
	logger.info(f"# Executing command: {cmd}")
	return subprocess.call(cmd, shell=False)



# //////////////////////////////////////////////////////////////////////////////
def WriteCSV(filename,rows):
	import csv
	with open(filename,"wt") as f:
		writer=csv.writer(f)
		writer.writerows([row for row in rows if row])

# //////////////////////////////////////////////////////////////////////////////
def ReadCSV(filename):
	import csv
	with open(filename,"rt") as f:
		reader=csv.reader(f)
		return [row for row in reader if row]

# //////////////////////////////////////////////////////////////////////
def WriteYaml(filename,data):
	import yaml
	logger.info(f"Writing yaml {filename}...")
	with open(filename, 'w') as stream:
		yaml.dump(data, stream)
	logger.info(f"Writing yaml {filename} DONE")

# //////////////////////////////////////////////////////////////////////
def ReadYaml(filename):
	import yaml
	logger.info(f"Reading yaml {filename}...")
	with open(filename, 'r') as stream:
		ret=yaml.load(stream)
	logger.info(f"Reading yaml {filename} DONE")
	return ret

# /////////////////////////////////////////////////////////////////////////
def HumanSize(size):
	KiB,MiB,GiB,TiB=1024,1024*1024,1024*1024*1024,1024*1024*1024*1024
	if size>TiB: return "{:.2f}TiB".format(size/TiB) 
	if size>GiB: return "{:.2f}GiB".format(size/GiB) 
	if size>MiB: return "{:.2f}MiB".format(size/MiB) 
	if size>KiB: return "{:.2f}KiB".format(size/KiB) 
	return str(size)

# ////////////////////////////////////////////////////////////////
def SetupLogger(logger, output_stdout:bool=True, log_filename:str=None, logging_level=logging.INFO):

	logger.setLevel(logging_level)

	# stdout
	if output_stdout:
		handler=logging.StreamHandler()
		handler.setLevel(logging_level)
		handler.setFormatter(logging.Formatter(fmt=f"[%(asctime)s][%(levelname)s][%(name)s] %(message)s", datefmt="%H%M%S"))
		logger.addHandler(handler)
	
	# file
	if log_filename:
		os.makedirs(os.path.dirname(log_filename),exist_ok=True)
		handler=logging.FileHandler(log_filename)
		handler.setLevel(logging_level)
		handler.setFormatter(logging.Formatter(fmt=f"[%(asctime)s][%(levelname)s][%(name)s] %(message)s", datefmt="%H%M%S"))
		logger.addHandler(handler)


# ///////////////////////////////////////////////////////////////////////
# deprecated: use python multiprocessing ThreadPool map or map_async
def RunJobsInParallel(jobs, advance_callback=None, nthreads=8, timeout=60*60*24*30):
	from multiprocessing.pool import ThreadPool
	p=ThreadPool(nthreads)
	chunk_size,N=1,len(jobs)
	return p.map_async(jobs, []*len(jobs), chunk_size).get(timeout=timeout) 


# /////////////////////////////////////////
def RemoveTree(dir):
	while os.path.isdir(dir):
		try:
			shutil.rmtree(dir, ignore_errors=False)
		except:
			logger.info(f"Failed to removed directory {dir}, retrying in feww seconds")
			time.sleep(1)
	logger.info(f"Removed directory {dir}")


# /////////////////////////////////////////////////////////////////////////
def RemoveFiles(pattern):
	files=glob.glob(pattern)
	logger.info(f"Removing files {files}")
	for it in files:
		if os.path.isfile(it):
			os.remove(it)
		else:
			shutil.rmtree(os.path.abspath(it),ignore_errors=True)		


# ////////////////////////////////////////////////////////////////////////////////
def TryRemoveFiles(mask):

	for filename in glob.glob(mask):
		try:
			os.remove(filename)
		except:
			pass


# ////////////////////////////////////////////////////////////////////////
def RunShellCommand(cmd, verbose=False, nretry=3):
	
	logger.info(f"RunShellCommand {cmd} ...")

	if isinstance(cmd,str):
		cmd=cmd.replace("\\","/") # shlex eats backslashes
		args=shlex.split(cmd)
	else:
		args=cmd

	logger.info(f"RunShellCommand {args} ...")
	
	t1 = time.time()

	for I in range(nretry):

		result=subprocess.run(args, 
			shell=False, 
			check=False,
			stdout=subprocess.PIPE,
			stderr=subprocess.STDOUT)

		output=result.stdout.decode('utf-8')

		if verbose: 
			logger.info(output)

		if result.returncode==0:
			break

		error_msg=f"RunShellCommand {args} failed with returncode={result.returncode} output:\n{output}"

		if verbose or I==(nretry-1):
			logger.info(error_msg)

		if I==(nretry-1):
			raise Exception(error_msg)

	sec=time.time()-t1
	logger.info(f"RunShellCommand {args} done in {sec} seconds")

# ///////////////////////////////////////////////////
class WorkerPool:

	"""
	We could use python ThreadPool for this, but you cannot submit jobs while running other jobs.
	This class should resolve this problem, so you can run jobs, add new jobs while running and iterate
	in results
	"""
    
	# ________________________________________________
	class WorkerClass:
 
		def runWorkerLoop(self, worker_pool):
			while True:
				task=worker_pool.popTask()
				if task is None: return # finished
				result=task()
				worker_pool.finishedTask(task, result)

	class LastResult: pass

	# constructor
	def __init__(self,num_workers, worker_class=None):
		if not worker_class:
			worker_class=WorkerPool.WorkerClass
		self.lock=multiprocessing.Lock()
		self.processing=0
		self.q=queue.Queue()
		self.results=queue.Queue()
		self.t1=time.time()	
		self.exit=0
		self.threads=[]
		for n in range(num_workers):
			worker = worker_class()
			thread = threading.Thread(name=f"worker-{n:02d}",target=functools.partial(worker.runWorkerLoop,self))
			self.threads.append(thread)

	# start
	def start(self):
		for worker in self.threads:
			worker.start()

	# __iter__
	def __iter__(self):
		return self
    
	# __next__
	def __next__(self):
		ret=self.results.get()
		if isinstance(ret,WorkerPool.LastResult):
			self.exit=True

			for thread in self.threads:
				self.q.put(None) # tell worker to quit
    
			for thread in self.threads:
				thread.join()
    
			raise StopIteration
		else:
			return ret

	# pushTask
	def pushTask(self, task):
		with self.lock: 
			self.q.put(task)
			self.processing+=1
   
  # popTask
	def popTask(self):
		return self.q.get()

	# finishedTask
	def finishedTask(self, task, result):
		with self.lock: 
			self.processing-=1
			self.results.put(result)
			# if not processing add END result
			if not self.processing:
				self.results.put(WorkerPool.LastResult())    
