from OpenVisus import *

import botocore,boto3

import configparser, urllib3
from   urllib.parse import parse_qs, urlparse

# /////////////////////////////////////////////////////////////////////////////////////////
class S3:

	# constructor
	def __init__(self, profile=None, no_verify_ssl=None, url=None, num_connections=None):

		if profile is None:
			profile=os.environ.get("AWS_PROFILE",None)

		if no_verify_ssl is None:
			no_verify_ssl=bool(os.environ.get("NO_VERIFY_SSL",False))

		if num_connections is None:
			num_connections=int(os.environ.get("AWS_NUM_CONNECTIONS",10))

		# or you use an url with query params
		if url:
      
			__bucket,__key,qs=S3.parseUrl(url)

			if 'profile' in qs:
				profile=qs['profile'][0]
    
			if 'no-verify-ssl' in qs:
				no_verify_ssl=True
	
			if 'num-connections' in qs:
				num_connections=int(qs['num-connections'][0])

		self.profile=profile if profile else None
		self.session=boto3.session.Session(profile_name=self.profile)
  
		self.no_verify_ssl=no_verify_ssl if no_verify_ssl else False
		if self.no_verify_ssl:
			urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)
  
		self.num_connections=num_connections
		#logger.info(f"Creating S3 url={url} with {num_connections} number of connections")
		botocore_config = botocore.config.Config(max_pool_connections=self.num_connections)
  
		self.endpoint_url=S3.guessEndPoint(profile) if profile else None
  
		self.client=self.session.client('s3',
			endpoint_url=self.endpoint_url, 
			config=botocore_config, 
			verify=False if no_verify_ssl else True)

	# parseUrl
	@staticmethod
	def parseUrl(url, is_folder=False):
		parsed=urlparse(url)
		qs=parse_qs(parsed.query, keep_blank_values=True)
		scheme, bucket, key, qs=parsed.scheme, parsed.netloc, parsed.path, qs  
		assert scheme=="s3"
		key=key.lstrip("/")
		if is_folder and key and not key.endswith("/"):
			key=key+"/"
		return bucket,key,qs

	# guessEndPoint
	@staticmethod
	def guessEndPoint(profile,aws_config_filename='~/.aws/config'):
		"""
		[profile cloudbank]
		region = us-west-1
		s3 =
		    endpoint_url = https://s3.us-west-1.amazonaws.com
		"""
		config = configparser.ConfigParser()
		config.read_file(open(os.path.expanduser(aws_config_filename)))
		v=[v for k,v in config.items(f'profile {profile}') if k=="s3"]
		if not v: return None
		body="[Default]\n"+v[0]
		config = configparser.ConfigParser()
		config.read_string(body)
		return config.get('Default','endpoint_url')

	# createBucket
	def createBucket(self,bucket_name):
		self.client.create_bucket(Bucket=bucket_name)

	# getObject
	def getObject(self,url):
		bucket,key,qs=S3.parseUrl(url)
		t1=time.time()
		body = self.client.get_object(Bucket=bucket, Key=key)['Body'].read()
		logger.debug(f"s3-get_object {key} done in {time.time()-t1} seconds")
		return body

	# putObject
	def putObject(self, url, binary_data):
		bucket,key,qs=S3.parseUrl(url)
		t1=time.time()
		ret=self.client.put_object(Bucket=bucket, Key=key,Body=binary_data)
		assert ret['ResponseMetadata']['HTTPStatusCode'] == 200
		sec=time.time()-t1
		logger.debug(f"s3-put-object {url} done in {sec} seconds")

	# touchObject
	def touchObject(self,url):
		self.putObject(url,"0") # I don't think I can have zero-byte size on S3

	# downloadObject
	def downloadObject(self, url, filename, force=False,nretries=5):
		bucket,key,qs=S3.parseUrl(url)
		
		if not force and os.path.isfile(filename): 
			return 
		t1=time.time()
		os.makedirs( os.path.dirname(filename), exist_ok=True)	
		for retry in range(nretries):
			try:
				self.client.download_file(bucket, key, filename)
				break
			except:
				if retry==(nretries-1):
					logger.error(f"Cannot download_file({bucket}, {key}, {filename})")
					raise
				else:
					time.sleep(0.500)
		
		size_mb=os.path.getsize(filename)//(1024*1024)
		sec=time.time()-t1
		logger.debug(f"s3-download-file {url} {filename}  {size_mb} MiB done in {sec} seconds")

	# uploadObject
	def uploadObject(self, filename, url):
		bucket,key,qs=S3.parseUrl(url)
		size_mb=os.path.getsize(filename)//(1024*1024)
		t1=time.time()
		self.client.upload_file(filename, bucket, key)
		# assert self.existObject(key)
		sec=time.time()-t1
		logger.debug(f"s3-upload-file {filename} {url} {size_mb}MiB done in {sec} seconds")

	# existObject
	def existObject(self, url):
		bucket,key,qs=S3.parseUrl(url)
		try:
			self.client.head_object(Bucket=bucket, Key=key)
			return True
		except:
			return False

	# deleteFolder
	def deleteFolder(self, url):
		bucket,key,qs=S3.parseUrl(url)
		t1=time.time()
		logger.info(f"S3 deleting folder {url}...")
		while True:
			filtered = self.client.list_objects(Bucket=bucket, Prefix=f"{key}/").get('Contents', [])
			if len(filtered)==0: break
			self.client.delete_objects(Bucket=bucket, Delete={'Objects' : [{'Key' : obj['Key'] } for obj in filtered]})
		sec=time.time()-t1
		logger.info(f"S3 delete folder {url} done in {sec} seconds")

	# listObjects
	def listObjects(self,url):
		ret=[]
	  
		# return the list of buckets
		if not url or url=="s3://":
			for it in self.client.list_buckets()['Buckets']:
				bucket=it['Name']
				it['url']=f"s3://{bucket}/"
				ret.append(it)
			return ret 

		# start from a bucket name
		assert url.startswith("s3://")
		if not url.endswith("/"): 
			url+="/"
   
		v=url[5:].split("/",1)
		bucket,key=v[0],v[1] if len(v)>1 else ""
		response = self.client.list_objects(Bucket=bucket, Prefix=key, Delimiter='/')
  
		# folders (end with /) have (Prefix,)
		for it in response.get('CommonPrefixes',[]):
			it['url']=f"s3://{bucket}/{it['Prefix']}"
			ret.append(it)
	  
	  # objects have (ETag,Key,LastModified,Owner.DisplayName,Size,StorageClass,)
		for it in response.get('Contents',[]):
			it['url']=f"s3://{bucket}/{it['Key']}"
			# there is an item which is the folder itself
			if it['url']!=url:
				ret.append(it)

		return ret

	# listObjectsInParallel (set num_connections properly)
	def listObjectsInParallel(self, url):
		"""
		Example
			import OpenVisus as ov
			import OpenVisus.s3 
			s3=OpenVisus.s3.S3(profile="wasabi",num_connections=64)
			for it in s3.listObjectsInParallel("s3://Pania_2021Q3_in_situ_data"):
			   print(it)
			   break
		"""
		bucket, key, qs=S3.parseUrl(url, is_folder=True)
		known_folders=set()
		pool=WorkerPool(self.num_connections)
		lock=multiprocessing.Lock()

		def ListFolderTask(folder, continuation_token=None):
	  
				# cannot be more than 1000 
				kwargs=dict(Bucket=bucket, Prefix=folder['Prefix'], Delimiter='/', MaxKeys=1000) 
		  
				if continuation_token: 
					kwargs['ContinuationToken']=continuation_token
		    
				# get network reponse
				resp = self.client.list_objects_v2(**kwargs) 
				files  =[file for file in resp.get('Contents',[]) if file['Key'][-1]!='/'] # remove the current directory
				folders=resp.get('CommonPrefixes',[])
				
				# continue with the current one
				next_continuation_token = resp.get('NextContinuationToken',None)
				if next_continuation_token:
					pool.pushTask(functools.partial(ListFolderTask,folder,next_continuation_token))  
	    
				# remove known folders
				with lock:
					folders=[folder for folder in folders if not folder['Prefix'] in known_folders]
					for folder in folders:
						known_folders.add(folder['Prefix'])
	   
				# add subfolders
				for folder in folders:
					pool.pushTask(functools.partial(ListFolderTask,folder))

				return (files,folders)

		# start from the root
		pool.pushTask(functools.partial(ListFolderTask, {'Prefix': key}) )
		pool.start()
		return pool

	# sync (just for testing, better to use external tools unless you need an adhoc order)
	def sync(self,local_dir,remote_dir):
		opt=""
		if self.profile:
			opt+=' --profile ' + self.profile
		if self.endpoint_url:
			opt+=' --endpoint-url ' + self.endpoint_url
		if self.no_verify_ssl:
			opt+=' --no-verify-ssl'
		cmd=f"aws s3  {opt} sync {local_dir} {remote_dir} --only-show-errors --no-progress"
		RunShellCommand(cmd)

	# downloadImage (NOT TESTED)
	def downloadImage(self, url):
		import imageio
		ret=imageio.imread(io.BytesIO(self.getObject(url)))
		return ret

	# uploadImage (NOT TESTED)
	def uploadImage(self, img, url):
		bucket,key,qs=S3.parseUrl(url)
		buffer = io.BytesIO()
		ext = os.path.splitext(key)[-1].strip('.').upper()
		img.save(buffer, ext)
		buffer.seek(0)
		self.putObject(url, buffer)

	# computeStatistics
	def computeStatistics(self,prefix):
		s3=S3(num_connections=128)
		num_files,num_folders,tot_size=0,0,0
		t1=time.time()
		for (files,folders) in self.listObjectsInParallel(prefix):
			num_folders+=len(folders)
			for file in files:
				num_files+=1
				tot_size+=int(file['Size'])
			if time.time()-t1>10.0:
				logger.info(f"num_files={num_files:,} tot_size={tot_size:,}...")
				t1=time.time()
		logger.info(f"num_files={num_files:,} tot_size={tot_size:,}")
