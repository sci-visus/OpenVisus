---
layout: default
parent: Python OpenViSUS Functions
nav_order: 2
---

# LoadDataset(url)

Describe function here.

# Function Definition

```python
def LoadDataset(url):
	from OpenVisus.dataset import PyDataset
	return PyDataset(LoadDatasetCpp(url))

```