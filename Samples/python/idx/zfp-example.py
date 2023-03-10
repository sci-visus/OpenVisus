import shutil
import os ,sys, time, logging
from datetime import datetime
import numpy as np

sys.path.append(r"C:\projects\OpenVisus\build\RelWithDebInfo")
import OpenVisus as ov

# //////////////////////////////////////////
if __name__ == "__main__":

	# if you have only one writer
	os.environ["VISUS_DISABLE_WRITE_LOCK"]="1"

	logger= logging.getLogger("OpenVisus")
	ov.SetupLogger(logger, output_stdout=True)

	t1 = time.time()
	data=np.load("recon_combined_1_fullres.npy")
	depth,height,width=data.shape
	print(f"np.load done in {time.time()-t1} seconds dtype={data.dtype} shape={data.shape} c_size={width*height*depth*4:,}")


	idx_filename="tmp/visus.idx"
	shutil.rmtree(os.path.dirname(idx_filename), ignore_errors=True)
	fields=[ov.Field("data",str(data.dtype),"row_major")]
	db=ov.CreateIdx(url=idx_filename,dims=[width,height,depth],fields=fields,compression="raw")

	t1 = time.time()
	db.write(data)
	print(f"db.write (uncompressed) done in {time.time() - t1} seconds")

	# encoding-number-of-bits and decoding-number-of-bits
	# it will be written in the IDX file and used as the field.default_compression
	# this is needed since the IDX block header does not support/store number-of-bitblanes
	t1 = time.time()
	db.compressDataset(["zfp-precision=8-precision=8"]) 
	print(f"db.compressDataset done in {time.time()-t1} seconds")
