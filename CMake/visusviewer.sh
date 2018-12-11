#!/bin/bash    

this_dir=$(cd "$(dirname "${BASH_SOURCE[0]}" )" && pwd)

cd ${this_dir}

USE_PYQT5=${USE_PYQT5:0}

if [[  $1 == "--bootstrap" ]] ; then

	python -m pip install --user numpy 
	
	if (( USE_PYQT5 == 1 )) ; then
		rm -Rf ./bin/Qt*
		python -m pip install --user PyQt5  
		python deploy.py --set-qt5 $(python -c "import os,PyQt5; print(os.path.dirname(PyQt5.__file__))")/Qt
	fi

fi

export PYTHONPATH=$(pwd):$(python -c "import sys; print(':'.join(sys.path))")
export LD_LIBRARY_PATH=$(python -c "import os,sysconfig; print(os.path.realpath(sysconfig.get_config_var('LIBDIR')))")

if (( USE_PYQT5 == 1 )) ; then
	QT5_DIR=$(python -c "import os,PyQt5; print(os.path.dirname(PyQt5.__file__))")/Qt
	export LD_LIBRARY_PATH=${QT5_DIR}/lib:${LD_LIBRARY_PATH}
	export QT_PLUGIN_PATH=${QT5_DIR}/plugins
else
	export QT_PLUGIN_PATH=$(pwd)/bin/Qt/plugins
fi

./bin/visusviewer "$@"
