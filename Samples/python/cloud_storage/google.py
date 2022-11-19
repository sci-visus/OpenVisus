"""

Follows instructions here:

https://cloud.google.com/storage/docs/reference/libraries#cloud-console

Make sure Google Storage API is  enabled

"""

import sys
import io
import boto3
import zlib
import multiprocessing 

import google
from google.cloud import storage

from OpenVisus import *

idx="D:/GoogleSci/visus_dataset/2kbit1/zip/rowmajor/visus.idx"
layout="" # row major 
bucket_name="2kbit1"

# change to point to your credential file
storage_client = storage.Client.from_service_account_json("./google_credentials.json")

# ///////////////////////////////////////////////////
def Main():
	
	db=LoadDataset(idx)
	
	try:
		storage_client.create_bucket(bucket_name)
	except google.api_core.exceptions.Conflict as ex:
		pass
		
	bucket = storage_client.bucket(bucket_name)
	
	body=db.getDatasetBody().toString().encode()
	bucket.blob("visus.idx").upload_from_string(body, content_type="application/octet-stream")
		
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
		
		bucket.blob(filename).upload_from_string(body, content_type="application/octet-stream")

		print("[{:8d}]".format(os. getpid()),"Uploaded",filename, "{:.2f}%".format(100.0*block_id/float(tot)))

	access.endRead()
	print("all done")


# /////////////////////////////////////////////////////////////////////////
if __name__ == '__main__':	
	Main()

"""

https://console.developers.google.com/apis/credentials

"+ Create credential" / Oath Client ID / 
ApplicationType: Desktop App
Name: openvisus-client

client_id="xxxxx"
client_secret="yyyyy"

Open the following URL in a browser:
echo "https://accounts.google.com/o/oauth2/auth?client_id=${client_id}&response_type=code&scope=https://www.googleapis.com/auth/devstorage.read_only&redirect_uri=urn:ietf:wg:oauth:2.0:oob"

authorization_code="zzzzz"

Generate a refresh token:

curl \
	--data "code=${authorization_code}" \
	--data "client_id=${client_id}" \
	--data "client_secret=${client_secret}" \
	--data "redirect_uri=urn:ietf:wg:oauth:2.0:oob" \
	--data "grant_type=authorization_code" \
	"https://www.googleapis.com/oauth2/v3/token"

refresh_token="wwwww"

THis is your connection string:
echo "https://storage.googleapis.com?client_id=${client_id}&client_secret=${client_secret}&refresh_token=${refresh_token}"

To open the dataset in the viewer, add this to your visus.config:

bucket_name="2kbit1"

cat << EOF
<dataset name='${bucket_name} on Google' url="https://storage.googleapis.com/${bucket_name}/visus.idx?alt=media&amp;client_id=${client_id}&amp;client_secret=${client_secret}&amp;refresh_token=${refresh_token}" >
	<access type="CloudStorageAccess"
		url="https://storage.googleapis.com?client_id=${client_id}&amp;client_secret=${client_secret}&amp;refresh_token=${refresh_token}"
		chmod="r"
		compression="zip"
		layout=""
		filename_template="/${bucket_name}/${time}/${field}/${block}"
		reverse_filename="false"
	/>
</dataset>
EOF

"""	