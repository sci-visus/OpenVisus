---
layout: default
title: GetCommandOutput(cmd)
parent: All Functions
grand_parent: Python Functions
nav_order: 2
---

# GetCommandOutput(cmd)

Describe function here.

# Function Definition

```python
def GetCommandOutput(cmd):
	output=subprocess.check_output(cmd)
	if sys.version_info >= (3, 0): output=output.decode("utf-8")
	return output.strip()
```