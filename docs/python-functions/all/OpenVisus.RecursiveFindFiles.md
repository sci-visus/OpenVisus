---
layout: default
title: OpenVisus.RecursiveFindFiles
parent: All Functions
grand_parent: Python Functions
nav_order: 2
---

# OpenVisus.RecursiveFindFiles

Describe function here.

# Function Definition

```python
def RecursiveFindFiles(rootdir='.', pattern='*'):
  return [os.path.join(looproot, filename)
          for looproot, _, filenames in os.walk(rootdir)
          for filename in filenames
          if fnmatch.fnmatch(filename, pattern)]
```