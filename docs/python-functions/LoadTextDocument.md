---
layout: default
parent: Python Functions
nav_order: 2
---

# LoadTextDocument(filename)

Describe function here.

# Function Definition

```python
def LoadTextDocument(filename):
	if not os.path.isfile(filename): return []
	file=open(filename,"rt")
	content=file.read()
	file.close()	
	return content
```