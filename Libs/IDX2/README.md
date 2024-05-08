# What is idx2?
idx2 is a compressed file format for scientific data represented as 2D or 3D regular grids of data samples.
From a single idx2 dataset, you can retrieve data at different resolution and tolerance levels that best suit your needs at hand.
<!-- TODO: what is the speed? -->
<!-- idx2 supports adaptive, coarse-scale data retrieval in both resolution and precision. -->
<!-- idx2 is the next version of the idx file format, which is handled by [OpenVisus](https://github.com/sci-visus/OpenVisus) (alternatively, a less extensive but lightweight idx reader and writer is [hana](https://github.com/hoangthaiduong/hana)). Compared to idx, idx2 features better compression (leveraging [zfp](https://github.com/LLNL/zfp)) and the capability to retrieve coarse-precision data. -->
<!-- Currently there is an executable (named `idx2App`) for 2-way conversion between raw binary and the idx2 format, and a header-only library (`idx2.hpp`) for working with the format at a lower level. -->

# Compilation
`idx2` can be built using CMake. The dependencies are:

- CMake (>= 3.8)
- A C++ compiler supporting C++17
- (Optional) nanobind (do `git submodule update --recursive --init` to pull it from GitHub)
- (Optional) Python 3

The optional dependencies are only needed if `BUILD_IDX2PY` is set to `ON` in CMake.

# Using the `idx2App` command line tool to encode raw to idx2
```
idx2App --encode Miranda-Viscosity-[384-384-256]-Float64.raw
```

For convenience, the dimensions of the input are automatically parsed if the input file is named in the `Name-Field-[DimX-DimY-DimZ]-Type.raw` format.
<!-- <!-- , where `Name` and `Field` can be anything, `DimX`, `DimY`, `DimZ` are the field's dimensions (any of which can be 1), and  -->
Note that `Type` can only be either `Float32` or `Float64` (currently idx2 only supports **floating-point** scalar fields).
If the input raw file name is not in this form, please additionally provide `--name`, `--field`, `--dims`, and `--type`.
<!-- Most of the time, the only options that should be customized are `--input` (the input raw file), `--out_dir` (the output directory), `--num_levels` (the number of resolution levels) and `--tolerance` (the absolute error tolerance).
The outputs will be multiple files written to the `out_dir/Name` directory, and the main metadata is stored in `out_dir/Name/Field.idx2`. -->

# Using the `idx2App` command line tool to decode idx2 to raw
```
idx2App --decode Miranda/Viscosity.idx2 --downsampling 1 1 1 --tolerance 0.001
```

`--downsampling` specifies the desired downsampling passes along each axis (each pass halves the number of samples along an axis), and `--tolerance` to specify the desired absolute error tolerance.
<!-- The output will be written to a raw file in the current directory. -->
Optionally, use `--first x_begin y_begin z_begin` and `--last x_end y_end z_end` (the end points are inclusive) to specify the region of interest instead of decoding the whole field.

# Using the C++ API to read from an idx2 dataset to memory

See the `Source/Applications/idx2Samples.cpp` file for an example of how to use idx2's C++ API.

## (Most convenient option) Using the header-only library `idx2.hpp`
Just include a single header file for convenience. The `idx2.hpp` header file can be included anywhere, but you need to `#define idx2_Implementation` in *exactly one* of your cpp files before including it.

```
#define idx2_Implementation
#include <idx2.hpp>
```

## Using the compiled `idx2` library
Alternatively, with CMake, you can build an `idx2` library and link it against your project. Then, just `#include <idx2.h>` to use it.

<!-- For instructions on using the library, please refer to the code examples with comments in `Source/Applications/Examples.cpp`. -->

# References
[Efficient and flexible hierarchical data layouts for a unified encoding of scalar field precision and resolution](https://ieeexplore.ieee.org/document/9222049)
D. Hoang, B. Summa, H. Bhatia, P. Lindstrom, P. Klacansky, W. Usher, P-T. Bremer, and V. Pascucci.
2021 - IEEE Transactions on Visualization and Computer Graphics

For the paper preprint and presentation slides, see http://www.sci.utah.edu/~duong
