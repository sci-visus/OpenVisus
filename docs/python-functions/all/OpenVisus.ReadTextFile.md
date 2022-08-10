---
layout: default
title: OpenVisus.ReadTextFile
parent: All Functions
grand_parent: Python Functions
nav_order: 2
---

# OpenVisus.ReadTextFile

Describe function here.

# Function Definition

```python
def ReadTextFile(filename):
	file = open(filename, "r") 
	ret=file.read().strip()
	file.close()
	return ret
```