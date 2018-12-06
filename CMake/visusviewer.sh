#!/bin/bash    

this_dir=$(cd "$(dirname "${BASH_SOURCE[0]}" )" && pwd)

cd ${this_dir}

PYTHONPATH=$(pwd) QT_PLUGIN_PATH=${this_dir}/bin/Qt/plugins  ./bin/visusviewer "$@"
