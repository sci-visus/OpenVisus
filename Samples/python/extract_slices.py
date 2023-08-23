
import os, sys
import numpy as np
import imageio
import skimage

# /////////////////////////////////////////////////////////////
if __name__=="__main__":

	in_filename,W,H,D,dtype=sys.argv[1:]
	W,H,D=[int(it) for it in (W,H,D)]
	data = np.fromfile(in_filename, dtype=dtype).reshape((D, H, H))

	# remap to 0,1
	m,M=np.min(data),np.max(data) 
	print("m",m,"M",M)
	data=(data-m)/(M-m)
	
	for Z in range(D):
		out_filename=os.path.splitext(in_filename)[0]+f".{Z:03d}.png"
		imageio.imwrite(out_filename, skimage.img_as_ubyte(data[Z,:,:]))
		print("wrote",out_filename)