
@echo on
setlocal

set DEPLOY_HOST=buildbot@atlantis.sci.utah.edu
set DEPLOY_DIRNAME=/usr/sci/cedmav/builds/visus/%APPVEYOR_REPO_BRANCH%/win_%PLATFORM%/%CONFIGURATION%/%APPVEYOR_REPO_COMMIT_TIMESTAMP:~0,10%
set DEPLOY_FILENAME=%DEPLOY_DIRNAME%/ViSUS.%APPVEYOR_REPO_COMMIT%.zip

set SSH=ssh -o 'StrictHostKeyChecking=no' -o 'UserKnownHostsFile=/dev/null'
set SCP=scp -o 'StrictHostKeyChecking=no' -o 'UserKnownHostsFile=/dev/null' -p 

%SSH% %DEPLOY_HOST% "mkdir -p %DEPLOY_DIRNAME%"
%SCP% "%APPVEYOR_BUILD_FOLDER%/build/ViSUS.zip" "%DEPLOY_HOST%:%DEPLOY_FILENAME%"
%SSH% %DEPLOY_HOST% "chmod go+r %DEPLOY_FILENAME%"  

