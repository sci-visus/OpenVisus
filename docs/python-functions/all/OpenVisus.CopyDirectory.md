---
layout: default
title: OpenVisus.CopyDirectory
parent: All Functions
grand_parent: Python Functions
nav_order: 2
---

# OpenVisus.CopyDirectory

Describe function here.

# Function Definition

```python
def CopyDirectory(src,dst):
	
	src=os.path.realpath(src)
	
	if not os.path.isdir(src):
		return
	
	CreateDirectory(dst)
	
	# problems with symbolic links so using shutil	
	dst=dst+"/" + os.path.basename(src)
	
	if os.path.isdir(dst):
		shutil.rmtree(dst,ignore_errors=True)
		
	shutil.copytree(src, dst, symlinks=True)				
```