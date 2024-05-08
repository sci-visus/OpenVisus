
import os, sys
import numpy as np
import imageio
import skimage

# /////////////////////////////////////////////////////////////
if __name__=="__main__":

	filename,W,H,D,dtype,prefix=sys.argv[1:]
	shape=list(reversed([int(it) for it in (W,H,D)]))
	print(f"filename={filename} dtype={dtype} shape={shape} prefix={prefix}")
	data = np.fromfile(filename, dtype=dtype).reshape(shape)

	# remap to 0,1 and write as uuint8
	m,M=np.min(data),np.max(data) 
	print("m",m,"M",M)
	data=(data-m)/(M-m)
	
	os.makedirs(prefix,exist_ok=True)
	for Z in range(data.shape[0]):
		out_filename=f"{prefix}/{Z:03d}.png"
		imageio.imwrite(out_filename, skimage.img_as_ubyte(data[Z,:,:]))
		print("wrote",out_filename)