#!/bin/sh
cd `dirname $0`
export LD_LIBRARY_PATH=$PWD/bin
export QT_PLUGIN_PATH=$PWD/bin/plugins
export PYTHONPATH=$PWD
find ./visuscache -name '*.lock' -delete
./bin/visusviewer --config datasets/visus.config
