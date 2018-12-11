#!/bin/bash    

this_dir=$(cd "$(dirname "${BASH_SOURCE[0]}" )" && pwd)

cd ${this_dir}

if [ ! -f .visus.command.ready ] ; then
	python -m pip install --user numpy 
	touch .visus.command.ready
fi

export PYTHONPATH=$(pwd):$(python -c "import sys; print(':'.join(sys.path))")

./bin/visus.app/Contents/MacOS/visus "$@"
