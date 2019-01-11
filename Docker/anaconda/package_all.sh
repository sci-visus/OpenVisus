#!/bin/bash

tar zcf visus_miniconda_install.tgz -C $CODE/OpenVisus/build/install .
tar zcf visus_webviewer.tgz         -C $CODE ./webviewer

