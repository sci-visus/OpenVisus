---
layout: default
title: OpenVisus.GetFilenameWithoutExtension
parent: All Functions
grand_parent: Python Functions
nav_order: 2
---

# OpenVisus.GetFilenameWithoutExtension

Describe function here.

# Function Definition

```python
def GetFilenameWithoutExtension(filename):
	return os.path.splitext(os.path.basename(filename))[0]
```