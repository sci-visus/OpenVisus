---
layout: default
parent: Python OpenViSUS Functions
nav_order: 2
---

# GuessUniqueFilename(pattern)

Describe function here.

# Function Definition

```python
def GuessUniqueFilename(pattern):
	I=0
	while True:
		filename=pattern %(I,)
		if not os.path.isfile(filename): 
			return filename
		I+=1
```