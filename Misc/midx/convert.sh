#!/bin/bash

# please execute from root source directory
DIR="./docs/examples/midx"

rm -Rf \
  $DIR/visus.midx \
  $DIR/temp \
  $DIR/A \
  $DIR/B \
  $DIR/C \
  $DIR/D \

CONVERT=$1

if [ ! $CONVERT ]; then
	CONVERT="build/vs12/Debug/visus.exe"
fi

midx=$DIR/visus.midx
echo "<dataset typename='IdxMultipleDataset'>" >> $midx

for db in A B C D 
do
  
  # uncompressed
  idx_uncompressed=$DIR/temp/visus.idx
  
  $CONVERT create $idx_uncompressed --box "0 2047 0 1023" --fields "temperature uint8[3] + pressure uint8[3] " --time 0 9 "time%02d/"
  for time in 0 1 2 3 4 5 6 7 8 9
  do
    for field in temperature pressure
    do
      formatted_time=$(printf "%02d" $time)
      $CONVERT import $DIR/tif/$field/${db}_time_${formatted_time}.tif export $idx_uncompressed --field $field --time $time --box "0 2047 0 1023"
    done 
  done
  
  # compress
  idx=$DIR/$db/visus.idx
  $CONVERT create $idx --box "0 2047 0 1023" --fields "temperature uint8[3] default_compression(zip) + pressure uint8[3] default_compression(zip)" --time 0 9 "time%02d/"
  $CONVERT copy-dataset $idx_uncompressed  $idx
  
  rm -Rf $DIR/temp
  
  echo "<dataset url='file://\$(CurrentFileDirectory)/$db/visus.idx' name= '$db'  />" >> $midx
  
done

echo "</dataset>" >> $midx
