
import OpenVisus as ov

ABORTED=ov.Aborted()
ABORTED.setTrue() 

# ///////////////////////////////////////////////////////////////////
def LoadDataset(url):
    return ov.LoadDataset(url)

# ///////////////////////////////////////////////////////////////////
class Aborted:
    
	# constructor
	def __init__(self,value=False):
		self.inner=ov.Aborted()
		if value: self.inner.setTrue()

	# setTrue
	def setTrue(self):
		self.inner.setTrue()

	# isTrue
	def isTrue(self):
		return self.inner.__call__()==ABORTED.__call__()

# ///////////////////////////////////////////////////////////////////
def ReadStats(reset=False):

	io =ov.File.global_stats()
	net=ov.NetService.global_stats()

	return {
		"io": {
			"r":io.getReadBytes(),
			"w":io.getWriteBytes(),
			"n":io.getNumOpen(),
		},
		"net":{
			"r":net.getReadBytes(), 
			"w":net.getWriteBytes(),
			"n":net.getNumRequests(),
		}
	}

	if reset:
		ov.File      .global_stats().resetStats()
		ov.NetService.global_stats().resetStats()


# //////////////////////////////////////////////////////
class Query:
    
	# constructor
	def __init__(self, db, timestep, field, logic_box, end_resolutions, aborted=None):
		self.db=db
		self.timestep=timestep
		self.field=field
		self.logic_box=logic_box
		self.end_resolutions=end_resolutions
		self.aborted=aborted.inner if aborted is not None else ov.Aborted()

	# begin
	def begin(self):

		box_ni=ov.BoxNi(ov.PointNi(self.logic_box[0]), ov.PointNi(self.logic_box[1]))
		field=self.db.getField(self.field)

		self.query = self.db.createBoxQuery(box_ni, field, self.timestep, ord('r'), self.aborted)

		if not self.query:
			return
			
		for H in self.end_resolutions:
			self.query.end_resolutions.push_back(H)

		self.db.beginBoxQuery(self.query)

	# isRunning
	def isRunning(self):
		return self.query and self.query.isRunning()

	# getCurrentResolution
	def getCurrentResolution(self):
		return self.query.getCurrentResolution() if self.isRunning() else 0

	# execute
	def execute(self,access):
		assert self.isRunning()
		if not self.db.executeBoxQuery(access, self.query):
			return None
		return ov.Array.toNumPy(self.query.buffer, bShareMem=False) 

	# next
	def next(self):
		assert self.isRunning()
		self.db.nextBoxQuery(self.query)