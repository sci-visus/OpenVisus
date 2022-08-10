---
layout: default
title: OpenVisus.dataset.PyDataset.writeBlock
parent: All Functions
grand_parent: Python Functions
nav_order: 2
---

# OpenVisus.dataset.PyDataset.writeBlock

Describe function here.

# Function Definition

```python
	def writeBlock(self, block_id, time=None, field=None, access=None, data=None, aborted=Aborted()):
		Assert(access is not None)
		Assert(isinstance(data, numpy.ndarray))
		field=self.getField() if field is None else self.getField(field)	
		time = self.getTime() if time is None else time
		write_block = self.db.createBlockQuery(block_id, field, time, ord('w'), aborted)
		write_block.buffer=Array.fromNumPy(data,TargetDim=self.getPointDim(), bShareMem=True)
		# note write_block.buffer.layout is empty (i.e. rowmajor)
		self.executeBlockQueryAndWait(access, write_block)
		return write_block.ok()
```