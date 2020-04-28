#!/bin/bash

this_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)

# change as needed
PYTHON=python${PYTHON_VERSION_MAJOR}${PYTHON_VERSION_MINOR}

export PYTHONPATH=$(${PYTHON} -c "import sys;print(sys.path)"):${PYTHONPATH}
export LD_LIBRARY_PATH=$(${PYTHON} -c "import os,sysconfig;print(os.path.realpath(sysconfig.get_config_var('LIBDIR')))"):${LD_LIBRARY_PATH}


if [[ "${VISUS_GUI}" == "1" ]] ; then
	if [ -d "${this_dir}/bin/qt" ]; then
		echo "Using internal Qt5"
		export Qt5_DIR=${this_dir}/bin/qt
	else
		echo "Using external PyQt5" 
		export Qt5_DIR=$("${PYTHON}" -c "import os,PyQt5; print(os.path.dirname(PyQt5.__file__)+'/Qt')")
	fi
	export QT_PLUGIN_PATH=${Qt5_DIR}/plugins
fi

cd ${this_dir}
${this_dir}/${TARGET_FILENAME} "$@"

