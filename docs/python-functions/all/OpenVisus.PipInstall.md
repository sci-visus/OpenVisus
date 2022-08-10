---
layout: default
title: OpenVisus.PipInstall
parent: All Functions
grand_parent: Python Functions
nav_order: 2
---

# OpenVisus.PipInstall

Describe function here.

# Function Definition

```python
def PipInstall(packagename,extra_args=[]):
	cmd=[sys.executable,"-m","pip","install","--progress-bar","off","--user",packagename]
	if extra_args: cmd+=extra_args
	print("# Executing",cmd)
	return_code=subprocess.call(cmd)
	return return_code==0
```