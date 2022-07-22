---
layout: default
title: CreateDirectory(value)
parent: All Functions
grand_parent: Python Functions
nav_order: 2
---

# CreateDirectory(value)

Describe function here.

# Function Definition

```python
def CreateDirectory(value):
	try: 
		os.makedirs(value)
	except OSError:
		if not os.path.isdir(value):
			raise
```