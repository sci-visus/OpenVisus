---
layout: default
title: OpenVisus.LoadDataset
parent: All Functions
grand_parent: Python Functions
nav_order: 2
---

# OpenVisus.LoadDataset

Describe function here.

# Function Definition

```python
def LoadDataset(url):
	from OpenVisus.dataset import PyDataset
	return PyDataset(LoadDatasetCpp(url))
```