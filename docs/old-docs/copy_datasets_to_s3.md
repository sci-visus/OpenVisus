---
layout: default
parent: Old Docs
nav_order: 2
---

## Table of contents
{: .no_toc .text-delta }

1. TOC
{:toc}

---

# Prerequisites. Create an Access Key


## Amazon S3


To create an access key go to https://console.aws.amazon.com/iam/home?#/users$new?step=details:

```
access_key: python-s3
Type: Programmatic access
Attach existing policies: AmazonS3FullAccess
Run aws configure (change as needed):
    AWS Access Key ID [None]: xxxxxxxxx
    AWS Secret Access Key [None]: yyyyyy
    Default region name [None]: us-east-1
    Default output format [None]:
```

## Wasabi

See https://wasabi-support.zendesk.com/hc/en-us/articles/360019677192-Creating-a-Wasabi-API-Access-Key-Set for creating an access key.

**Open problem** sometimes we get STATUS_FORBIDDEN  on wasabi:

```
# 103400580 NetService:623 28088:22172 GET connection 3 wait 10 running 526 status STATUS_FORBIDDEN url https://s3.us-west-1.wasabisys.com/visus-server-foam/2/data/0000000000006980
```

## OSN Pod

Go to `portal.osn.xsede.org` and use the `University of Utah` institution login to access the bucket and/or create an access key.


# Copy OpenVisus datasets

Specify the **source** dataset. The source can be a local dataset (`local->S3` copy) or a remote one (`S3->S3` copy). For example in case of Wasabi:


```
export SRC=https://s3.us-west-1.wasabisys.com/visus-server-foam/visus.idx?layout=hzorder
```

Specify the **destination**. For example, in case of OSN Pods:

```
export ACCESS_KEY=XXXXXX
export SECRET_ACCESS_KEY=YYYYYYYY
export DST=https://mghp.osn.xsede.org/vpascuccibucket1/visus-server-foam
```

Run the copy (see https://github.com/sci-visus/OpenVisus/blob/master/Samples/python/cloud_storage/s3.py):

```
python Samples/python/cloud_storage/s3.py copy-blocks \
    --src $SRC \
    --dst $DST \
    --num-threads 8
```

At the end of the process the copy will print out the instruction regardning how to reference the new dataset in your `visus.config`.  You will get a new config that automatically enables caching:

```
<dataset url='...' >
	<access type='multiplex'>
		<access type='disk' chmod='rw' url='file://D:/visus-cache/...' />
		<access type="CloudStorageAccess" url="..." chmod="r" compression="zip" />
	</access>
</dataset>"
```


# Use RClone for S3->S3 copy

If you have the right configurations in `rclone` you can do the transfer S3->S3 in the background:

```
# optional delete 
rclone -vv delete osn:vpascuccibucket1/visus-server-foam
rclone -vv --progress sync WasabiDrone:visus-server-foam osn:vpascuccibucket1/visus-server-foam
```




