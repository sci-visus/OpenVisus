#!/bin/bash    

this_dir=$(cd "$(dirname "${BASH_SOURCE[0]}" )" && pwd)

cd ${this_dir}

export LD_LIBRARY_PATH=$(python -c "import os,sysconfig; print(os.path.realpath(sysconfig.get_config_var('LIBDIR')))")
export PYTHONPATH=$(pwd):$(python -c "import sys; print(':'.join(sys.path))")
export QT_PLUGIN_PATH=${this_dir}/bin/Qt/plugins

./bin/visusviewer "$@"
