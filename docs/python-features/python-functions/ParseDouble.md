---
layout: default
parent: Python OpenViSUS Functions
nav_order: 2
---

# ParseDouble(value, default_value=0.0)

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