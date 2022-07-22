---
layout: default
title: SaveTextDocument(filename, content)
parent: All Functions
grand_parent: Python Functions
nav_order: 2
---

# SaveTextDocument(filename, content)

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