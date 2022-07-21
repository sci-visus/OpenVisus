---
layout: default
parent: Python OpenViSUS Functions
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