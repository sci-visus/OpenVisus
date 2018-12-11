#!/bin/bash    

this_dir=$(cd "$(dirname "${BASH_SOURCE[0]}" )" && pwd)

cd ${this_dir}

# assuming the current python is version-compatible
export PYTHONPATH=$(pwd):$(python -c "import sys; print(':'.join(sys.path))")

./bin/visus.app/Contents/MacOS/visus "$@"
