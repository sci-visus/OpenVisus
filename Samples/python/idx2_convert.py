
import os, sys
import numpy as np
import imageio
import skimage

# /////////////////////////////////////////////////////////////
if __name__=="__main__":
	"""
	Utility to convert idx2 ouptut (raw array) to an image
	see ReadMe for instructions
	"""
	in_filename=sys.argv[1]
	
	v=os.path.basename(in_filename).split("-") # I am assuming the name has a special pattern
	Name,Field,W,H,D,dtype=v[0],v[1],int(v[2].lstrip("[")),int(v[3]),int(v[4].rstrip("]")),v[5].lower()
	print(f"Name={Name} Field={Field} W={W} H={H} D={D} dtype={dtype}")
	data = np.fromfile(in_filename, dtype=dtype).reshape((D, H, H))
	# remap to 0,1
	m,M=np.min(data),np.max(data) 
	print("m",m,"M",M)
	data=(data-m)/(M-m)
	for Z in range(D):
		out_filename=os.path.splitext(in_filename)[0]+f".{Z:03d}.png"
		imageio.imwrite(out_filename, skimage.img_as_ubyte(data[Z,:,:]))
		print("wrote",out_filename)