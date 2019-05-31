
set PYTHON_EXECUTABLE=${PYTHON_EXECUTABLE}
set VISUS_GUI=${VISUS_GUI}
set TARGET_FILENAME=${TARGET_FILENAME}

set this_dir=%~dp0
set PATH=%this_dir%\bin

if NOT "%PYTHON_EXECUTABLE%" == "" (
	set PATH=%PYTHON_EXECUTABLE%\..;%PATH%
)

if "%VISUS_GUI%" == "1" (
	if EXIST %this_dir%\bin\Qt (
		echo "Using internal Qt5" 
		set Qt5_DIR=%this_dir%\bin\Qt
	) else (
		echo "Using external PyQt5" 
		for /f "usebackq tokens=*" %%G in (`%PYTHON_EXECUTABLE% -c "import os,PyQt5; print(os.path.dirname(PyQt5.__file__))"`) do set Qt5_DIR=%%G\Qt
	)
)


if "%VISUS_GUI%" == "1" (
	set QT_PLUGIN_PATH=%Qt5_DIR%\plugins
)

cd %this_dir%
"%TARGET_FILENAME%" %*




