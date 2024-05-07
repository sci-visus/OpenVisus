# VISUS_IDX2

## Introduction

Links:
- https://github.com/sci-visus/idx2.git

IDX2 is disabled by default in GitHub Actions since it requires C++17 (OpenVisus requires C++11 only) 
and the 'AVX2' architecture, so binary packages DO NOT CONTAIN IDX2.

If you want to enable IDX2 inside OpenVisus you need to manually compile binaries:

set `VISUS_IDX2` to "ON" in CMake.


# Familiarize with OpenVisus

See:
- https://github.com/sci-visus/OpenVisus

Install OpenVisus Pip binaries; run the viewer; open some datasets, run jupyter notebooks, run quick tour etc

# OpenVisus with IDX2 support


Download a test file from here https://github.com/sci-visus/OpenVisus/releases/download/files/MIRANDA-DENSITY-.384-384-256.-Float64.raw

Setup your prompt:

```
SET PATH=%PATH%;C:\Python38;C:\Python38\Lib\site-packages\PyQt5\Qt\bin;build\RelWithDebInfo\OpenVisus\bin
SET VISUS_VERBOSE_DISKACCESS=0
SET VISUS_CPP_VERBOSE=0
```

Test pure IDX2, with IDX2 file format (not cachable, not cloud-ready)


```
set VISUS_USE_IDX2_FILE_FORMAT=1
rmdir /S /Q MIRANDA
visus.exe idx2 --encode "MIRANDA-DENSITY-.384-384-256.-Float64.raw" --name Miranda --field Density --dims  384 384 256 --type float64 --tolerance 1e-16 --num_levels 2 --out_dir .
visus.exe idx2 --decode Miranda/Density.idx2 --downsampling 1 1 1 --tolerance 0.001 --out_file test.raw
python Samples/python/extract_slices.py test.raw  193 193 129 float64
visusviewer.exe "MIRANDA/Density.idx2"
```

Then test using one-chunk per file (useful for CloudAccess, DiskAccess):
- you will see a lot of files, one per block/chunk

```
set VISUS_USE_IDX2_FILE_FORMAT=0
rmdir /S /Q MIRANDA
visus.exe idx2 --encode MIRANDA-DENSITY-.384-384-256.-Float64.raw --name Miranda --field Density --dims  384 384 256 --type float64 --tolerance 1e-16 --num_levels 2 --out_dir .
visus.exe idx2 --decode Miranda/Density.idx2 --downsampling 1 1 1 --tolerance 0.001 --out_file test.raw
python Samples/python/extract_slices.py test.raw  193 193 129 float64
visusviewer.exe "MIRANDA/Density.idx2"
```