#!/bin/bash

pushd /tmp
tar zcf visus_miniconda_install.tgz -C $CODE/OpenVisus/build/install .
tar zcf visus_webviewer.tgz         -C $CODE ./webviewer
popd

exit 0  #otherwise the next commands won't get executed if the build fails

