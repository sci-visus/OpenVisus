#!/bin/bash

set -ex

# TAG=$(python3 Libs/swig/setup.py new-tag) && echo ${TAG}
TAG=2.2.81

git commit -a -m "New tag ($TAG)" 
git tag -a $TAG -m "$TAG"
git push origin $TAG
git push origin

