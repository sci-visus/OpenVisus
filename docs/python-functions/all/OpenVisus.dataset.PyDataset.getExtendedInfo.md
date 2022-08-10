---
layout: default
title: OpenVisus.dataset.PyDataset.getExtendedInfo
parent: All Functions
grand_parent: Python Functions
nav_order: 2
---

# OpenVisus.dataset.PyDataset.getExtendedInfo

Describe function here.

# Function Definition

```python
	def getExtendedInfo(self):
		pdim=self.getPointDim()
		p1=self.getLogicBox()[0]
		p2=self.getLogicBox()[1]
		center=[(p1[I]+p2[I])//2 for I in range(pdim)]
		dims=[(p2[I]-p1[I]) for I in range(pdim)]
		fields=[self.getField(it) for it in self.getFields()]
		timesteps=[int(it) for it in self.getTimesteps().asVector()]

		import datetime

		files=[]
		for filename in list(self.getAllFilenames()):
			size=FileUtils.getFileSize(Path(filename))
			if size>0:
				files.append({
					"filename" : filename, 
					"size" : size,
					"modification_time" : datetime.datetime.fromtimestamp(os.path.getmtime(filename)).strftime('%Y-%m-%d %H:%M:%S'),
					"creation_time" : datetime.datetime.fromtimestamp(os.path.getctime(filename)).strftime('%Y-%m-%d %H:%M:%S'),
				})

		ret={
			"url": self.getUrl(),
			"dimension": pdim,
			"logic_box" : self.getLogicBox(),
			"dims" : dims,
			"timesteps": timesteps,
			"fields" : [],
			"num_files" : len(files),
			"total_file_size" : sum([it["size"] for it in files]),
			"total_field_size": sum([field.dtype.getByteSize(PointNi(dims)) for field in fields]),
			"files": files,
		}

		for field in fields:
			dtype=field.dtype
			ranges=[dtype.getDTypeRange(I) for I in range(dtype.ncomponents())]
			ranges=[(r.From,r.To) if r.delta()>0 else (0,0) for r in ranges]

			ret["fields"].append({
				"name":field.name,
				"dtype":  dtype.toString(),
				"default_compression":field.default_compression,
				"default_layout":field.default_layout,
				"default_value":field.default_value,
				"filter":field.filter,
				"dtype_ranges":ranges,
				"total_field_size" : dtype.getByteSize(PointNi(dims)),
			})

		return ret
```