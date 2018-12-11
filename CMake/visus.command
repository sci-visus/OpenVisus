#!/bin/bash    

this_dir=$(cd "$(dirname "${BASH_SOURCE[0]}" )" && pwd)

cd ${this_dir}

if [[  $1 == "--bootstrap" ]] ; then
	python -m pip install --user numpy 
	exit 0
fi

export PYTHONPATH=$(pwd):$(python -c "import sys; print(':'.join(sys.path))")
./bin/visus.app/Contents/MacOS/visus "$@"
