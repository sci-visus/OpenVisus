"""

Copy a local dataset to Microsoft Azure Storage.

Prerequisites
python -m pip install azure-storage-blob

You need to create a storage account, I created `openvisus`:

https://docs.microsoft.com/en-us/azure/storage/blobs/storage-quickstart-blobs-python
https://docs.microsoft.com/en-us/python/api/azure-storage-blob/?view=azure-python

"""

import sys
import io
import boto3
import zlib
import multiprocessing 

from azure.core.exceptions import ResourceExistsError
from azure.storage.blob import *

from OpenVisus import *


idx="D:/GoogleSci/visus_dataset/2kbit1/zip/rowmajor/visus.idx"
layout="" # row major (!)
acl='public-read'
container_name="2kbit1"

connect_str=os.getenv('AZURE_STORAGE_CONNECTION_STRING')
client = BlobServiceClient.from_connection_string(connect_str)


# ///////////////////////////////////////////////////
def Main():
	
	db=LoadDataset(idx)
	
	# cannot delete/create in <30 seconds
	try:
		client.create_container(container_name)
		print("Container",container_name,"created")
	except ResourceExistsError:
		pass

	# Upload visus.idx:
	try:
		client.get_blob_client(container=container_name, blob="visus.idx").upload_blob(db.getDatasetBody().toString().encode())
	except ResourceExistsError:
		pass
	
	tot=db.getTotalNumberOfBlocks()
	time=db.getTime()
	field=db.getField()
	
	access=db.createAccessForBlockQuery()
	access.beginRead()
	for block_id in range(tot):
		block = db.createBlockQuery(block_id, field, time)
		bOk=db.executeBlockQueryAndWait(access, block)
		
		# could be that the block is not stored
		if not bOk:
			continue 
			
		# I'm storing  zipped block on S3
		decoded=Array.toNumPy(block.buffer,bShareMem=True)
		body=zlib.compress(decoded,zlib.Z_DEFAULT_COMPRESSION)
		filename="{time}/{field}/{block:016x}".format(time=int(time),field=field.name, block=block_id)

		try:
			client.get_blob_client(container=container_name, blob=filename).upload_blob(body)
		except ResourceExistsError:
			pass
			
		print("[{:8d}]".format(os. getpid()),"Uploaded",filename, "{:.2f}%".format(100.0*block_id/float(tot)))

	access.endRead()
	print("all done")


# /////////////////////////////////////////////////////////////////////////
if __name__ == '__main__':	
	Main()
	

"""
To open the dataset in the viewer, add this to your visus.config:

<dataset name='2kbit1 on Azure' url='https://<account>.blob.core.windows.net/<container_name>/visus.idx?access_key=xxxxyyyyzzz' >
	<access type="CloudStorageAccess"
		url='https://<account>.blob.core.windows.net?access_key=xxxxyyyyzzz' 
		chmod="r"
		compression="zip"
		layout=""
		filename_template="/<container_name>/${time}/${field}/${block}"
		reverse_filename="false" 
	/>
</dataset>
"""	