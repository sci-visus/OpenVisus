import os,sys,time,threading,queue,math, types,logging,copy
import numpy as np

from .utils import IsIterable,Clamp
from .backend import Query, Aborted

logger = logging.getLogger(__name__)

# ////////////////////////////////////////////////////////////////////////////////////////////////////////////
def GetAlignedBox(db, logic_box, H, slice_dir:int=None):
	ret=copy.deepcopy(logic_box)
	pdim=db.getPointDim()
	maxh=db.getMaxResolution()
	bitmask=db.getBitmask().toString()
	delta=[1,1,1]
	for B in range(maxh,H,-1):
		bit=ord(bitmask[B])-ord('0')
		A,B,D=ret[0][bit], ret[1][bit], delta[bit]
		D*=2
		A=int(math.floor(A/D))*D
		B=int(math.ceil (B/D))*D
		B=max(A+D,B)
		ret[0][bit] = A 
		ret[1][bit] = B
		delta[bit] = D
	  
	#  force to be a slice?
	if pdim==3 and slice_dir is not None:
		offset=ret[0][slice_dir]
		ret[1][slice_dir]=offset+0
		ret[1][slice_dir]=offset+1
		delta[slice_dir]=1
   
	num_pixels=[(ret[1][I]-ret[0][I])//delta[I] for I in range(pdim)]
	return ret, delta,num_pixels

# ////////////////////////////////////////////////////////////////////////////////////////////////////////////
def ExecuteQuery(args):

	db=args["db"]
	assert(db)

	pdim=db.getPointDim()
	assert pdim==2 or pdim==3 # todo other cases?

	maxh=db.getMaxResolution()
	bitmask=db.getBitmask().toString()
	dims=db.getLogicSize()

	access=args["access"]
	timestep=args.get("timestep",db.getTime())
	field=args.get("field",db.getField())
	logic_box=args["logic_box"]
	max_pixels=args.get("max_pixels",None)
	endh=args.get("endh",maxh)
	num_refinements=args.get("num_refinements",1)
	aborted=args.get("aborted",Aborted())

	logger.info(f"ExecuteQuery begin timestep={timestep} field={field} logic_box={logic_box} num_refinements={num_refinements} max_pixels={max_pixels} endh={endh}")

	if IsIterable(max_pixels):
		max_pixels=int(np.prod(max_pixels,dtype=np.int64))
	 
	# if box is not specified get the all box
	if logic_box is None:
		W,H,D=[int(it) for it in db.getLogicSize()]
		logic_box=[[0,0,0],[W,H,D]]
	  
	# fix logic box by cropping
	if True:
		p1,p2=list(logic_box[0]),list(logic_box[1])
		slice_dir=None
		for I in range(pdim):
			# *************** is a slice? *******************
			if pdim==3 and (p2[I]-p1[I])==1:
				assert slice_dir is None 
				slice_dir=I
				p1[I]=Clamp(p1[I],0,dims[I])
				p2[I]=p1[I]+1
			else:
				p1[I]=Clamp(int(math.floor(p1[I])),     0,dims[I])
				p2[I]=Clamp(int(math.ceil (p2[I])) ,p1[I],dims[I])
			assert p1[I]<p2[I]
		logic_box=(p1,p2)
	 
	# is view dependent? if so guess max resolution 
	if max_pixels:
		original_box=logic_box
		for H in range(maxh,0,-1):
			aligned_box,delta,num_pixels=GetAlignedBox(db,original_box,H, slice_dir=slice_dir)
			tot_pixels=np.prod(num_pixels,dtype=np.int64)
			if tot_pixels<=max_pixels*1.10:
				endh=H
				logger.info(f"ExecuteQuery Guess resolution H={H} original_box={original_box} aligned_box={aligned_box} delta={delta} num_pixels={repr(num_pixels)} tot_pixels={tot_pixels:,} max_pixels={max_pixels:,} end={endh}")
				logic_box=aligned_box
				break

	# this is the query I need
	logic_box,delta,num_pixels=GetAlignedBox(db,logic_box, endh, slice_dir=slice_dir)

	logic_box=[
		[int(it) for it in logic_box[0]],
		[int(it) for it in logic_box[1]]
	]

	end_resolutions=list(reversed([endh-pdim*I for I in range(num_refinements) if endh-pdim*I>=0]))

	# print("beginBoxQuery","box",logic_box.toString(),"field",field,"timestep",timestep,"end_resolutions",end_resolutions)
	t1=time.time()
	I,N=0,len(end_resolutions)
	query=Query(db, timestep, field, logic_box, end_resolutions, aborted)
	query.begin()
	while query.isRunning():

		data=query.execute(access)

		if data is None: 
			break

		# is a slice? I need to reduce the size (i.e. from 3d data to 2d data)
		if slice_dir is not None:
			dims=list(reversed(data.shape))
			assert dims[slice_dir]==1
			del dims[slice_dir]
			while len(dims)>2 and dims[-1]==1: dims=dims[0:-1] # remove right `1`
			data=data.reshape(list(reversed(dims))) 
    
		H=query.getCurrentResolution()
		msec=int(1000*(time.time()-t1))
		logger.info(f"ExecuteQuery got data {I}/{N} timestep={timestep} field={field} H={H} data.shape={data.shape} data.dtype={data.dtype} logic_box={logic_box} m={np.min(data)} M={np.max(data)} ms={msec}")
		yield {"I":I,"N":N,"timestep":timestep,"field":field,"logic_box":logic_box, "H":H, "data":data,"msec":msec}
		I+=1
		query.next()

	logger.info(f"ExecuteQuery read done")