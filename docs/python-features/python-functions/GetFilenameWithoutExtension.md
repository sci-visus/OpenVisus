---
layout: default
parent: Python OpenViSUS Functions
nav_order: 2
---

# GetFilenameWithoutExtension(filename)

Describe function here.

# Function Definition

```python
def GetFilenameWithoutExtension(filename):
	return os.path.splitext(os.path.basename(filename))[0]
```