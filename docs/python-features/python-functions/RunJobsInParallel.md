---
layout: default
parent: Python OpenViSUS Functions
nav_order: 2
---

# RunJobsInParallel(jobs, advance_callback=None, nthreads=8)

Describe function here.

# Function Definition

```python
def RunJobsInParallel(jobs, advance_callback=None, nthreads=8):

	class MyThread(threading.Thread):

		# constructor
		def __init__(self):
			super(MyThread, self).__init__()

		# run
		def run(self):
			for job in self.jobs:
				self.jobDone(job())

	nthreads=min(nthreads,len(jobs))
	threads,results=[],[]
	for WorkerId in range(nthreads):
		thread=MyThread()
		threads.append(thread)
		thread.jobs=[job for I,job in enumerate(jobs)  if (I % nthreads)==WorkerId]
		thread.jobDone=lambda result: results.append(result)
		thread.start()
			
	while True:

		time.sleep(0.01)

		if len(results)==len(jobs):
			[thread.join() for thread in threads]
			return results

		if advance_callback:
				advance_callback(len(results))

```