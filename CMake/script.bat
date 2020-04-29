
set this_dir=%~dp0

REM change as needed!
set PYTHON=c:\python${Python_VERSION_MAJOR}${Python_VERSION_MINOR}\python.exe

if NOT EXIST %PYTHON% (
	set PYTHON="%LocalAppData%\Programs\Python\Python${Python_VERSION_MAJOR}${Python_VERSION_MINOR}\python.exe"
	if NOT EXIST %PYTHON% (
		echo "change the batch file to locate your python"
		pause
	)
)

set PATH=%this_dir%\bin
set PYTHONPATH=%this_dir%

set PATH=%PYTHON%\..;%PATH%

if "${VISUS_GUI}" == "1" (
	if EXIST %this_dir%\bin\Qt (
		echo "Using internal Qt5" 
		set Qt5_DIR=%this_dir%\bin\Qt
		
	) else (
		echo "Using external PyQt5" 
		for /f "usebackq tokens=*" %%G in (`%PYTHON% -c "import os,PyQt5; print(os.path.dirname(PyQt5.__file__))"`) do set Qt5_DIR=%%G\Qt
		
	)
)

if "${VISUS_GUI}" == "1" (
	set PATH=%Qt5_DIR%\bin;%PATH%
	set QT_PLUGIN_PATH=%Qt5_DIR%\plugins
)

cd %this_dir%
"%this_dir%\${TARGET_FILENAME}" %*




