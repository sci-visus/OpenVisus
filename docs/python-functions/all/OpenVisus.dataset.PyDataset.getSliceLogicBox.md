---
layout: default
title: OpenVisus.dataset.PyDataset.getSliceLogicBox
parent: All Functions
grand_parent: Python Functions
nav_order: 2
---

# OpenVisus.dataset.PyDataset.getSliceLogicBox

Describe function here.

# Function Definition

```python
	def getSliceLogicBox(self,axis,offset):
		ret=self.getLogicBox()
		p1[axis]=offset+0
		p1[axis]=offset+1
		return (p1,p2)
```