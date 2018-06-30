import os, sys, setuptools
import shutil

this_dir="."
files=[]

# clean
shutil.rmtree("build", ignore_errors=True)
shutil.rmtree("dist", ignore_errors=True)
shutil.rmtree("OpenVisus.egg-info", ignore_errors=True)
shutil.rmtree("__pycache__", ignore_errors=True)

for dirpath, __dirnames__, filenames in os.walk(this_dir):
  for filename in filenames:
    file= os.path.abspath(os.path.join(dirpath, filename))
    files.append(file)

setuptools.setup(
  name = "OpenVisus",
  description = "ViSUS multiresolution I/O, analysis, and visualization system",
  version='1.0.1',
  url="https://github.com/sci-visus/OpenVisus",
  author="visus.net",
  author_email="support@visus.net​",
  packages=["OpenVisus"],
  package_dir={"OpenVisus":'.'},
  package_data={"OpenVisus": files},
  classifiers=(
    "Programming Language :: Python :: 3",
    "License :: OSI Approved :: BSD License",
    'Operating System :: MacOS :: MacOS X',
    'Operating System :: Microsoft :: Windows',
    'Operating System :: POSIX',
    'Operating System :: Unix',
  ),
  platforms=['Linux', 'OS-X', 'Windows'],
  license = "BSD",
  install_requires=[
    'numpy', 
    # "PyQt5>='5.9'" removed because I'm having some problems with travis
  ],
)
