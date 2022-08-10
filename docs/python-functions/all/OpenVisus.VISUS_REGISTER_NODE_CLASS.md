---
layout: default
title: OpenVisus.VISUS_REGISTER_NODE_CLASS
parent: All Functions
grand_parent: Python Functions
nav_order: 2
---

# OpenVisus.VISUS_REGISTER_NODE_CLASS

Describe function here.

# Function Definition

```python
def VISUS_REGISTER_NODE_CLASS(TypeName, creator):
# print("Registering Python class",TypeName)
    class PyNodeCreator(NodeCreator):
        def __init__(self,creator):
            super().__init__()
            self.creator=creator
        def createInstance(self):
            return self.creator()
    NodeFactory.getSingleton().registerClass(TypeName, PyNodeCreator(creator))
```