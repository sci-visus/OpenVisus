---
layout: default
title: OpenVisus.ParseDouble
parent: All Functions
grand_parent: Python Functions
nav_order: 2
---

# OpenVisus.ParseDouble

Describe function here.

# Function Definition

```python
def ParseDouble(value,default_value=0.0):

	if isinstance(value,str) and len(value)==0:
		return default_value
	try:
		return float(value)
	except:
		return default_value
```