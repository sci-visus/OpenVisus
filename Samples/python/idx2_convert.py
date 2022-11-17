
import os, sys
import numpy as np
import imageio

# /////////////////////////////////////////////////////////////
if __name__=="__main__":
	in_filename=sys.argv[1]
	out_filename=sys.argv[2]
	v=os.path.basename(in_filename).split("-")
	Name,Field,W,H,D,dtype=v[0],v[1],int(v[2].lstrip("[")),int(v[3]),int(v[4].rstrip("]")),v[5].lower()
	print(f"Name={Name} Field={Field} W={W} H={H} D={D} dtype={dtype}")
	assert(D==1)
	data = np.fromfile(in_filename, dtype=dtype).reshape((W, H))
	imageio.imwrite(out_filename, data)
	print("Wrote",out_filename)