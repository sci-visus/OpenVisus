#!/bin/bash    

this_dir=$(cd "$(dirname "${BASH_SOURCE[0]}" )" && pwd)

cd ${this_dir}

if [[  $1 == "--bootstrap" ]] ; then
	python -m pip install --user numpy 
	exit 0
fi


export PYTHONPATH=$(pwd):$(python -c "import sys; print(':'.join(sys.path))")
export LD_LIBRARY_PATH=$(python -c "import os,sysconfig; print(os.path.realpath(sysconfig.get_config_var('LIBDIR')))")

./bin/visus "$@"
