#!/bin/sh
cd "`dirname "${0}"`"
find ./visuscache -name '*.lock' -delete
export QT_PLUGIN_PATH=$PWD/PlugIns/
./visusviewer.app/Contents/MacOS/visusviewer 
