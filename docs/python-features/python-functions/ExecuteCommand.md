---
layout: default
parent: Python OpenViSUS Functions
nav_order: 2
---

# ExecuteCommand(cmd)

Describe function here.

# Function Definition

```python
def ExecuteCommand(cmd):	
	"""
	note: shell=False does not support wildcard but better to use this version
	because quoting the argument is not easy
	"""
	print("# Executing command: ",cmd)
	return subprocess.call(cmd, shell=False)
```