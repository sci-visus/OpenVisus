---
layout: default
title: OpenVisus.CreateDirectory
parent: All Functions
grand_parent: Python Functions
nav_order: 2
---

# OpenVisus.CreateDirectory

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