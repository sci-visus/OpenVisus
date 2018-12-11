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

	exit 0
	
fi

export PYTHONPATH=$(pwd):$(python -c "import sys; print(':'.join(sys.path))")

if (( USE_PYQT5 == 1 )) ; then
	export QT_PLUGIN_PATH=$(python -c "import os,PyQt5; print(os.path.dirname(PyQt5.__file__))")/Qt/plugins
else
	export QT_PLUGIN_PATH=$(pwd)/bin/Qt/plugins
fi

bin/visusviewer.app/Contents/MacOS/visusviewer "$@"
