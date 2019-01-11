#!/bin/bash

pushd OpenVisus/build
cmake --build . --target install -- -j16
popd

pushd XIDX/build
cmake --build . --target install -- -j16
popd

#nothing to do to build webviewer

exit 0  #otherwise the next commands won't get executed if the build fails

