---
layout: default
title: OpenVisus.LoadTextDocument
parent: All Functions
grand_parent: Python Functions
nav_order: 2
---

# OpenVisus.LoadTextDocument

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