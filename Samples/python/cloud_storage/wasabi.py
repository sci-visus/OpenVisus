
import sys
import io
import zlib
import multiprocessing 
import argparse

# pip install boto3
import boto3

from OpenVisus import *

# ////////////////////////////////////////////////////////////////
class S3:
	
	# constructor
	def __init__(self, bucket_name, endpoint_url='https://s3.us-west-1.wasabisys.com', username=None,password=None, acl=None):
		self.bucket_name=bucket_name
		
		self.s3 = boto3.resource('s3',
			endpoint_url = endpoint_url,
			aws_access_key_id = username,
			aws_secret_access_key = password)
			
		self.bucket=self.s3.Bucket(self.bucket_name)
		self.acl=acl
		
	# deleteBucket
	# Add a function to delete recursively a bucket from Wasabi S3:
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
			
	# createBucket
	def createBucket(self):
		self.bucket=self.s3.create_bucket(Bucket=self.bucket_name, ACL=self.acl)
		print("Created bucket",self.bucket_name)
		
	# uploadFile
	def uploadFile(self,key,body):
		self.bucket.put_object(Key=key, Body=body, ACL=self.acl)
		print("Uploaded file",key)

	
# /////////////////////////////////////////////////////////////////////////
def CopyDatasetToS3(source="", destination="", username="", password="", filename_template="{time}/{field}/{block:016x}", layout="", reverse_filename=False, acl="public-read"):
	
	# example: https://bucket_name.hostname/all/other
	protocol,other=destination.split("://",1)
	hostname,destination_filename=other.split("/",1)
	bucket_name,endpoint_url=hostname.split(".",1)
	endpoint_url=protocol+"://"+endpoint_url
	
	print("source ",source)
	print("destination ",destination)
	print("  bucket name ",bucket_name)
	print("  endpoint_url", endpoint_url)
	print("  filename ", destination_filename)
	#print("username ",username)
	#print("password ",password)
	print("filename_template ",filename_template)
	print("layout ",layout)
	print("reverse_filename ",reverse_filename)
	print("acl ",acl)
	
	s3=S3(
		bucket_name, 
		endpoint_url=endpoint_url, 
		username=username,
		password=password, 
		acl=acl
	)
	
	# dangerous
	#s3.deleteBucket()
	
	s3.createBucket()
	
	db=LoadDataset(source)
	body=db.getDatasetBody().toString().encode()

	s3.uploadFile(destination_filename, body)

	timesteps=[int(it) for it in db.getTimesteps().asVector()]
	print("timesteps",timesteps)
		
	field=db.getField()
	print("field",field.name,field.dtype.toString())
		
	access=db.createAccessForBlockQuery()
	access.beginRead()
	
	# uploadBlock
	def uploadBlock(time, block_id):

			block = db.createBlockQuery(block_id, field, time,ord('r'),Aborted())
			bOk=db.executeBlockQueryAndWait(access, block)
			
			# could be that the block is not stored
			if not bOk:
				return
				
			# I'm storing  zipped block on S3
			decoded=Array.toNumPy(block.buffer,bShareMem=False)
			encoded=zlib.compress(decoded,zlib.Z_DEFAULT_COMPRESSION)
			filename=filename_template.format(time=time,field=field.name, block=block_id)
			
			# someone says it's better to reverse the filename to parallelize better
			if reverse_filename: 
				filename=filename[::-1]
					
			s3.uploadFile(filename,encoded)
	
	jobs=[]
	for time in timesteps:
		for block_id in range(db.getTotalNumberOfBlocks()):
			jobs.append(lambda args=(time,block_id): uploadBlock(*args))
			
	
	def AdvanceCallback(ndone):
			print("{:.2f}%".format(100.0*ndone/float(len(jobs))))

	RunJobsInParallel(jobs, advance_callback=AdvanceCallback ,nthreads=8)
		
		
	access.endRead()
	print("All Blocks uploaded")

	print("""
Use the following Wasabi config (change as needed):

<dataset name='wasabi-{bucket_name}' url='{destination}' >
	<access type='multiplex'>
		<access type='disk' chmod='rw' url='file://D:/visus-cache/foam/visus.idx' />
	  <access type="CloudStorageAccess" url="{destination}" chmod="r" compression="zip" layout="{layout}" />
	</access>
</dataset>
""".format(destination=destination,bucket_name=bucket_nam,layout=layout))	
		
	
# /////////////////////////////////////////////////////////////////////////
if __name__ == '__main__':	
	
	"""
	Some examples:
	
	python Samples/python/cloud_storage/wasabi.py copy-dataset-to-s3 --source C:/projects/OpenVisus/datasets/cat/gray.idx  --destination https://cat-gray.s3.us-west-1.wasabisys.com/visus.idx --username XXXXX --password YYYYY
	
	
	python Samples/python/cloud_storage/wasabi.py copy-dataset-to-s3 --source D:/GoogleSci/visus_dataset/2kbit1/zip/hzorder/visus.idx  --destination https://2kbit1.s3.us-west-1.wasabisys.com/visus.idx --username XXXXX --password YYYYY
	
	"""
	print("Got args",repr(sys.argv))
	
	action=sys.argv[1]
	action_args=sys.argv[2:]
	
	if action=="copy-dataset-to-s3":
	
		parser = argparse.ArgumentParser(description="cloud copy command.")
		
		# source dataset to copy
		parser.add_argument("--source", type=str, help="Source database", required=True,default="")
		
			# password
		parser.add_argument("--destination", type=str, help="cloud destination", required=True,default="") 
			
		# username
		parser.add_argument("--username", "--access-key",type=str, help="username", required=True,default="") 
		
		# password
		parser.add_argument("--password", "--secret-access-key",type=str, help="password", required=True,default="") 

		# filename template on the cloud
		parser.add_argument("--filename-template", type=str, help="Cloud filename template (do not change)", required=False,default="{time}/{field}/{block:016x}") 
		
		# row major
		parser.add_argument("--layout", type=str, help="Cloud layout, default is rowmajor (leave it as it is)", required=False,default="rowmajor") 
		
		# sometimes it will speed up things on the cloud
		parser.add_argument("--reverse-filename" , help="Reverse filename, sometimes you gain some speed reversing", action='store_true') 
		
		# acl 
		parser.add_argument("--acl", type=str, help="acl", required=False,default="public-read") 
		
		args = parser.parse_args(action_args)
		
		CopyDatasetToS3(
			source=args.source, 
			destination=args.destination, 
			username=args.username, 
			password=args.password, 
			filename_template=args.filename_template,
			layout=args.layout,
			reverse_filename=args.reverse_filename,
			acl=args.acl)
		
		print("All done")
		sys.exit(0)

	
		

