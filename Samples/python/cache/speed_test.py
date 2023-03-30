import os,sys,time,datetime,threading,queue,random,copy,math, types, multiprocessing

import OpenVisus as ov

# //////////////////////////////////////////////////////////////////////////////////////
def CacheSpeedTest(urls, num_query_per_db=6, num_threads=3):

	# I am using one thread for direction, I don't think it makes sense to use other values
	assert num_threads==3

	datasets=[ov.LoadDataset(url) for url in urls]

	def RunTask(dir, ):
		query=QueryNode()
		query.startThread()
		query.disableOutputQueue()
		dims=datasets[0].getLogicSize()
		logic_box=[[0,0,0], dims]
		
		# for each dataset
		for scan,db in enumerate(datasets):
			access=db.createAccessForBlockQuery()
			
			# create num_queries_per db
			for offset in range(0,dims[dir],dims[dir]//num_query_per_db):	
				logic_box[0][dir]=offset+0
				logic_box[1][dir]=offset+1 
				query.pushJob(db=db, access=access, timestep=db.getTime(), field=db.getField(), logic_box=logic_box, max_pixels=1024*768, num_refinements=4, aborted=ov.Aborted())
				
				# wait for completition
				query.waitIdle()
				
		query.stopThread()	

	QueryNode.stats.startQuery()
	multiprocessing.pool.ThreadPool(num_threads).map(RunTask, range(3)) 
	QueryNode.stats.stopQuery()
 
# //////////////////////////////////////////////////////////////////////////////////////
if __name__=="__main__":
	urls=sys.argv[1:]
	CacheSpeedTest(urls)