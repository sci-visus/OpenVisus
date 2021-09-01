from OpenVisus import *
import numpy as np
import os.path
import argparse
from skimage import io
from glob import glob


descript = """Attaching per-county data to the globale output"""
parser = argparse.ArgumentParser(description=descript)

parser.add_argument("-i", help="The list of input tiff files lexicographically ordered", nargs='+')
parser.add_argument("-ii", help="\"Wildcard string for all tiff files\"", type=str)
parser.add_argument("-sort", help="Re-sort the filenames explicitly by trying to extract a running number", action='store_true')
parser.add_argument("-o", help="Name of the output idx file", default="output.idx")
parser.add_argument("-direction", help="Indicate whether the tiff stack is sliced in [X|Y|Z] direction", default="Z", choices=['X','Y','Z'])
parser.add_argument("-type", help="Datatype in the image [int16,float32,float64]", default="int16", choices=['int16','float32','float64'])
parser.add_argument("-fieldname", help="The name of the channel", default='xray')
parser.add_argument("-slab", help="The number of slices written concurrentll", default=1,type=int)

args = parser.parse_args()

if args.i:
    tiff_files = args.i

if args.ii:
    tiff_files = glob(args.ii)
    tiff_files.sort()    
    
if len(tiff_files) > 1 and args.sort:
    prefix = tiff_files[0]
    postfix = tiff_files[0]
    
    for f in tiff_files[1:]:
        # If the current prefix is not a true prefix
        if prefix != f[:len(prefix)]:
            i = 0
            while prefix[i] == f[i]:
                i += 1
            prefix = prefix[:i]
        
        if postfix != f[-len(postfix):]:
            
            i = -1
            while postfix[i] == f[i]:
                i -= 1
            postfix = postfix[i+1:]
    
    start = len(prefix)
    stop = -len(postfix)
    
    tiff_files = sorted(tiff_files,key=lambda a: int(a[start:stop]))
    

output_name = args.o
direction = args.direction
fieldname = args.fieldname
slab = args.slab

if args.type == 'int8':
    type = np.uint8
    maximum = (1 << 8) - 1
elif args.type == 'int16':
    type = np.uint16
    maximum = (1 << 16) - 1
elif args.type == "float32":
    type = np.float32
    maximum = 1
else:
    type = np.float64
    maximum = 1
    
# First we will load a single slice and figure out the dimensions
img =  io.imread(tiff_files[0])

if direction == 'X':
    dim = [len(tiff_files),img.shape[1],img.shape[0]]
elif direction == 'Y':
    dim = [img.shape[1], len(tiff_files), img.shape[0]]
else:
    dim = [img.shape[1], img.shape[0], len(tiff_files)]

# Now create the idx file 

DbModule.attach()

idx_name = os.path.join(output_name)

idx_file = CreateIdx(url=idx_name,
                     dims=dim,
                     fields=[Field(str(fieldname),"float32")]
                     )

dataset = LoadDataset(idx_name)
if not dataset: raise Exception("Cannot load idx")

access = dataset.createAccess()
if not access: raise Exception("Cannot create access")


field = dataset.getField(fieldname)

# Now compute the target location
bottom = [0,0,0]

if direction == 'X':
    top = [0,dim[1],dim[2]]
    delta = Point[1,0,0]
    data = np.zeros(shape=[dim[2],dim[1],slab],dtype=np.float32)
elif direction == 'Y':
    top = [dim[0],0,dim[2]]
    delta = [0,1,0]
    data = np.zeros(shape=[dim[2],slab,dim[0]],dtype=np.float32)
else:
    top = [dim[0],dim[1],0]
    delta = [0,0,1]
    data = np.zeros(shape=[slab,dim[1],dim[0]],dtype=np.float32)
    

top = np.array(top,dtype=int)
bottom = np.array(bottom,dtype=int)
delta = np.array(delta,dtype=int)
    
count = 0 # The current slab size    
for file in tiff_files:
    
    
    slice = np.flip(np.array(io.imread(file).astype(np.float32))/maximum,0)
    
    if direction == 'X':
        data[:,:,count%slab] = slice
    elif direction == 'Y':
        data[:,count%slab,:] = slice
    else:
        data[count%slab,:,:] = slice
         

    count += 1
    
    if count % slab == 0:
        
        low = bottom + (count - slab)*delta
        high = top + count*delta
        target_box = BoxNi(PointNi(int(low[0]),int(low[1]),int(low[2])), 
                           PointNi(int(high[0]),int(high[1]),int(high[2])))
        
        print(low, high)

        dataset.write(data, field=field, logic_box=target_box)
        
# Now remember to write whatever remaining slices
if count % slab != 0:
        low = bottom + (count - count%slab)*delta
        high = top + count*delta
        target_box = BoxNi(PointNi(int(low[0]),int(low[1]),int(low[2])), 
                           PointNi(int(high[0]),int(high[1]),int(high[2])))

        print(low, high)
        
        # We need to make a copy because the array might not be contiguous
        if direction == 'X':
            data = np.array(data[:,:,:count%slab],dtype=np.float32)
        elif direction == 'Y':
            data = np.array(data[:,:count%slab:,:],dtype=np.float32)
        else:
            data = np.array(data[:count%slab:,:,:],dtype=np.float32)

        dataset.write(data, field=field, logic_box=target_box)

 
        
idx_file.compressDataset(["zip"])
DbModule.detach()







