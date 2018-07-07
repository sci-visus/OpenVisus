FROM quay.io/pypa/manylinux1_x86_64

# /////////////////////////////////////////////

# i need curl to download files
RUN yum install -y curl 

# recent version of cmake
RUN  set -x \
	&& mkdir -p /tmp 
	&& curl https://cmake.org/files/v3.12/cmake-3.12.0-rc1.tar.gz -O \
	&& tar xvzf cmake-3.12.0-rc1.tar.gz && cd cmake-3.12.0-rc1 \
	&& ./bootstrap \
	&& make -j 8  \
	&& make install  \
	&& hash -r  \
	&& cd .. && rm -Rf cmake-3.12.0-rc1
	
# swig
RUN set -x \
	&& mkdir -p /tmp 
	&& curl "https://vorboss.dl.sourceforge.net/project/swig/swig/swig-3.0.12/swig-3.0.12.tar.gz" -O \
	&& tar xvzf swig-3.0.12.tar.gz && cd swig-3.0.12 \
	c&& url "https://ftp.pcre.org/pub/pcre/pcre-8.42.tar.gz" -O \
	&& ./Tools/pcre-build.sh \
	&& ./configure  \
	&& make -j 4 \
	&& make install \
	&& cd ../ && rm -Rf swig-3.0.12

# python3
RUN set -x \
	&& mkdir -p /tmp 
	&& yum install -y zlib-devel
	&& curl https://www.python.org/ftp/python/3.6.5/Python-3.6.5.tgz -O \
	&& t&ar xvzf Python-3.6.5.tgz && cd Python-3.6.5  \
	&& echo "SSL=/usr/local" >> >> Modules/Setup.dist \
	&& echo "_ssl _ssl.c -DUSE_SSL -I/usr/local/include -I/usr/local/include/openssl -L/usr/local/lib -lssl -lcrypto" >> Modules/Setup.dist \
	&& ./configure -enable-shared 
	&& make -j 4 \
	&& make install \
	&& pip3 install --upgrade pip \
	&& pip3 install numpy PyQt5==5.9 setuptools wheel twine auditwheel \
	&& cd ../ && rm -Rf Python-3.6.5


# /////////////////////////////////////////////
# OpenVisus (forcing clone)

ENV VISUS_HOME  /home/visus
ENV GIT_BRANCH pip
ENV GITHUB_RAW_URL https://raw.githubusercontent.com/sci-visus/OpenVisus/$GIT_BRANCH
ADD https://api.github.com/repos/sci-visus/OpenVisus/git/refs/heads/$GIT_BRANCH version.json
RUN git clone -b$GIT_BRANCH https://github.com/sci-visus/OpenVisus.git $VISUS_HOME
RUN mkdir -p $VISUS_HOME/build
WORKDIR $VISUS_HOME/build 

RUN cmake -DVISUS_GUI=0 -DDISABLE_OPENMP=1 ../
RUN cmake --build . --target all -- -j 4
RUN cmake --build . --target install

WORKDIR $VISUS_HOME/build/install

RUN ldd ./bin/libVisusKernel.so # informative
RUN for it in $(find bin/*); do patchelf --set-rpath '$ORIGIN' $it; patchelf --shrink-rpath $it; done
RUN ldd ./bin/libVisusKernel.so # informative

RUN python3 setup.py bdist_wheel --python-tag=cp36 --plat-name=linux_x86_64
	
RUN WHEEL=$(find ./dist -iname "*.whl") && \
	WHEEL=${WHEEL/linux_x86_64/manylinux1_x86_64} && \
	mv dist/*.whl $WHEEL && \
	auditwheel show $WHEEL
	
#RUN twine upload --repository-url https://upload.pypi.org/legacy/ $WHEEL











  
