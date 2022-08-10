---
layout: default
title: OpenVisus.dataset.PyDataset.readBlock
parent: All Functions
grand_parent: Python Functions
nav_order: 2
---

# OpenVisus.dataset.PyDataset.readBlock

Describe function here.

# Function Definition

```python
	def readBlock(self, block_id, time=None, field=None, access=None, aborted=Aborted()):
		Assert(access)
		field=self.getField() if field is None else self.getField(field)	
		time = self.getTime() if time is None else time
		read_block = self.db.createBlockQuery(block_id, field, time, ord('r'), aborted)
		self.executeBlockQueryAndWait(access, read_block)
		if not read_block.ok(): return None
		self.db.convertBlockQueryToRowMajor(read_block) # default is to change the layout to rowmajor
		return Array.toNumPy(read_block.buffer, bShareMem=False)
```