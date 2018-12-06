cd /d %~dp0

if "%PYTHON_EXECUTABLE%"=="" (

	if EXIST "%cd%\win32\Python\python.exe" (
		set PYTHON_EXECUTABLE=%cd%\win32\Python\python.exe

	) else if EXIST "%cd%\..\..\..\python.exe" (
		set PYTHON_EXECUTABLE=%cd%\..\..\..\python.exe

	) 	
)

if NOT EXIST "%PYTHON_EXECUTABLE%" (
	echo "Cannot find PYTHON_EXECUTABLE"
	pause
	exit -1
)

"%PYTHON_EXECUTABLE%" -m pip install --user numpy

set PATH=%PYTHON_EXECUTABLE%\..;%cd%\bin;%PATH%

bin\visus.exe %*

