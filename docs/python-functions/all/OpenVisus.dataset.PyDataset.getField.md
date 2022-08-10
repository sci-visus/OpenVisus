---
layout: default
title: OpenVisus.dataset.PyDataset.getField
parent: All Functions
grand_parent: Python Functions
nav_order: 2
---

# OpenVisus.dataset.PyDataset.getField

Describe function here.

# Function Definition

```python
	def getField(self,value=None):
		
		if value is None:
			return self.db.getField()

		if isinstance(value,str):
			return self.db.getField(value)
			
		return value
```