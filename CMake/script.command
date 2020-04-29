#!/bin/bash

this_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)

# change as needed
PYTHON=${PYTHON:-python${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}}

if [[ ! -x ${PYTHON} ]] ; then
	# this is brew default...
	PYTHON=$(brew --prefix python@${Python_VERSION_MAJOR}.${Python_VERSION_MINOR})/bin/python${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}
fi

export PYTHONPATH=$(${PYTHON} -c "import sys;print(sys.path)"):${PYTHONPATH}
export DYLD_LIBRARY_PATH=$(${PYTHON} -c "import os,sysconfig;print(os.path.realpath(sysconfig.get_config_var('LIBDIR')))"):${DYLD_LIBRARY_PATH}

if [[ "${VISUS_GUI}" == "1" ]]; then
	if [ -d "${this_dir}/bin/qt" ]; then
		echo "Using internal Qt5"
		export Qt5_DIR="${this_dir}/bin/qt"
	else
		echo "Using external PyQt5" 
		export Qt5_DIR=$("${PYTHON}" -c "import os,PyQt5; print(os.path.dirname(PyQt5.__file__)+'/Qt')")
	fi
	export QT_PLUGIN_PATH="${Qt5_DIR}/plugins"
fi

cd ${this_dir}
${this_dir}/${TARGET_FILENAME} "$@"

