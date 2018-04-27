#!/bin/sh
cd $(dirname $0)
export QT_PLUGIN_PATH=$PWD/plugins
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$QT_PLUGIN_PATH
find ./visuscache -name '*.lock' -delete
./visusviewer

