#!/bin/bash

#
# Build the OpenVisus docker images.
#

# These four images use pip to install OpenVisus:
declare -a arr=("mod_visus-ubuntu" "mod_visus-opensuse" "anaconda" "dataportal")
# ...but these three build it inside the image, so they're currently disabled since they take so long:
# "ubuntu" "opensuse" "manylinux"

#
# Build them
#
for i in "${arr[@]}"
do
  echo "Building $i..."
  sudo docker build -t $i -f Dockerfile.$i .
done

