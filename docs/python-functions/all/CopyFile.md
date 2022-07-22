---
layout: default
title: CopyFile(src, dst)
parent: All Functions
grand_parent: Python Functions
nav_order: 2
---

# CopyFile(src, dst)

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