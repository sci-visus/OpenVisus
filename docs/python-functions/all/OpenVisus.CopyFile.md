---
layout: default
title: OpenVisus.CopyFile
parent: All Functions
grand_parent: Python Functions
nav_order: 2
---

# OpenVisus.CopyFile

Describe function here.

# Function Definition

```python
def CopyFile(src,dst):
	
	src=os.path.realpath(src) 
	dst=os.path.realpath(dst)		
	
	if src==dst or not os.path.isfile(src):
		return		

	CreateDirectory(os.path.dirname(dst))
	shutil.copyfile(src, dst)	
```