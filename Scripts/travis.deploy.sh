#!/bin/sh

DEPLOY_HOST=buildbot@atlantis.sci.utah.edu
DEPLOY_DIRNAME=/usr/sci/cedmav/builds/visus/${TRAVIS_BRANCH}/${TRAVIS_OS_NAME}/${BUILD_TYPE}/$(date +'%Y%m%d%H%M%S')
DEPLOY_FILENAME=$DEPLOY_DIRNAME/ViSUS.$(git log --format=%h -1).tar.gz

rm -f ~/.ssh/config
sudo chmod 400 ~/.ssh/config
echo "Host *"                          >> ~/.ssh/config
echo "   StrictHostKeyChecking no"     >> ~/.ssh/config
echo "   userKnownHostsFile=/dev/null" >> ~/.ssh/config
echo ""                                >> ~/.ssh/config

ssh ${DEPLOY_HOST} "mkdir -p ${DEPLOY_DIRNAME}"
scp -p "${TRAVIS_BUILD_DIR}/build/ViSUS.tar.gz" ${DEPLOY_HOST}:${DEPLOY_FILENAME}
ssh ${DEPLOY_HOST}  "chmod go+r ${DEPLOY_FILENAME}"
