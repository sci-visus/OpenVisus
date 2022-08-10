---
layout: default
title: OpenVisus.TryRemoveFiles
parent: All Functions
grand_parent: Python Functions
nav_order: 2
---

# OpenVisus.TryRemoveFiles

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