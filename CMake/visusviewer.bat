cd /d %~dp0
set PYTHONPATH=./;%PYTHONPATH%
bin\\$<TARGET_FILE_NAME:visusviewer> --visus-config datasets/visus.config
pause
