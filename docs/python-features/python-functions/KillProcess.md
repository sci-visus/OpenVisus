---
layout: default
parent: Python OpenViSUS Functions
nav_order: 2
---

# KillProcess(process)

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