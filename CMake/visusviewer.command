#!/bin/bash    

this_dir=$(cd "$(dirname "${BASH_SOURCE[0]}" )" && pwd)

cd ${this_dir}

PYTHONPATH=${this_dir} QT_PLUGIN_PATH=${this_dir}/bin/Qt/plugins bin/visusviewer.app/Contents/MacOS/visusviewer "$@"
