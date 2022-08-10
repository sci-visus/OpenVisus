---
layout: default
title: OpenVisus.ExecuteCommand
parent: All Functions
grand_parent: Python Functions
nav_order: 2
---

# OpenVisus.ExecuteCommand

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