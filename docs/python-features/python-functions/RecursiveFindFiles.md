---
layout: default
parent: Python OpenViSUS Functions
nav_order: 2
---

# RecursiveFindFiles(rootdir='.', pattern='*')

Describe function here.

# Function Definition

```python
def RecursiveFindFiles(rootdir='.', pattern='*'):
  return [os.path.join(looproot, filename)
          for looproot, _, filenames in os.walk(rootdir)
          for filename in filenames
          if fnmatch.fnmatch(filename, pattern)]
```