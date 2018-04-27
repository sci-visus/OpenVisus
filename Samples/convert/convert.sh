#!/bin/bash

rm -Rf  temp/*

CONVERT=$1
RESOURCES=$2

if [ ! $CONVERT ]; then
	CONVERT="build/Debug/visus.exe"
fi

if [ ! $RESOURCES ]; then
	
	RESOURCES="../../Misc"
	
	if [ ! -d $RESOURCES ] ; then
	  RESOURCES="./Misc"
	fi
	
fi

# ///////////////////////////////////////////
# For very simple conversions, all you need is create
# ///////////////////////////////////////////
$CONVERT import $RESOURCES/cat_gray.tga create temp/cat_gray.idx 
$CONVERT import $RESOURCES/cat_rgb.tga  create temp/cat_rgb.idx 


# ///////////////////////////////////////////
# To get information about any file (including non-idx), use info
# ///////////////////////////////////////////
$CONVERT info $RESOURCES/cat_gray.tga
$CONVERT info $RESOURCES/cat_rgb.tga
$CONVERT info temp/cat_gray.idx
$CONVERT info temp/cat_rgb.idx


# ///////////////////////////////////////////
# For more control, explicitly create the idx volume
# ///////////////////////////////////////////
fields="scalar uint8 compressed + vector uint8[3] compressed"

$CONVERT create temp/test_formats.idx --box "0 511 0 511" --fields "$fields" --bitsperblock 12

# ///////////////////////////////////////////
# example of tiling the same image multiple times
# ///////////////////////////////////////////

$CONVERT import $RESOURCES/cat_gray.tga export temp/test_formats.idx --field scalar --box "  0 255     0 255"
$CONVERT import $RESOURCES/cat_gray.tga export temp/test_formats.idx --field scalar --box "256 511     0 255"
$CONVERT import $RESOURCES/cat_gray.tga export temp/test_formats.idx --field scalar --box "  0 255   256 511"
$CONVERT import $RESOURCES/cat_gray.tga export temp/test_formats.idx --field scalar --box "256 511   256 511"

$CONVERT import $RESOURCES/cat_rgb.tga export temp/test_formats.idx --field vector --box "  0 255     0 255"
$CONVERT import $RESOURCES/cat_rgb.tga export temp/test_formats.idx --field vector --box "256 511     0 255"
$CONVERT import $RESOURCES/cat_rgb.tga export temp/test_formats.idx --field vector --box "  0 255   256 511"
$CONVERT import $RESOURCES/cat_rgb.tga export temp/test_formats.idx --field vector --box "256 511   256 511"


$CONVERT import temp/test_formats.idx --field scalar --box "  0 255     0 255"  export temp/test_formats_scalar.tga 
$CONVERT import temp/test_formats.idx --field scalar --box "256 511     0 255"  export temp/test_formats_scalar.jpg  
$CONVERT import temp/test_formats.idx --field scalar --box "  0 255   256 511"  export temp/test_formats_scalar.tif  
$CONVERT import temp/test_formats.idx --field scalar --box "256 511   256 511"  export temp/test_formats_scalar.bmp 
$CONVERT import temp/test_formats.idx --field scalar --box "128 383   128 383"  export temp/test_formats_scalar.png 


$CONVERT import temp/test_formats.idx --field vector --box "  0 255     0 255"  export temp/test_formats_vector.tga 
$CONVERT import temp/test_formats.idx --field vector --box "256 511     0 255"  export temp/test_formats_vector.jpg  
$CONVERT import temp/test_formats.idx --field vector --box "  0 255   256 511"  export temp/test_formats_vector.tif  
$CONVERT import temp/test_formats.idx --field vector --box "256 511   256 511"  export temp/test_formats_vector.bmp 
$CONVERT import temp/test_formats.idx --field vector --box "128 383   128 383"  export temp/test_formats_vector.png 

# ///////////////////////////////////////////
# example of inplace pasting an image
# ///////////////////////////////////////////

$CONVERT create temp/test_paste.idx --box "0 255 0 1023" --fields "DATA uint8[3] default_compression(zip)" --bitsperblock 12

# show how several files can be "merged" together in import

$CONVERT \
	import /dev/null --dtype 3*uint8 --dims "256 1024" \
	paste $RESOURCES/cat_rgb.tga --destination-box "0 255 0   255"  \
	paste $RESOURCES/cat_rgb.tga --destination-box "0 255 256 511"  \
	paste $RESOURCES/cat_rgb.tga --destination-box "0 255 512 767"  \
	paste $RESOURCES/cat_rgb.tga --destination-box "0 255 768 1023" \
	export temp/test_paste.idx --box "0 255 0 1023"
	
$CONVERT \
  import temp/test_paste.idx --box "0 255 0 1023" \
  export temp/test_paste.jpg

# ///////////////////////////////////////////
# example of several timesteps
# ///////////////////////////////////////////

$CONVERT create temp/test_timesteps.idx --box "0 255 0 255" --fields "scalar uint8 default_compression(zip)" --time 0 3 time%02d/

$CONVERT import $RESOURCES/cat_gray.tga export temp/test_timesteps.idx --time 0 --box "0 255 0 255"
$CONVERT import $RESOURCES/cat_gray.tga export temp/test_timesteps.idx --time 1 --box "0 255 0 255"
$CONVERT import $RESOURCES/cat_gray.tga export temp/test_timesteps.idx --time 2 --box "0 255 0 255"
$CONVERT import $RESOURCES/cat_gray.tga export temp/test_timesteps.idx --time 3 --box "0 255 0 255"

$CONVERT import temp/test_timesteps.idx --field scalar --box "0 255 0 255"  --time 0 export temp/test_timesteps_0.tga 
$CONVERT import temp/test_timesteps.idx --field scalar --box "0 255 0 255"  --time 1 export temp/test_timesteps_1.jpg  
$CONVERT import temp/test_timesteps.idx --field scalar --box "0 255 0 255"  --time 2 export temp/test_timesteps_2.tif  
$CONVERT import temp/test_timesteps.idx --field scalar --box "0 255 0 255"  --time 3 export temp/test_timesteps_3.bmp 

# ///////////////////////////////////////////
# example of getting input from network (works using idx too!)
# ///////////////////////////////////////////

$CONVERT \
  import "http://www.dia.uniroma3.it/~scorzell/phd/docs/html/fig/tower.all.gif" \
  export temp/test_network_0.png
  
$CONVERT \
  import "http://atlantis.sci.utah.edu/mod_visus?dataset=atlanta&compression=jpg" --box "16000 17023 14000 15034" \
  export temp/test_network_1.png



