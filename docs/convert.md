# OpenVisus convert

![Diagram](https://raw.githubusercontent.com/sci-visus/OpenVisus/master/docs/openvisus-convert.png)




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

# final pass to compress and reduce size
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

