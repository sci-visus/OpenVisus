---
layout: default
title: OpenVisus.dataset.PyDataset.getLogicSize
parent: All Functions
grand_parent: Python Functions
nav_order: 2
---

# OpenVisus.dataset.PyDataset.getLogicSize

Describe function here.

# Function Definition

```python
	def getLogicSize(self):
		p1,p2=self.getLogicBox()
		return numpy.subtract(p2,p1)
```