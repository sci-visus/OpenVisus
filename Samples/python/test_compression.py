
import sys,threading
import numpy as np
import urllib.request
import shutil
import os,sys,time
import OpenVisus as ov

# ////////////////////////////////////////////////////////////////////////////////
def RemoveDir(dir):
	while os.path.isdir(dir):
		shutil.rmtree(dir,ignore_errors=True) 
  

# ////////////////////////////////////////////////////////////////////////////////
def RemoveFile(filename):
	while os.path.isfile(filename):
		os.remove(filename)
  
# ////////////////////////////////////////////////////////////////////////////////
def Main(url=None,shape=None,arco="modvisus"):

	local_filename=os.path.basename(url)
	urllib.request.urlretrieve(url, local_filename) if not os.path.isfile(local_filename) else None
	data = np.fromfile(local_filename, dtype=np.uint8).reshape(*shape)

	idx_dir=os.path.join("./tmp",os.path.splitext(local_filename)[0])
	idx_filename=f"{idx_dir}.idx"

	RemoveDir(idx_dir)
	RemoveFile(idx_filename)

	db = ov.CreateIdx(url=idx_filename,  dims=list(reversed(data.shape)),  fields=[ov.Field('data','uint8')],  arco=arco)
	db.write(data)

	# read (need to explicitely specify to not use compression)
	if True:
		access=db.createAccessForBlockQuery()
		access.compression="raw"
		check=db.read(access=access)
		if not (data == check).all():
			error_msg=f"ERROR: Failed read BEFORE compression arco={arco}"
			print(error_msg)
			raise Exception(error_msg)

	# read with compression
	if True:
		db.compressDataset()
		check=db.read()
		if not (data == check).all():
			error_msg=f"ERROR: Failed read AFTER compression arco={arco}"
			print(error_msg)
			raise Exception(error_msg)


# /////////////////////////////////////////////////////////////////
if __name__=="__main__":
	"""
	Example: python3 Samples/samples/python/test_compression.py
	"""
	max_seconds=60*10 # run for 10 minute
	url='https://klacansky.com/open-scivis-datasets/hydrogen_atom/hydrogen_atom_128x128x128_uint8.raw'
	shape=(128, 128, 128)
	I,T1=0,time.time()
	while (time.time()-T1)<max_seconds: 
		arco="1mb" if I % 2==1 else "modvisus"
		Main(url=url,shape=shape, arco=arco)
		print(f"Step I={I} arco={arco} done")
		I+=1   