---
layout: default
parent: Python OpenViSUS Functions
nav_order: 2
---

# VISUS_REGISTER_NODE_CLASS(TypeName, creator)

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