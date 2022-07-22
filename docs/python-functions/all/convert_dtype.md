---
layout: default
title: convert_dtype(value)
parent: All Functions
grand_parent: Python Functions
nav_order: 2
---

# convert_dtype(value)

Describe function here.

# Function Definition

```python
def convert_dtype(value):

	import numpy

# get first component
	if isinstance(value,DType):
		value=value.get(0).toString() 

	if isinstance(value,str):
		if value=="uint8":    return numpy.uint8
		if value=="int8":     return numpy.int8
		if value=="uint16":   return numpy.uint16
		if value=="int16":    return numpy.int16
		if value=="uint32":   return numpy.uint32
		if value=="int32":    return numpy.int32
		if value=="float32":  return numpy.float32
		if value=="float64":  return numpy.float64

	if isinstance(value,numpy):
		if value==numpy.uint8:   return "uint8"
		if value==numpy.int8:    return "int8"
		if value==numpy.uint16:  return "uint16"
		if value==numpy.int16:   return "int16"
		if value==numpy.uint32:  return "uint32"
		if value==numpy.int32:   return "int32"
		if value==numpy.float32: return "float32"
		if value==numpy.float64: return "float64"

	raise Exception("Internal error")
```