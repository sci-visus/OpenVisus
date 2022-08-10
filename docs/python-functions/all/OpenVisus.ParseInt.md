---
layout: default
title: OpenVisus.ParseInt
parent: All Functions
grand_parent: Python Functions
nav_order: 2
---

# OpenVisus.ParseInt

Describe function here.

# Function Definition

```python
def ParseInt(value,default_value=0):

	if isinstance(value,str) and len(value)==0:
		return default_value
	try:
		return int(value)
	except:
		return default_value			
```