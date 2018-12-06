#!/bin/bash    

this_dir=$(cd "$(dirname "${BASH_SOURCE[0]}" )" && pwd)

cd ${this_dir}

PYTHONPATH=$(pwd) ./bin/visus "$@"
