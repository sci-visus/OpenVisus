---
layout: default
title: OpenVisus.dataset.PyDataset.__getattr__
parent: All Functions
grand_parent: Python Functions
nav_order: 2
---

# OpenVisus.dataset.PyDataset.__getattr__

Describe function here.

# Function Definition

```python
	def __getattr__(self,attr):
	    return getattr(self.db, attr)	
```