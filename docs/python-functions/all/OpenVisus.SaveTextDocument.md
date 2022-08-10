---
layout: default
title: OpenVisus.SaveTextDocument
parent: All Functions
grand_parent: Python Functions
nav_order: 2
---

# OpenVisus.SaveTextDocument

Describe function here.

# Function Definition

```python
def SaveTextDocument(filename,content):
	try:
		os.makedirs(os.path.dirname(filename),exist_ok=True)
	except:
		pass

	file=open(filename,"wt")
	file.write(content)
	file.close()		
```