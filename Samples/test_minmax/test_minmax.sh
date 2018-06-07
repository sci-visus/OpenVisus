#!/bin/bash

rm -Rf ./temp/test_minmax*

VISUS="build/vs12/Debug/visus.exe"
VIEWER="build/vs12/Debug/visusviewer.exe"

BOX="0 63 0 63"
DIMS="64 64"
MAXH=12

for I in 0 1 2
do

  num=$(printf "%02d" $I)

  SRC=docs/examples/visus/test_minmax/test_minmax.$num.png 
  
  DIR=./temp/test_minmax/$num
  mkdir -p $DIR

  $VISUS create $DIR/original.idx --box "$BOX" --fields "data uint8[1]"
  $VISUS create $DIR/filtered.idx --box "$BOX" --fields "data uint8[2] filter(min)"
  
  $VISUS import $SRC               export $DIR/original.idx --box "$BOX"
  $VISUS import $SRC cast uint8[2] export $DIR/filtered.idx --box "$BOX"
  
  cp $SRC $DIR/original.png
  
  $VISUS apply-filters $DIR/filtered.idx
  for J in $(seq 0 $MAXH)
  do
     $VISUS import $DIR/filtered.idx --box "$BOX" --fromh 0 --toh $J cast uint8[1] export $DIR/filtered.$J.png
  done
  
  # I want to see the image as it is (see --disable-filters)
  $VISUS import $DIR/filtered.idx --box "$BOX" --fromh 0 --toh $MAXH --disable-filters cast uint8[1] export $DIR/filtered.disablefilters.png 
    

  #if diff $DIR/original.png  $DIR/filtered.$MAXH.png >/dev/null ; then
  #  echo OK
  #else
  #  echo ERROR happened images not equal
  #  echo diff $DIR/original.png  $DIR/filtered.$MAXH.png
  #  exit 1
  #fi  

done

