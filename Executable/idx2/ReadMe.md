# Instructions

Prerequisites: ask for credentials.


Edit your `$HOME/.aws/config` and set wasabi profile:

```
[profile wasabi]
region = us-west-1
s3 =
    endpoint_url = https://s3.us-west-1.wasabisys.com
```


Edit your `$HOME/.aws/credentials` and set credentials

```
[wasabi]
aws_access_key_id = XXXXX
aws_secret_access_key = YYYYY
```

Run `CMAKE` and set (note: most visus options are disabled to keep it simple):
- VISUS_DATAFLOW=0
- VISUS_GUI=0
- VISUS_HDF5=0
- VISUS_IMAGE=0
- VISUS_MODVISUS=0
- VISUS_OSPRAY=0
- VISUS_PYTHON=0
- VISUS_SLAM=0
- VISUS_WAVING=0
- VISUS_IDX2=0 #  NOTE: this refers to the OLD hyphotesis of having a full IDX2 library working inside OpenVisus viewer
- VISUS_NET=1  # we need libcurl for network requests

Then run the `visus_idx2` binary.
Change main.cpp as needed.

TODO:
- add async example
- add DiskAccess      (need to work without a dataset)
- add MemoryAccess    (need to work without a dataset)
- add MultiplexAccess (need to work without a dataset)