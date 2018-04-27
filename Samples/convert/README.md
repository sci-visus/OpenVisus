# Convert data to ViSUS IDX

```
#!/bin/bash

# path to visus executable
CONVERT=build/RelWithDebInfo/visus.exe

# path to your brand new IDX file
IDXFILE="G:/visus_dataset/2kbit1/visus.idx"

# box is specified as [x1 x2] [y1 y2] [z1 z2], min-max included (in this case the dataset has size 2048^3)
BOX="0 2047 0 2047 0 2047"

# change your data DTYPE (in this case a single Uint8)
DTYPE="uint8[1]"

# first you need to write the data uncompressed (i.e. raw), later you will compress it
# if you don't do so, you will end up with an overall file size of 1.5/2.0x times uncompressed size!
COMPRESSION="default_compression(raw)"

$CONVERT --create "$IDXFILE" --box "$BOX" --fields "DATA $DTYPE $COMPRESSION"

# example of conversion of png slices to IDX
for sliceid in $(seq -f "%04g" 0 2047)
do
  $CONVERT \
     --import "G:/visus_dataset/2kbit1/png/$sliceid.png" \
     --export "$IDXFILE" --box "0 2047 0 2047 $sliceid $sliceid" 
done

# finally compress your dataset (you can use zip or lz4 here)
$CONVERT --compress-dataset $IDXFILE lz4

```

