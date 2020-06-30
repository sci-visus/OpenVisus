import os,sys,glob,threading,platform,sysconfig,re,time,subprocess, errno, fnmatch, shutil

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
	print("# Executing",cmd)
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
	print("# Executing command: ",cmd)
	return subprocess.call(cmd, shell=False)


# ///////////////////////////////////////////////////////////////////////
def RunJobsInParallel(jobs, advance_callback=None, nthreads=8):

	class MyThread(threading.Thread):

		# constructor
		def __init__(self):
			super(MyThread, self).__init__()

		# run
		def run(self):
			for job in self.jobs:
				self.jobDone(job())

	nthreads=min(nthreads,len(jobs))
	threads,results=[],[]
	for WorkerId in range(nthreads):
		thread=MyThread()
		threads.append(thread)
		thread.jobs=[job for I,job in enumerate(jobs)  if (I % nthreads)==WorkerId]
		thread.jobDone=lambda result: results.append(result)
		thread.start()
			
	while True:

		time.sleep(0.01)

		if len(results)==len(jobs):
			[thread.join() for thread in threads]
			return results

		if advance_callback:
				advance_callback(len(results))


# /////////////////////////////////////////////////////////////////////////
def RemoveFiles(pattern):
	files=glob.glob(pattern)
	print("Removing files",files)
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
