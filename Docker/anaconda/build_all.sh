#!/bin/bash

cd OpenVisus
cmake --build . --target install -- -j16

cd ../XIDX
cmake --build . --target install -- -j16

#nothing to do for webviewer

