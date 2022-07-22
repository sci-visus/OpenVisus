---
layout: default
title: RemoveFiles(pattern)
parent: All Functions
grand_parent: Python Functions
nav_order: 2
---

# RemoveFiles(pattern)

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