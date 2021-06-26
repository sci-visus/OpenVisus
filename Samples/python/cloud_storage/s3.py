"""
Copy a local dataset to S3.

For Amazon AWS S3
	Create a user for S3 access https://console.aws.amazon.com/iam/home?#/users$new?step=details (change as needed):
	access_key: python-s3
	Type: Programmatic access
	Attach existing policies: AmazonS3FullAccess
	Run aws configure (change as needed):
		AWS Access Key ID [None]: xxxxxxxxx
		AWS Secret Access Key [None]: yyyyyy
		Default region name [None]: us-east-1
		Default output format [None]:
		
For Wasabi SE:
	https://wasabi-support.zendesk.com/hc/en-us/articles/360019677192-Creating-a-Wasabi-API-Access-Key-Set
"""

import sys
import io
import zlib
import threading
import argparse
import boto3
import hashlib
from urllib.parse import urlparse

from OpenVisus import *

# ////////////////////////////////////////////////////////////////
def ComputeMd5(body):
		md5_hash = hashlib.md5()
		md5_hash.update(body)
		return md5_hash.hexdigest() 	

# ////////////////////////////////////////////////////////////////
# https://boto3.amazonaws.com/v1/documentation/api/latest/reference/services/s3.html#bucket
class CopyToS3:
	
	# constructor
	def __init__(self, dst, access_key=None,secret_access_key=None, acl=None):
		
		self.dst=dst
		self.access_key=access_key
		self.secret_access_key=secret_access_key
		self.acl=acl
		
		url=urlparse(dst)
		self.endpoint_url=url.scheme+"://"+url.netloc
		
		# path example 
		#   /bucket_name               (prefix=='')
		#   /bucket_name/with/prefix   (prefix=='with/prefix')
		v=url.path[1:].split("/",1)
		self.bucket_name=v[0]
		self.prefix="/".join(v[1:])
		
		print("CopyToS3:")
		print("   dst",dst)
		print("   endpoint_url",self.endpoint_url)
		print("   bucket_name",self.bucket_name)
		print("   prefix",self.prefix)
		
		self.client = boto3.client('s3',
			endpoint_url = self.endpoint_url,
			aws_access_key_id = access_key,
			aws_secret_access_key = secret_access_key)
		
		self.s3 = boto3.resource('s3',
			endpoint_url =self.endpoint_url,
			aws_access_key_id = access_key,
			aws_secret_access_key = secret_access_key)
			
		try:
			self.s3.create_bucket(Bucket=self.bucket_name, ACL=self.acl)
		except:
			pass
		self.bucket=self.s3.Bucket(self.bucket_name)
		
	# deleteBucket
	def deleteBucket(self):
		try:
			self.bucket.objects.all().delete()
		except:
			pass
		try:
			self.bucket.object_versions.delete()
		except:
			pass
		
		print("Deleted bucket",self.bucket_name)
		self.bucket=None
			

	# existFile
	def existFile(self,blob_name, body):
		remote_md5=""
		try:
			remote_md5=self.client.head_object(Bucket=self.bucket_name, Key=blob_name)['ETag'][1:-1]
		except:
			pass	
		return remote_md5!="" and remote_md5==ComputeMd5(body)
		
	# doCopy
	def doCopy(self, blob_name, body):
		
		if self.prefix:
			blob_name=self.prefix + "/" + blob_name
			
		# boto3 does not want a starting '/'
		Assert(blob_name[0]!='/')
		
		exists= self.existFile(blob_name,body)
		if not exists:
			self.bucket.put_object(Key=blob_name, Body=body, ACL=self.acl)
		
		return {"filename" : self.endpoint_url + "/" + self.bucket_name+ "/" + blob_name, "exists" : exists, "nbytes" : len(body)}
		

	
# /////////////////////////////////////////////////////////////////////////
class CopyBlocks:
	
	# __init__
	def __init__(self, src="", dst=None, nthreads=8,reverse_filename=False,verbose=False):
		self.dst=dst
		self.src=src
		self.nthreads=nthreads
		self.reverse_filename=reverse_filename
		self.T1=Time.now()
		self.verbose=verbose

	# copyMainFile
	def copyMainFile(self):
		self.db=LoadDataset(self.src)
		body=self.db.getDatasetBody().toString().encode()
		
		dst_filename=os.path.basename(self.src)
		
		# remove parameters (i.e all after the ?)
		dst_filename=dst_filename.split("?")[0]
		
		print("Copied dataset body to", self.dst.doCopy(dst_filename, body))

	# copyBlocks
	def copyBlocks(self):

		timesteps=[int(it) for it in self.db.getTimesteps().asVector()]
		print("timesteps",timesteps)
			
		field=self.db.getField()
		print("field","name",field.name,"dtype",field.dtype.toString())

		# collect jobs
		print("Number of blocks",self.db.getTotalNumberOfBlocks())
		self.jobs=[]
		for time in timesteps:
			for blockid in range(self.db.getTotalNumberOfBlocks()):
				self.jobs.append(lambda args=(time, field, blockid): self.copyBlock(*args))

		# and run in parallel
		self.access={}
		self.t1=Time.now()
		self.nbytes=0
		
		# for debugging purpouse
		if self.nthreads<=1:
			for I,job in enumerate(self.jobs):
				job()
				self.advanceCallback(I)
		else:
			RunJobsInParallel(self.jobs, advance_callback=self.advanceCallback ,nthreads=self.nthreads)
			
		for key in self.access:
			self.access[key].endRead()
		self.access=None
		
		print("All Blocks copied")


	# advanceCallback
	def advanceCallback(self,ndone):
		
		sec=self.t1.elapsedSec()
		if sec>3:
			mb_per_sec=(self.nbytes / sec)/(1024*1024)
			print("# *** Progress {:.2f}%".format(100.0*ndone/float(len(self.jobs))),"{:.2f}MB/sec".format(mb_per_sec),"{}/{}".format(ndone,len(self.jobs)))
			self.nbytes=0
			self.t1=Time.now()

	# copyBlock
	def copyBlock(self, time, field, blockid):
		
			tid=threading.current_thread().ident
			
			if not tid in self.access:
				access=self.db.createAccessForBlockQuery()
				access.beginRead()
				self.access[tid]=access
			
			access=self.access[tid]
			buffer = self.db.readBlock(blockid, field=field, time=time,access=access)
			
			# if you want to debug
			# return ArrayUtils.saveImage("tmp.png",Array.fromNumPy(buffer, TargetDim=2, bShareMem=True))
			
			# could be that the block is not stored
			if buffer is None:
				return

			# I'm storing  zipped block on S3
			buffer=zlib.compress(buffer,zlib.Z_DEFAULT_COMPRESSION)

			# someone says it's better to reverse the filename to parallelize better
			filename="{time}/{field}/{block:016x}".format(time=time,field=field.name, block=blockid)
			if self.reverse_filename:  
				filename=filename[::-1]
					
			ret=self.dst.doCopy(filename,buffer)
			if self.verbose:
				print("Copied block",filename,ret)
			self.nbytes+=ret["nbytes"]

	# Main
	@staticmethod
	def Main(args):

		parser = argparse.ArgumentParser(description="Copy blocks")
		
		parser.add_argument("--src","--source", type=str, help="source", required=True,default="")
		parser.add_argument("--dst","--destination", type=str, help="destination", required=True,default="") 
		parser.add_argument("--access-key",type=str, help="Access key", required=False,default=os.getenv('ACCESS_KEY')) 
		parser.add_argument("--secret-access-key",type=str, help="secret_access_key", required=False,default=os.getenv('SECRET_ACCESS_KEY')) 
		parser.add_argument("--reverse-filename" , help="Reverse filename, sometimes you gain some speed reversing (see https://aws.amazon.com/blogs/aws/amazon-s3-performance-tips-tricks-seattle-hiring-event/)", action='store_true') 
		parser.add_argument("--acl", type=str, help="acl", required=False,default="public-read") 
		parser.add_argument("--num-threads", type=int, help="Number of threads", required=False,default=8) 
		parser.add_argument("--verbose", help="Verbose", required=False,action='store_true') 
		
		args = parser.parse_args(args)
		
		dst=CopyToS3(
			dst=args.dst,
			access_key=args.access_key, 
			secret_access_key=args.secret_access_key, 	
			acl=args.acl)
		
		copy_blocks=CopyBlocks(
			src=args.src, 
			dst=dst, 
			nthreads=args.num_threads,
			reverse_filename=args.reverse_filename,
			verbose=args.verbose)
			
		copy_blocks.copyMainFile()
		copy_blocks.copyBlocks()
		
		dst_filename=args.dst + "/" + os.path.basename(args.src)
		cache_dir=dst_filename.replace(":","")
		
		print("""Use the following visus.config:
<dataset url='{dst_filename}' >
	<access type='multiplex'>
		<access type='disk' chmod='rw' url='file://D:/visus-cache/{cache_dir}' />
		<access type="CloudStorageAccess" url="{dst_filename}" chmod="r" compression="zip" />
	</access>
</dataset>""".format(dst_filename=dst_filename,cache_dir=cache_dir))

	
# /////////////////////////////////////////////////////////////////////////
if __name__ == '__main__':	
	
	"""
	set ACCESS_KEY=XXXX
	set SECRET_ACCESS_KEY=YYYY
	
	python Samples/python/cloud_storage/s3.py copy-blocks --src C:/projects/OpenVisus/datasets/cat/gray.idx              --dst https://s3.us-west-1.wasabisys.com/cat-gray
	python Samples/python/cloud_storage/s3.py copy-blocks --src C:/projects/OpenVisus/datasets/cat/gray.idx              --dst https://s3.us-west-1.wasabisys.com/cat-gray/example/prefix
	python Samples/python/cloud_storage/s3.py copy-blocks --src D:/GoogleSci/visus_dataset/2kbit1/zip/hzorder/visus.idx  --dst https://s3.us-west-1.wasabisys.com/2kbit1
	python Samples/python/cloud_storage/s3.py copy-blocks --src D:/GoogleSci/visus_dataset/2kbit1/zip/hzorder/visus.idx  --dst https://mghp.osn.xsede.org/vpascuccibucket1/2kbit1
	
	"""	

	print("Got args",repr(sys.argv))
	action=sys.argv[1]
	action_args=sys.argv[2:]
	
	if action=="copy-blocks":
		CopyBlocks.Main(action_args)
		print("All done")
		sys.exit(0)

	
		

