"""
Copy a local dataset to Amazon S3.

Prerequisites.
Create a user for S3 access https://console.aws.amazon.com/iam/home?#/users$new?step=details (change as needed):

Username: python-s3
Type: Programmatic access
Attach existing policies: AmazonS3FullAccess
Run aws configure (change as needed):

AWS Access Key ID [None]: xxxxxxxxx
AWS Secret Access Key [None]: yyyyyy
Default region name [None]: us-east-1
Default output format [None]:
"""

import sys
import io
import boto3
import zlib
import multiprocessing 


from OpenVisus import *

#idx="D:/GoogleSci/visus_dataset/david_subsampled/visus.idx"
#layout="hzorder"
#acl='private'
# bucket_name='david-subsampled'

idx="D:/GoogleSci/visus_dataset/2kbit1/zip/rowmajor/visus.idx"
layout="" # row major (!)
acl='public-read'
bucket_name='2kbit1'

# use or not multiprocessing
num_process=1

# Add a function to delete recursively a bucket from Amazon S3:
def DeleteBucket(bucket_name):
	s3 = boto3.resource('s3')
	bucket = s3.Bucket(bucket_name)
	try:
		bucket.objects.all().delete()
	except:
		pass
	try:
		bucket.object_versions.delete()
	except:
		pass


# Add a function to load files to a bucket, files must be a list like [(filename,body), (filename,body),..] 
def UploadFilesToBucket(bucket_name, tot, files):
	s3 = boto3.resource(service_name='s3')
	bucket=s3.Bucket(bucket_name)
	for I, (filename, body) in enumerate(files):
		bucket.put_object(Key=filename, Body=body, ACL=acl)
		perc=100.0*I/float(tot)
		print("[{:8d}]".format(os. getpid()),"Uploaded",filename, "{:.2f}%".format(perc))



# Add a function which reads all OpenVisus block and yield them with a unique filename:
def ReadBlocks(db, blocks,filename_template="{time}/{field}/{block:016x}", reverse_filename=False):
	time=db.getTime()
	field=db.getField()
	access=db.createAccessForBlockQuery()
	access.beginRead()
	for block_id in blocks:
		block = db.createBlockQuery(block_id, field, time)
		bOk=db.executeBlockQueryAndWait(access, block)
		if not bOk:
			continue # could be that the block is not stored
		# I'm storing  zipped block on S3
		decoded=Array.toNumPy(block.buffer,bShareMem=True)
		encoded=zlib.compress(decoded,zlib.Z_DEFAULT_COMPRESSION)
		filename=filename_template.format(time=int(time),field=field.name, block=block_id)
		if reverse_filename: 
			filename=filename[::-1]
		yield (filename, encoded)
	access.endRead()


def Uploader(idx,blocks):
	UploadFilesToBucket(bucket_name, len(blocks), ReadBlocks(LoadDataset(idx), blocks))
	

	
# /////////////////////////////////////////////////////////////////////////
if __name__ == '__main__':	
		
	# Create Amazon S3 bucket:
	DeleteBucket(bucket_name)
	boto3.resource('s3').create_bucket(Bucket=bucket_name, ACL=acl)

	# Upload visus.idx:
	db=LoadDataset(idx)
	body=db.getDatasetBody().toString().encode()
	boto3.resource('s3').Bucket(bucket_name).put_object(Key='visus.idx', Body=body, ACL=acl)

	# Upload blocks to Amazon S3:
	# you can upload using multi-processing too (https://stackoverflow.com/questions/51310604/how-to-use-boto3-client-with-python-multiprocessing)
	blocks=list(range(db.getTotalNumberOfBlocks()))
	if num_process==1:
	    Uploader(idx,blocks)
	else:
	    multiprocessing.Pool(num_process).starmap(Uploader, [ (idx, blocks[I::num_process]) for I in range(num_process)])
	print("Blocks uploaded")


"""
To open the dataset in the viewer, add this to your visus.config:

<dataset name='<bucket_name> on Amazon S3' url='http://<bucket_name>.s3.amazonaws.com/visus.idx?username=xxx&amp;password=yyy' >
  <access type="CloudStorageAccess"
  	url="http://<bucket_name>.s3.amazonaws.com?username=xxx&amp;password=yyy"
  	chmod="r"
  	compression="zip"
		layout="hzorder"
  	filename_template="/${time}/${field}/${block}"
		reverse_filename="false"
  />
</dataset>

Consider using reverse_filename=True' if you want to speed up the download from S3 
see https://aws.amazon.com/blogs/aws/amazon-s3-performance-tips-tricks-seattle-hiring-event/
"""

