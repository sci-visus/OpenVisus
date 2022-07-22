---
layout: default
title: WriteTextFile(filename, content)
parent: All Functions
grand_parent: Python Functions
nav_order: 2
---

# WriteTextFile(filename, content)

Describe function here.

# Function Definition

```python
def WriteTextFile(filename,content):
	if not isinstance(content, str):
		content="\n".join(content)+"\n"
	CreateDirectory(os.path.dirname(os.path.realpath(filename)))
	file = open(filename,"wt") 
	file.write(content) 
	file.close() 		
```