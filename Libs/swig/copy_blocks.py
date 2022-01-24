import sys
import io
from threading import Thread
import argparse
import numpy as np 

from OpenVisus import *

# /////////////////////////////////////////////////////////////////////////////
class CopyBlocks:

	# constructor
	def __init__(self,src,dst, time=None, field=None, num_read_per_request=1, verbose=False):
		self.src=src
		self.dst=dst
		self.time =time   if time is not None and time !="" else src.getTime()
		self.field=field if field is not None and field!="" else src.getField()
		self.verbose=verbose

		# for mod_visus aggregation (do N block requests in one network request)
		self.num_read_per_request=num_read_per_request 

		# statistics
		self.t1=Time.now()
		self.ndone=0
		self.tot=0

	# advance
	def advance(self,num):
		self.ndone+=num
		sec=self.t1.elapsedSec()
		if sec>1:
			perc=100.0*self.ndone/float(self.tot)
			print("# *** Progress {:.2f}% {}/{}".format(perc,self.ndone,self.tot))
			self.t1=Time.now()

	# writeBlocks
	def writeBlocks(self, waccess, read_blocks):

		src,dst=self.src,self.dst

		for read_block in read_blocks:
			read_block.waitDone()

			# could be that the block is not stored
			if not read_block.ok(): 
				continue

			# default is to change the layout to rowmajor
			src.convertBlockQueryToRowMajor(read_block)

			buffer= Array.toNumPy(read_block.buffer, bShareMem=False)

			# I don't care if it fails????
			write_ok=dst.writeBlock(read_block.blockid, field=read_block.field.name, time=read_block.time,access=waccess, data=buffer)
			
			if self.verbose or not write_ok:
				if not write_ok: print("ERROR",end=' ')
				print(f"writeBlock time({read_block.time}) field({read_block.field.name}) blockid({read_block.blockid}) write_ok({write_ok})")

	# doCopy
	def doCopy(self, A, B):

		self.tot+=(B-A)
		src,dst=self.src,self.dst
		print(f"Copying blocks time({self.time}) field({self.field.name}) A({A}) B({B}) ...")

		waccess=dst.createAccessForBlockQuery()

		# for mod_visus there is the problem of aggregation (Ii.e. I need to call endRead to force the newtork request)
		if "mod_visus" in src.getUrl():
			num_read_per_request=self.num_read_per_request 
			# mov_visus access will not call itself the flush batch, but will wait for endRead()
			raccess=src.createAccessForBlockQuery(StringTree.fromString("<access type='network' chmod='r'  compression='zip' nconnections='1'  num_queries_per_request='1073741824' />"))
		else:
			num_read_per_request=1
			raccess=src.createAccessForBlockQuery()

		read_blocks=[]
		aborted=Aborted()
		raccess.beginRead()
		waccess.beginWrite()
		for blockid in range(A,B):

			read_block = src.createBlockQuery(blockid, self.field, self.time, ord('r'),aborted)
			src.executeBlockQuery(raccess, read_block)
			read_blocks.append(read_block)

			if len(read_blocks)>=num_read_per_request or blockid==(B-1):
				raccess.endRead() 
				self.writeBlocks(waccess, read_blocks)
				raccess.beginRead()
				self.advance(len(read_blocks))
				read_blocks=[]
			
		Assert(len(read_blocks)==0)
		raccess.endRead()
		waccess.endWrite()

	@staticmethod
	def Main(args):
		print("CopyBlocks", "Got args",args)
		parser = argparse.ArgumentParser(description="Copy blocks")
		parser.add_argument("--src","--source", type=str, help="source", required=True,default="")
		parser.add_argument("--dst","--destination", type=str, help="destination", required=True,default="") 
		parser.add_argument("--field", help="field"  , required=False,default="") 
		parser.add_argument("--time", help="time", required=False,default="") 
		parser.add_argument("--num-threads", help="number of threads", required=False,type=int, default=4)
		parser.add_argument("--num-read-per-request", help="number of read block per network request (only for mod_visus)", required=False,type=int, default=256)
		parser.add_argument("--verbose", help="Verbose", required=False,action='store_true') 
		args = parser.parse_args(args)

		# load src and destination dataset
		src=LoadDataset(args.src); Assert(src)

		# can in which destination  *.idx does not exist
		if not os.path.isfile(args.dst):
			src.db.idxfile.createNewOne(args.dst)
		dst=LoadDataset(args.dst)

		copier=CopyBlocks(src,dst,time=args.time,field=args.field,num_read_per_request=args.num_read_per_request, verbose=args.verbose)

		tot_blocks=src.getTotalNumberOfBlocks()

		nthreads=args.num_threads
		num_per_thread=tot_blocks//nthreads
		print("tot_blocks",tot_blocks,"nthreads",nthreads,"num_per_thread",num_per_thread)

		if nthreads<=1:
			copier.doCopy(0,tot_blocks)
		else:
			threads=[]
			for I in range(nthreads):
				A=(I+0)*num_per_thread
				B=(I+1)*num_per_thread if I<(nthreads-1) else tot_blocks
				threads.append(Thread(target=CopyBlocks.doCopy, args=(copier,A,B)))
			for t in threads: t.start()
			for t in threads: t.join()


