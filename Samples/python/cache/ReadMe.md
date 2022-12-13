
# Cache speed test

To run, just provide a list of remote datasets (can be cloud or modvisus):
- NOTE: the cache will be created in ~/visus/cache
- it will simulate 3 pure-python Query, each one running one direction (i.e. x,y,z)
- it simulates concurrent access/caching to the same dataset

Outcomes:
- it seems always better to use cached=idx with moderate concurrency (i.e. 3 tasks). The effect of file-locks does not seem to slow down things too much
- for a **server-side caching**, with lots of user, I am not sure which one will perform better. Could be file locks a problem?
- in *read mode* idx always wins, this is because it reduces consistently fopen/fclose

# WSL2 Results - cached=idx

``` 
# you can choose idx for IDX1 file format, or disk for simil-cloud one-block/one-file disk access
CACHED=idx
python3 Samples/python/cache/speed_test.py \
   "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112509.h5/r/idx/1mb/visus.idx?profile=wasabi&cached=${CACHED}&blob_extension=.bin.zz" \
   "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112512.h5/r/idx/1mb/visus.idx?profile=wasabi&cached=${CACHED}&blob_extension=.bin.zz" \
   "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112515.h5/r/idx/1mb/visus.idx?profile=wasabi&cached=${CACHED}&blob_extension=.bin.zz" \
   "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112517.h5/r/idx/1mb/visus.idx?profile=wasabi&cached=${CACHED}&blob_extension=.bin.zz" \
   "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112520.h5/r/idx/1mb/visus.idx?profile=wasabi&cached=${CACHED}&blob_extension=.bin.zz" \
   "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112522.h5/r/idx/1mb/visus.idx?profile=wasabi&cached=${CACHED}&blob_extension=.bin.zz" \
   "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112524.h5/r/idx/1mb/visus.idx?profile=wasabi&cached=${CACHED}&blob_extension=.bin.zz" \
   "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112526.h5/r/idx/1mb/visus.idx?profile=wasabi&cached=${CACHED}&blob_extension=.bin.zz" \
   "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112528.h5/r/idx/1mb/visus.idx?profile=wasabi&cached=${CACHED}&blob_extension=.bin.zz" \
   "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112530.h5/r/idx/1mb/visus.idx?profile=wasabi&cached=${CACHED}&blob_extension=.bin.zz" \
   "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112532.h5/r/idx/1mb/visus.idx?profile=wasabi&cached=${CACHED}&blob_extension=.bin.zz" 

# RESULTS
# 1st-execution
#    Statistics enlapsed=720 seconds
#       IO  r=34.86GiB r_sec=49.81MiB w=52.81GiB w_sec=75.46MiB/sec n=116,173 n_sec=162/sec
#       NET r=24.44GiB r_sec=34.91MiB w=0 w_sec=0.0/sec n=54,009 n_sec=75/sec
# 
# 2nd-execution
#    Statistics enlapsed=26 seconds
#       IO  r=87.55GiB r_sec=3.41GiB/sec w=0 w_sec=0.0/sec n=12,267 n_sec=477/sec
#       NET r=       0 r_sec=    0.0/sec w=0 w_sec=0.0/sec n=0 n_sec=0/sec
```


# WIN32 Results - cached=idx

``` 
SET PYTHONPATH=build\RelWithDebInfo
SET VISUS_NETSERVICE_VERBOSE=0
python Samples/python/cache/speed_test.py ^
   "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112509.h5/r/idx/1mb/visus.idx?profile=wasabi&cached=idx&blob_extension=.bin.zz" ^
   "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112512.h5/r/idx/1mb/visus.idx?profile=wasabi&cached=idx&blob_extension=.bin.zz" ^
   "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112515.h5/r/idx/1mb/visus.idx?profile=wasabi&cached=idx&blob_extension=.bin.zz" ^
   "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112517.h5/r/idx/1mb/visus.idx?profile=wasabi&cached=idx&blob_extension=.bin.zz" ^
   "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112520.h5/r/idx/1mb/visus.idx?profile=wasabi&cached=idx&blob_extension=.bin.zz" ^
   "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112522.h5/r/idx/1mb/visus.idx?profile=wasabi&cached=idx&blob_extension=.bin.zz" ^
   "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112524.h5/r/idx/1mb/visus.idx?profile=wasabi&cached=idx&blob_extension=.bin.zz" ^
   "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112526.h5/r/idx/1mb/visus.idx?profile=wasabi&cached=idx&blob_extension=.bin.zz" ^
   "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112528.h5/r/idx/1mb/visus.idx?profile=wasabi&cached=idx&blob_extension=.bin.zz" ^
   "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112530.h5/r/idx/1mb/visus.idx?profile=wasabi&cached=idx&blob_extension=.bin.zz" ^
   "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112532.h5/r/idx/1mb/visus.idx?profile=wasabi&cached=idx&blob_extension=.bin.zz" 

# 1st-execution
#   Statistics enlapsed=920 seconds
#      IO  r=34.92GiB r_sec=38.91MiB w=52.75GiB w_sec=58.78MiB/sec n=112,872 n_sec=122/sec
#      NET r=24.40GiB r_sec=27.20MiB w=0 w_sec=0.0/sec n=53,947 n_sec=58/sec
# 
# 2nd-execution
#    Statistics enlapsed=3.4e+01
#      IO  r=87.55GiB r_sec=2.58GiB w=0 w_sec=0.0/sec n=12,760 n_sec=376/sec
#      NET r=0 r_sec=0.0 w=0 w_sec=0.0/sec n=0 n_sec=0/sec
```


# WSL2 Results - cached=disk

```
# you can choose idx for IDX1 file format, or disk for simil-cloud one-block/one-file disk access
CACHED=disk
python3 Samples/python/cache/speed_test.py \
   "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112509.h5/r/idx/1mb/visus.idx?profile=wasabi&cached=${CACHED}&blob_extension=.bin.zz" \
   "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112512.h5/r/idx/1mb/visus.idx?profile=wasabi&cached=${CACHED}&blob_extension=.bin.zz" \
   "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112515.h5/r/idx/1mb/visus.idx?profile=wasabi&cached=${CACHED}&blob_extension=.bin.zz" \
   "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112517.h5/r/idx/1mb/visus.idx?profile=wasabi&cached=${CACHED}&blob_extension=.bin.zz" \
   "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112520.h5/r/idx/1mb/visus.idx?profile=wasabi&cached=${CACHED}&blob_extension=.bin.zz" \
   "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112522.h5/r/idx/1mb/visus.idx?profile=wasabi&cached=${CACHED}&blob_extension=.bin.zz" \
   "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112524.h5/r/idx/1mb/visus.idx?profile=wasabi&cached=${CACHED}&blob_extension=.bin.zz" \
   "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112526.h5/r/idx/1mb/visus.idx?profile=wasabi&cached=${CACHED}&blob_extension=.bin.zz" \
   "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112528.h5/r/idx/1mb/visus.idx?profile=wasabi&cached=${CACHED}&blob_extension=.bin.zz" \
   "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112530.h5/r/idx/1mb/visus.idx?profile=wasabi&cached=${CACHED}&blob_extension=.bin.zz" \
   "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112532.h5/r/idx/1mb/visus.idx?profile=wasabi&cached=${CACHED}&blob_extension=.bin.zz" 

# 1st-execution
#    Statistics enlapsed=840 seconds
#       IO  r=14.51GiB r_sec=17.72MiB/sec w=24.51GiB w_sec=29.93MiB/sec n=89,640 n_sec=106/sec
#       NET r=24.52GiB r_sec=29.94MiB/sec w=0 w_sec=0.0/sec n=54,171 n_sec=64/sec  
# 
# 2nd-execution
#    Statistics enlapsed=110 seconds
#       IO  r=39.03GiB r_sec=377.25MiB/sec w=0 w_sec=0.0/sec n=89,631 n_sec=846/sec
#       NET r=       0 r_sec=      0.0/sec w=0 w_sec=0.0/sec n=0 n_sec=0/s
```


# WIN32 results - cached=disk

```
SET PYTHONPATH=build\RelWithDebInfo
SET VISUS_NETSERVICE_VERBOSE=0
python Samples/python/cache/speed_test.py ^
   "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112509.h5/r/idx/1mb/visus.idx?profile=wasabi&cached=disk&blob_extension=.bin.zz" ^
   "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112512.h5/r/idx/1mb/visus.idx?profile=wasabi&cached=disk&blob_extension=.bin.zz" ^
   "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112515.h5/r/idx/1mb/visus.idx?profile=wasabi&cached=disk&blob_extension=.bin.zz" ^
   "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112517.h5/r/idx/1mb/visus.idx?profile=wasabi&cached=disk&blob_extension=.bin.zz" ^
   "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112520.h5/r/idx/1mb/visus.idx?profile=wasabi&cached=disk&blob_extension=.bin.zz" ^
   "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112522.h5/r/idx/1mb/visus.idx?profile=wasabi&cached=disk&blob_extension=.bin.zz" ^
   "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112524.h5/r/idx/1mb/visus.idx?profile=wasabi&cached=disk&blob_extension=.bin.zz" ^
   "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112526.h5/r/idx/1mb/visus.idx?profile=wasabi&cached=disk&blob_extension=.bin.zz" ^
   "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112528.h5/r/idx/1mb/visus.idx?profile=wasabi&cached=disk&blob_extension=.bin.zz" ^
   "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112530.h5/r/idx/1mb/visus.idx?profile=wasabi&cached=disk&blob_extension=.bin.zz" ^
   "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112532.h5/r/idx/1mb/visus.idx?profile=wasabi&cached=disk&blob_extension=.bin.zz" 

# 1st-execution
#    Statistics enlapsed=1100 seconds
#       IO  r=14.51GiB r_sec=13.57MiB/sec w=24.51GiB w_sec=22.91MiB/sec n=89,618 n_sec=81/sec
#       NET r=24.53GiB r_sec=22.93MiB/sec w=0 w_sec=0.0/sec n=54,102 n_sec=49/sec
# 
# 2nd-execution
#    Statistics enlapsed=190
#       IO  r=39.03GiB r_sec=211.96MiB w=0 w_sec=0.0/sec n=89,631 n_sec=475/sec
#       NET r=0 r_sec=0.0 w=0 w_sec=0.0/sec n=0 n_sec=0/sec
```

