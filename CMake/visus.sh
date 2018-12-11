#!/bin/bash    

this_dir=$(cd "$(dirname "${BASH_SOURCE[0]}" )" && pwd)

cd ${this_dir}

if [ ! -f .visus.sh.ready ] ; then
	python -m pip install --user numpy 
	touch .visus.sh.ready
fi

export PYTHONPATH=$(pwd):$(python -c "import sys; print(':'.join(sys.path))")
export LD_LIBRARY_PATH=$(python -c "import os,sysconfig; print(os.path.realpath(sysconfig.get_config_var('LIBDIR')))")

./bin/visus "$@"
