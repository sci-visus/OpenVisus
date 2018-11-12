FROM opensuse:42.3

ARG PYTHON_VERSION=3.6.6
ARG GIT_BRANCH=master
ARG DOCKER_DEBUG=0
                                
ADD https://api.github.com/repos/sci-visus/OpenVisus/git/refs/heads/$GIT_BRANCH version.json
RUN git clone -b${GIT_BRANCH} https://github.com/sci-visus/OpenVisus.git /home/OpenVisus      

WORKDIR /home/OpenVisus 
RUN chmod +x CMake/build/*.sh
RUN if [ "${DOCKER_DEBUG}" -eq "0" ]; then ./CMake/build/opensuse.sh ; fi




  















