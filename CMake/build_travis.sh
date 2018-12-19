#!/bin/bash

if [ "$TRAVIS_OS_NAME" == "osx" ]; then 
	export COMPILER=clang++
	export PRETTY_BUILD_LOG=1
else 
	export COMPILER=g++-4.9
fi

cd "${TRAVIS_BUILD_DIR}" 
chmod +x ./CMake/build*.sh      

if [ -n "${DOCKER_IMAGE}" ]; then
	DOCKER_SHELL=${DOCKER_SHELL:-/bin/bash}
	sudo docker run -d -ti --name mydocker -v $(pwd):/home/OpenVisus -e PYTHON_VERSION=${PYTHON_VERSION} ${DOCKER_IMAGE} ${DOCKER_SHELL}
	sudo docker exec mydocker ${DOCKER_SHELL} -c "cd /home/OpenVisus && ./build.sh"
	sudo chown -R "$USER":"$USER" ${TRAVIS_BUILD_DIR}/build
	sudo chmod -R u+rwx           ${TRAVIS_BUILD_DIR}/build  

else
	./build.sh

fi

if [[ "${TRAVIS_TAG}" != "" ]]; then
	if [[ "$TRAVIS_OS_NAME" == "osx"  || "${DOCKER_IMAGE}" == "quay.io/pypa/manylinux1_x86_64" ]]; then
		echo [distutils]                                  > ${HOME}/.pypirc
		echo index-servers = pypi                        >> ${HOME}/.pypirc
		echo [pypi]                                      >> ${HOME}/.pypirc
		echo username=${PYPI_USERNAME}                   >> ${HOME}/.pypirc
		echo password=${PYPI_PASSWORD}                   >> ${HOME}/.pypirc
		python -m pip install --user --upgrade twine
		python -m twine upload --skip-existing ${TRAVIS_BUILD_DIR}/build/install/dist/*.whl
	fi
fi

