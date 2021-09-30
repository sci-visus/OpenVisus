import h5py
import os,sys
import numpy as np

if False:

	h5_filename=r"testfile3.h5"

	print("Creating dataset...")
	with h5py.File(h5_filename, "w") as f:
		group = f.create_group("group")
		dataset = group.create_dataset("IntArray", shape=list(reversed([2048,2048,2048])), dtype=np.uint8)


	print("Reading dataset...")
	with h5py.File(h5_filename,'r') as f:
		dataset=f["group"]["IntArray"]
		print(repr(dataset))
		data=dataset[1024,:,:]
		print(repr(data))




h5_filename=r"D:\GoogleSci\visus_dataset\2kbit1\zip\rowmajor\visus.h5"
idx_filename=r"D:\GoogleSci\visus_dataset\2kbit1\zip\rowmajor\visus.idx"
with h5py.File(h5_filename, "w") as f:
  group = f.create_group("group")
  dataset = group.create_dataset(idx_filename, shape=list(reversed([2048,2048,2048])), dtype=np.uint8)

with h5py.File(h5_filename,'r') as f:
		dataset=f["group"][idx_filename]
		print(repr(dataset))
		data=dataset[1024,500:600,500:600]
		print(data.shape,data.dtype)	

print("all done")
