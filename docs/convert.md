# OpenVisus convert

![Diagram](https://raw.githubusercontent.com/sci-visus/OpenVisus/master/docs/openvisus-convert.png)

Notes:
- to enable IDX caching do `?cached=idx&caching_compression=zip`
-    slower in writing (since it will create write locks for concurrent threads) but faster for reading (less `fopen`/`fclose` ops)
- to enable one-block-per-file caching do `?cached=arco&caching_compression=zip`
-    faster to write (no need of file locks) but slower for reading (more `fopen`/`fclose` ops)


## Convert image stack to OpenVisus

Instructions:
- for the `ARCO` arguments
   - use `modvisus` if you want to use the standard OpenVisus format, suitable to be run on the OpenVisus server 
   - use a string size (e.g. `512kb`, `1mb`, `2mb` etc.) to use the new ARCO format and run potentially  on cloud storage (i.e. serverless)
- `SRC` argument is a Python `glob.glob` expression (https://docs.python.org/3/library/glob.html) to find images 
- `DST` argument is the IDX destination filename. 
     - It's reccomended to dedicate one directory exclusive for the dataset (i.e. don't mix files with other datasets)

```bash
ARCO=4mb
SRC=/path/your/image/stack/**/*.png
DST=/path/to/your/openvisus/dataset/visus.idx 

# writes data uncompressed
python3 -m OpenVisus convert-image-stack --arco "${ARCO}" "${SRC}" "${DST}"

# final pass to compress and reduce size
python3 -m OpenVisus compress-dataset --compression zip ${DST} 
```


## Extract a slice from OpenVisus 

Instructions:
- `BOX` argument is in the format `x1,x2,y1,y2,z1,z2` (extrema included)
- bounds must be known in advance so change as needed

```bash
BOX="0 2047 0 2047 1024 1024"
python -m OpenVisus convert import /path/to/your/visus.idx  --box "${BOX}" export example.png
```

## Extracting slices from OpenVisus

```bash
for ((z=0; z<=2047; z++)); do
  python3 -m OpenVisus convert import /path/to/visus.idx --box "0 2047 0 2047 $z $z" export /path/to/image/stack/$(printf "%04d" $z).png
done
```


## Convert existing OpenVisus local dataset to ARCO format

Instructions:
- it will not copy blocks by blocks, so it may be slow. But it allows to change the blocksize (which is important for ARCO):

```bash
ARCO=2mb
SRC=/mnt/c/data/visus-dataset/2kbit1/modvisus/visus.idx
DST=/mnt/c/data/visus-dataset/2kbit1/1mb/visus.idx 

# writes the dataset uncompressed
python3 -m OpenVisus copy-dataset --arco ${ARCO} ${SRC} ${DST}
```
**Mandatory** compression step. This step is required because the reader currently assumes blocks are compressed.

```bash
# final pass to compress and reduce size.
# add [--timestep <int>] to select a certain timestep
# add [--field <string>] to select a certain field
python3 -m OpenVisus compress-dataset --compression zip ${DST} 
```

## Upload OpenVisus dataset to the cloud using aws s3

Instructions
- see https://github.com/aws/aws-cli for full details
- `--endpoint-url` argument could be extracted from the `~/.aws/credentials` file only if you install this https://github.com/wbingli/awscli-plugin-endpoint, otherwise you need to add `--endpoint-url` argument
- both `SRC` and `DST` arguments must refer to a directory (no '/' at the end)

```bash
SRC=/path/to/your/idx/dataset/directory
DST=s3://your-bucket-name/whatever/destination/directory
aws s3 [--debug] --profile <profile> sync ${SRC} ${DST}
```

## Upload OpenVisus ARCO dataset to the cloud using s5cmd

Instructions
- see https://github.com/peak/s5cmd for full details
- be careful to the right syntax (i.e. the `/` at the end of arguments)
- endpoint could be specified using env variables or by `--endpoint-url` (i.e. `s5cmd` does not seem to support any plugin)

```bash
SRC=""/path/to/your/idx/dataset/directory/*"
DST="s3://your-bucket-name/whatever/destination/directory/"
AWS_PROFILE=<profile> S3_ENDPOINT_URL=<your-endpoint> s5cmd cp --if-size-differ ${SRC} {DST}
```

## Conversion example

Concert image stack to OpenVisus:

```bash
python3 -m OpenVisus convert-image-stack --arco "4mb" "/path/your/image/stack/**/*.png" "my-new-dataset/visus.idx"
```

Compress OpenVisus dataset:

```
python3 -m OpenVisus compress-dataset --compression zip "my-new-dataset/visus.idx" 
```


Update to the cloud:

```
ENDPOINT_URL=https://s3.us-west-1.wasabisys.com
ACCESS_KEY=XXXXX
SECRET_KEY=YYYYY
BUCKET=your-bucket-name
AWS_PROFILE=wasabi s5cmd --endpoint-url ${ENDPOINT_URL} cp --if-size-differ "my-new-dataset/*" "s3://${BUCKET}/my-new-dataset/"
```

Add the OpenVisus dataset to your `visus.config`

```
cat <<EOF > datasets.config
<dataset name="my-new-dataset" url='${ENDPOINT_URL}/${BUCKET}/my-new-dataset/visus.idx?access_key=${ACCESS_KEY}&amp;secret_key=${SECRET_KEY}' />
EOF
```

# Cloud Access

You can use different OpenVisus Access classes to enable caching/sharding of block access.
As an example the following code snipped shows an access with:

- RamAcccess up to a certain size
- DiskAccess to cache blocks on client side
- Sharded cloud requests to several cloud storages (in the example the first 16 blocks are stored on wasabi, the following 16 blocks are stored on AWS etc)

```
<dataset name='complex' url='https://s3.us-west-1.wasabisys.com/nsdf/visus-datasets/2kbit1/1mb/visus.idx?access_key=XXX&amp;secret_key=YYY' >
	<access type='multiplex'>
		<access name="ram"          type='RamAccess'  chmod='rw' available='2GB' />
		<access name="cache"        type='DiskAccess' chmod='rw' compression="zip" layout="row_major" filename_template="$(VisusCache)/2kbit1/1mb/visus/$(time)/$(field)/$(block:%016x:%04x).bin" />
		<access name="wasabi"       type="cloud"      chmod='r'  shard="0/7 16" url='https://s3.us-west-1.wasabisys.com/nsdf/visus-datasets/2kbit1/1mb/visus.idx?access_key=XXX&amp;secret_key=YYY' />
		<access name='sealstorage'  type="cloud"      chmod='r'  shard="2/7 16" url='https://maritime.sealstorage.io/api/v0/s3/utah/nsdf/visus-datasets/2kbit1/1mb/visus.idx?access_key=XXX&amp;secret_key=YYY&amp;endpoint_url=https://maritime.sealstorage.io/api/v0/s3' /> 
		<access name='chpc'         type="cloud"      chmod='r'  shard="3/7 16" url='https://pando-rgw01.chpc.utah.edu/nsdf/visus-datasets/2kbit1/1mb/visus.idx?access_key=XXX&amp;secret_key=YYY' /> 
		<access name='mghp'         type="cloud"      chmod='r'  shard="4/7 16" url='https://mghp.osn.xsede.org/vpascuccibucket1/nsdf/visus-datasets/2kbit1/1mb/visus.idx?access_key=XXX&amp;secret_key=YYY' />
		<access name='ucsd'         type="cloud"      chmod='r'  shard="5/7 16" url='https://nsdf.s3.sdsc.edu/nsdf/visus-datasets/2kbit1/1mb/visus.idx?access_key=XXXX&amp;secret_key=YYY' /> 
		<access name='cloudflare'   type="cloud"      chmod='r'  shard="6/7 16" url='https://account_id.r2.cloudflarestorage.com/nsdf/visus-datasets/2kbit1/1mb/visus.idx?access_key=XXX&amp;secret_key=YYY' />
		</access>
</dataset> 
```

OpenVisus has been successfully tested with:

| Cloud Name             | Url reference                               | Endpoint (example)                           |
|------------------------|---------------------------------------------|----------------------------------------------|
| Amazon Web Services S3 | https://aws.amazon.com/pm/serv-s3           | https://s3.us-west-1.wasabisys.com           |
| Wasabi                 | https://wasabi.com/                         | https://s3.us-west-1.wasabisys.com           |
| Seal Storage           | https://www.sealstorage.io/                 | https://maritime.sealstorage.io/api/v0/s3    |
| Ceph storage system    | https://docs.ceph.com/en/latest/radosgw/s3/ | https://pando-rgw01.chpc.utah.edu            |
| Open Storage Network   | https://www.openstoragenetwork.org/         | https://mghp.osn.xsede.org/bucket-name       |
| MinIo Storage          | https://min.io/                             | https://nsdf.s3.sdsc.edu                     |
| CloudFlare R2          | https://www.cloudflare.com/pg-lp/r2         | https://account_id.r2.cloudflarestorage.com` |

![Diagram](https://raw.githubusercontent.com/sci-visus/OpenVisus/master/docs/openvisus-convert-access.png)


## Cost

Be careful to fees, and in particular EGRESS fees.
The following diagram (ref https://www.qualeed.com/en/qbackup/cloud-storage-comparison/) compares different cloud solutions.

![Diagram](https://www.qualeed.com/images/cloud-storages/pricing-20220922.png)

