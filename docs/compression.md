# TESTS to run


![Diagram](https://raw.githubusercontent.com/sci-visus/OpenVisus/master/docs/compression.png)

## [OK] visusviewer 

Note: to test different configurations, remember to remove the `visus.idx` file.
This is because I am usually not overwriting the file in case it exists, and IDX get the compression
info from the block header

[OK] (IdxDiskAccess) (openvisus-server) All local files contains multiple blocks with compression disabled
http://atlantis.sci.utah.edu/mod_visus?dataset=2kbit1&cached=idx&cache_compression=raw

[OK] (IdxDiskAccess) (openvisus-server)All local files contains multiple blocks with compression enabled:
http://atlantis.sci.utah.edu/mod_visus?dataset=2kbit1&cached=idx&cache_compression=zip

[OK] (IdxDiskAccess) (wasabi)All local files contains multiple blocks with compression disabled
https://s3.us-west-1.wasabisys.com/nsdf/visus-datasets/2kbit1/1mb/visus.idx?profile=wasabi&cached=idx&cache_compression=raw

[OK] (IdxDiskAccess) (wasabi)All local files contains multiple blocks with compression enabled:
https://s3.us-west-1.wasabisys.com/nsdf/visus-datasets/2kbit1/1mb/visus.idx?profile=wasabi&cached=idx&cache_compression=zip

[OK] (DiskAccess) (openvisus-server) All local files contains one block with compression disabled 
http://atlantis.sci.utah.edu/mod_visus?dataset=2kbit1&cached=arco&cache_compression=raw

[OK] (DiskAccess) (openvisus-server) All local files contains one block with compression disabled 
http://atlantis.sci.utah.edu/mod_visus?dataset=2kbit1&cached=arco&cache_compression=zip

[OK] (DiskAccess) (wasabi) All local files contains one block with compression disabled
https://s3.us-west-1.wasabisys.com/nsdf/visus-datasets/2kbit1/1mb/visus.idx?profile=wasabi&cached=arco&cache_compression=raw

[OK] (DiskAccess) (wasabi) All local files contains one block with compression disabled
https://s3.us-west-1.wasabisys.com/nsdf/visus-datasets/2kbit1/1mb/visus.idx?profile=wasabi&cached=arco&cache_compression=zip

# [OK] test-idx 

300 seconds of self-testing


# [OK] openvisus-quick-tour

All works fine


# [OK] compress-dataset 

```
set PYTHONPATH=build\RelWithDebugInfo
set PATH=%PATH%;build\RelWithDebInfo\OpenVisus\bin
set VISUS_CPP_VERBOSE=1
```

## Arco==0

[OK] Python/zip (convert.py CompressModVisusDataset)

```
visus create tmp/cat/visus.idx --box "0 255 0 255" --fields "DATA uint8"
visus import datasets/cat/gray.png export tmp/cat/visus.idx --box "0 255 0 255" 
python -m OpenVisus compress-dataset --compression zip tmp/cat/visus.idx
```

[OK] C++/others (Dataset::compressDataset), e.g. lz4

```
visus create tmp/cat/visus.idx --box "0 255 0 255" --fields "DATA uint8"
visus import datasets/cat/gray.png export tmp/cat/visus.idx --box "0 255 0 255" 
python -m OpenVisus compress-dataset --compression lz4  tmp/cat/visus.idx
rd /S tmp/cat
```

## Arco>-0

Python/zip (CompressArcoDataset/CompressArcoBlock )

```
visus create tmp/cat/visus.idx --box "0 255 0 255" --fields "DATA uint8" --arco 4096
visus import datasets/cat/gray.png export tmp/cat/visus.idx --box "0 255 0 255" 
python -m OpenVisus compress-dataset --compression zip tmp/cat/visus.idx
rd /S tmp/cat
```

C++ otherwise (LoadBinaryDocument, encoder)

```
visus create tmp/cat/visus.idx --box "0 255 0 255" --fields "DATA uint8" --arco 4096
visus import datasets/cat/gray.png export tmp/cat/visus.idx --box "0 255 0 255" 
python -m OpenVisus compress-dataset --compression lz4 tmp/cat/visus.idx
rd /S tmp/cat
```

# [OK] Samples/*

See "test-full" inside `__main__.py`:

```
python Samples/python/array.py
python Samples/python/dataflow/dataflow1.py
python Samples/python/dataflow/dataflow2.py
python Samples/python/idx/read.py
python Samples/python/idx/convert.py 
python Samples/python/wavelets/filters.py

# TODO later (slow)
# python Samples/python/idx/speed.py

python -m OpenVisus server --dataset ./datasets/cat/rgb.idx --port 10000 --exit
```

#  Test docs/convert cases

```
import OpenVisus as ov,numpy as np,shutil
from PIL import Image
from urllib.request import urlopen
```

# db.write

[OK] Arco==0

```
db=ov.CreateIdx(url="tmp/visus.idx", dim=2, fields=[ov.Field('data','uint8[3]','row_major')], dims=(256,256),arco=0)
db.write(np.asarray(Image.open('datasets/cat/gray.png')))
```

[OK]Arco>0:

```
db=ov.CreateIdx(url="tmp/visus.idx", dim=2, fields=[ov.Field('data','uint8[3]','row_major')], dims=(256,256),arco="4kb")
db.write(np.asarray(Image.open('datasets/cat/gray.png')))
```

# [OK] writeSlabs

[OK] Arco==0

```

def generateSlices(): 
	data=np.asarray(Image.open('datasets/cat/gray.png'))
	for I in range(16): 
		yield data
db=ov.CreateIdx(url="tmp/visus.idx", dim=3, fields=[ov.Field('data','uint8[3]','row_major')], dims=(256,256,16),arco=0)
db.writeSlabs(generateSlices())
```

[OK] Arco>0

```
def generateSlices(): 
	data=np.asarray(Image.open('datasets/cat/gray.png'))
	for I in range(16): 
		yield data
db=ov.CreateIdx(url="tmp/visus.idx", dim=3, fields=[ov.Field('data','uint8[3]','row_major')], dims=(256,256,16),arco="4kb")
db.writeSlabs(generateSlices())
```

## Convert existing OpenVisus local dataset to ARCO format

[OK] Arco==0:

```
python -m OpenVisus copy-dataset --arco "0" datasets/cat/gray.idx tmp/cat/non-arco/gray.idx 
```

[OK] Arco>0:

```
python -m OpenVisus copy-dataset --arco "4kb" datasets/cat/gray.idx tmp/cat/arco/gray.idx 
```
