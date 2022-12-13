# Instructions

To run locally without docker:

- if you want multiple views add `--multiple` to each command line

```

# set to 1 for debugging
export VISUS_NETSERVICE_VERBOSE=0
export VISUS_VERBOSE_DISKACCESS=0

cd Samples/dashboards

# 3d dataset on cloud storage
python3 -m bokeh serve run.py --dev --args \
    "https://s3.us-west-1.wasabisys.com/nsdf/visus-datasets/2kbit1/1mb/visus.idx?profile=wasabi&cached=1" \
    --palette-range "0 255.0"

# 3d dataset on on modvisus
python3 -m bokeh serve run.py  --dev --args \
    "http://atlantis.sci.utah.edu/mod_visus?dataset=2kbit1&cached=1" \
   --palette-range "0.0 255.0"

# 2d dataset on movisus 
python3 -m bokeh serve run.py  --dev --args \
    "http://atlantis.sci.utah.edu/mod_visus?dataset=david_subsampled&cached=1" \
    --palette-range "0.0 255.0"

# NOTE: for this you need the WASABI credentials in your ~/.aws/config|credentials
python3 -m bokeh serve run.py  --dev --args \
    "https://s3.us-west-1.wasabisys.com/Pania_2021Q3_in_situ_data/workflow/fly_scan_id_112509.h5/r/idx/1mb/visus.idx?profile=wasabi&cached=idx&blob_extension=.bin.zz" \
    --palette-range "0 65535.0"
```
