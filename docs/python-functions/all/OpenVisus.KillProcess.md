---
layout: default
title: OpenVisus.KillProcess
parent: All Functions
grand_parent: Python Functions
nav_order: 2
---

# OpenVisus.KillProcess

Describe function here.

# Function Definition

```python
def KillProcess(process):
	if not process:
		return
	try:
		process.kill()
	except:
		pass	
```