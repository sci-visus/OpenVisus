---
layout: default
title: ReadTextFile(filename)
parent: All Functions
grand_parent: Python Functions
nav_order: 2
---

# ReadTextFile(filename)

Describe function here.

# Function Definition

```python
def ReadTextFile(filename):
	file = open(filename, "r") 
	ret=file.read().strip()
	file.close()
	return ret
```