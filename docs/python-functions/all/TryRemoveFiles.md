---
layout: default
title: TryRemoveFiles(mask)
parent: All Functions
grand_parent: Python Functions
nav_order: 2
---

# TryRemoveFiles(mask)

Describe function here.

# Function Definition

```python
def TryRemoveFiles(mask):

	for filename in glob.glob(mask):
		try:
			os.remove(filename)
		except:
			pass
```