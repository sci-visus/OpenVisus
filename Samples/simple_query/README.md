# Simple External App

The simple example uses the VisusIO libraries to read meta-data about
some dataset passed, and print this information to the console. This example
provides the simples demo of linking with the Visus libraries from a client
application and using the library to open a datset.

## Building

After compiling and installing Visus you can build the simple sample. From
this directory:

```bash
mkdir build
cd build
cmake -DVisus_DIR=<path to visus install>/lib/cmake/visus/
make
```

## Running

You can then run the application and pass the path or URL to an IDX dataset.

```
./simple_external_app <idx file>
```

