#/bin/bash

# very verbose
set -x

# stop on error
set -e

# stop of the first command that fails with pipe (|)
set -o pipefail

cd "${TRAVIS_BUILD_DIR}"

# avoid collision with other builds
BUILD_DIR=./build_docker

# in case I need to deploy
cat <<EOF > ./.pypirc
[distutils]
index-servers =  pypi
[pypi]
username=${PYPI_USERNAME}
password=${PYPI_PASSWORD}
EOF

# I'm using an existing Docker image here (see scripts/build_travis.Dockerfile)
sudo docker run -ti \
	-v $(pwd):/root/OpenVisus \
	-e BUILD_DIR=${BUILD_DIR} \
	-e PYTHON_VERSION=${PYTHON_VERSION} \
	-e PYTHON_EXECUTABLE=/usr/local/bin/python${PYTHON_VERSION} \
	-d TRAVIS_TAG=${TRAVIS_TAG}
	-e Qt5_DIR=/opt/qt59 \
	-w /root/OpenVisus \
	visus/travis-image \
	/bin/bash -c "./scripts/build.sh"

# reassign permissions in case they changed
sudo chmod -R a+rwx           ${BUILD_DIR}   
sudo chown -R "$USER":"$USER" ${BUILD_DIR}



