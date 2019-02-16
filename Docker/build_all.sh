#!/bin/bash

#
# Build the OpenVisus docker images.
#

# These three images use pip to install OpenVisus:
declare -a arr=("mod_visus-ubuntu" "mod_visus-opensuse" "anaconda")
# These three build it inside the image; they're currently disabled because they're currently broken:
# "ubuntu" "opensuse" "manylinux"

#
# Build them
#
for i in "${arr[@]}"
do
  echo "Building $i..."
  sudo docker build -t $i -f Dockerfile.$i .
done

