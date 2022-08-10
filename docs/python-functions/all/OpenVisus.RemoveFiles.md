---
layout: default
title: OpenVisus.RemoveFiles
parent: All Functions
grand_parent: Python Functions
nav_order: 2
---

# OpenVisus.RemoveFiles

Describe function here.

# Function Definition

```python
def RemoveFiles(pattern):
	files=glob.glob(pattern)
	print("Removing files",files)
	for it in files:
		if os.path.isfile(it):
			os.remove(it)
		else:
			shutil.rmtree(os.path.abspath(it),ignore_errors=True)		
```