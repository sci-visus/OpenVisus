---
layout: default
title: OpenVisus.dataset.PyDataset.getFields
parent: All Functions
grand_parent: Python Functions
nav_order: 2
---

# OpenVisus.dataset.PyDataset.getFields

Describe function here.

# Function Definition

```python
	def getFields(self):
		return [field.name for field in self.db.getFields()]
```