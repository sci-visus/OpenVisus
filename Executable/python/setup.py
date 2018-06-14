#!/usr/bin/env python

import os, sys

package_name="visuspy"

# ///////////////////////////////////////////////////////
def collectFiles():

  this_dir=os.path.abspath(os.path.dirname(__file__))
  ret=[]
  for root, dirnames, filenames in os.walk(this_dir):

    # ignore in-place created directory
    bIgnore=False
    for purge_dir in ("build", "dist","visuspy.egg-info"):
      if root.startswith(os.path.join(this_dir,purge_dir)) :
        bIgnore=True
    
    if bIgnore:
      continue

    for filename in filenames:
      print("Found",filename)
      ret.append(os.path.join(root, filename))
  
  return ret

import setuptools
setuptools.setup(
  name = package_name,
  description = "ViSUS multiresolution I/O, analysis, and visualization system",
  version='1.0.0',
  url = "http://visus.net",
  packages=[package_name],
  package_dir={package_name:''},
  package_data={package_name: collectFiles()}
)    
    
    