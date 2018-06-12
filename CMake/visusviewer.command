cd `dirname $0`
find ./visuscache -name '*.lock' -exec rm -rf {} \; 2>/dev/null
export QT_PLUGIN_PATH=$(pwd)/bin/Plugins
export PYTHONPATH=$(pwd)
./bin/visusviewer.app/Contents/MacOS/visusviewer --visus-config datasets/visus.config
