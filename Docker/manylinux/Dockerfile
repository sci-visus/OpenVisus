FROM quay.io/pypa/manylinux1_x86_64

ARG PYTHON_VERSION
ARG GIT_BRANCH

# //////////////////////////////////////////
RUN set -x \
  && yum update \
  && yum install -y zlib-devel curl 

# //////////////////////////////////////////
# openssl 

RUN set -x \
  && mkdir -p /tmp \
  && cd /tmp \
  && curl https://www.openssl.org/source/openssl-1.0.2a.tar.gz -O \
  && tar xvzf openssl-1.0.2a.tar.gz \
  && cd openssl-1.0.2a \cmake --build . --target all -- -j 4 \
  && ./config -fpic shared --prefix=/usr --openssldir=/usr \
  && make \
  && make install \
  && cd ../ \
  && rm -Rf openssl-1.0.2a 


# //////////////////////////////////////////
# python

RUN mkdir -p /tmp && cd /tmp 
RUN curl -L https://raw.githubusercontent.com/yyuu/pyenv-installer/master/bin/pyenv-installer -O 
ENV PATH /root/.pyenv/shims:/root/.pyenv/bin:${PATH}
RUN chmod a+x pyenv-installer && ./pyenv-installer && rm -f ./pyenv-installer
RUN CONFIGURE_OPTS=--enable-shared pyenv install ${PYTHON_VERSION} 
RUN pyenv global ${PYTHON_VERSION} 
RUN pyenv rehash
RUN python -m pip install --upgrade pip 
RUN python -m pip install numpy setuptools wheel twine auditwheel

# //////////////////////////////////////////
# opengl (for qt)

RUN set -x \
  && yum install -y mesa-libGL mesa-libGLU mesa-libGL-devel mesa-libGLU-devel 

# //////////////////////////////////////////
# xcb proto (for qt)

RUN set -x \
  && mkdir -p /tmp \
  && cd /tmp \
  && wget --no-check-certificate http://xcb.freedesktop.org/dist/xcb-proto-1.11.tar.gz \
  && tar -xzf xcb-proto-1.11.tar.gz \
  && cd xcb-proto-1.11 \
  && ./configure \
  && make -j8 \
  && make install \
  && cd .. \
  && rm -Rf xcb-proto-1.11

# //////////////////////////////////////////
# pthread stubs (for qt)

RUN set -x \
  && mkdir -p /tmp \
  && cd /tmp \
  && wget --no-check-certificate http://xcb.freedesktop.org/dist/libpthread-stubs-0.3.tar.gz \
  && tar -xzf libpthread-stubs-0.3.tar.gz \
  && cd libpthread-stubs-0.3 \
  && ./configure \
  && make -j8 \
  && make install \
  && cd .. \
  && rm -Rf libpthread-stubs-0.3

# //////////////////////////////////////////
# xcb (for qt)

RUN set -x \
  && mkdir -p /tmp \
  && cd /tmp \
  && wget --no-check-certificate http://xcb.freedesktop.org/dist/libxcb-1.11.tar.gz \
  && tar -xzf libxcb-1.11.tar.gz \
  && cd libxcb-1.11 \
  && ./configure \
  && make -j8 \
  && make install \
  && cd .. \
  && rm -Rf libxcb-1.11

# //////////////////////////////////////////
# qt 

RUN set -x \
  && mkdir -p /tmp \
  && cd /tmp \
  && wget --no-check-certificate http://qt.mirror.constant.com/archive/qt/5.4/5.4.0/single/qt-everywhere-opensource-src-5.4.0.tar.gz \
  && tar -xzf qt-everywhere-opensource-src-5.4.0.tar.gz \
  && cd qt-everywhere-opensource-src-5.4.0 \
  && sed -i "s/#define QTESTLIB_USE_PERF_EVENTS/#undef QTESTLIB_USE_PERF_EVENTS/g" qtbase/src/testlib/qbenchmark_p.h \
  && ./configure -R ‘\\\$$ORIGIN’ \
     -D _X_INLINE=inline \
     -D XK_dead_currency=0xfe6f \
     -D XK_ISO_Level5_Lock=0xfe13 \
     -D FC_WEIGHT_EXTRABLACK=215 \
     -D FC_WEIGHT_ULTRABLACK=FC_WEIGHT_EXTRABLACK \
     -DGLX_GLXEXT_LEGACY \
     -v -opensource -confirm-license \
     -sysconfdir /etc/xdg \
     -release -shared \
     -qt-zlib -qt-libpng -qt-libjpeg -qt-pcre -qt-xcb -qt-xkbcommon \
     -xkb-config-root /usr/share/X11/xkb \
     -no-xcb-xlib \
     -c++11 \
     -nomake examples -nomake tests -no-dbus -no-icu \
     -skip activeqt -skip androidextras -skip connectivity -skip enginio -skip location -skip macextras \
     -skip multimedia -skip quick1 -skip sensors -skip serialport -skip wayland -skip webchannel -skip webengine \
     -skip webkit -skip webkit-examples -skip websockets -skip winextras -skip x11extras \
  && make -j8 \
  && make install \
  && cd .. \
  && rm -Rf qt-everywhere-opensource-src-5.4.0 cd 


# //////////////////////////////////////////
# cmake (for OpenVisus)

RUN set -x \
  && mkdir -p /tmp \
  && cd /tmp \
  && curl https://cmake.org/files/v3.12/cmake-3.12.0-rc1.tar.gz -O \
  && tar xvzf cmake-3.12.0-rc1.tar.gz \
  && cd cmake-3.12.0-rc1 \
  && ./bootstrap \
  && make -j 8 \
  && make install \
  && hash -r \
  && cd .. \
  && rm -Rf cmake-3.12.0-rc1rc1
	
# //////////////////////////////////////////
# swig (for OpenVisus)

RUN set -x \
  && mkdir -p /tmp \
  && cd /tmp \
  && curl "https://ftp.osuosl.org/pub/blfs/conglomeration/swig/swig-3.0.12.tar.gz" -O \
  && tar xvzf swig-3.0.12.tar.gz \
  && cd swig-3.0.12 \
  && curl "https://ftp.pcre.org/pub/pcre/pcre-8.42.tar.gz" -O \
  && ./Tools/pcre-build.sh \
  && ./configure \
  && make -j 4 \
  && make install \
  && cd ../ \
  && rm -Rf swig-3.0.12

# /////////////////////////////////////////////
# OpenVisus 

# force cloning
ADD https://api.github.com/repos/sci-visus/OpenVisus/git/refs/heads/$GIT_BRANCH version.json

RUN set -x \
  && git clone -b${GIT_BRANCH} https://github.com/sci-visus/OpenVisus.git /home/visus \
  && cd /home/visus \
  && mkdir -p build \
  && cd build \
  && PYTHON_INT_VERSION=${PYTHON_VERSION:0:3} \
  && if [ "${PYTHON_VERSION:0:1}" -gt "2" ]; then PYTHON_INT_VERSION=${PYTHON_INT_VERSION}m ;fi \
  && cmake \
    -DCMAKE_BUILD_TYPE=RelWithDebugInfo \
    -DDISABLE_OPENMP=1 \
    -DQt5_DIR=/usr/local/Qt-5.4.0/lib/cmake/Qt5/ \
    -DPYTHON_VERSION=${PYTHON_VERSION} \
    -DPYTHON_EXECUTABLE=$HOME/.pyenv/versions/${PYTHON_VERSION}/bin/python \
    -DPYTHON_INCLUDE_DIR=$HOME/.pyenv/versions/${PYTHON_VERSION}/include/python${PYTHON_INT_VERSION} \
    -DPYTHON_LIBRARY=$HOME/.pyenv/versions/${PYTHON_VERSION}/lib/libpython${PYTHON_INT_VERSION}.so \
    ../ \
  && cmake --build . --target all -- -j 4 \
  && cmake --build . --target install \
  && cmake --build . --target deploy \
  && cmake --build . --target dist 
  
  
WORKDIR  /home/visus/build/install

RUN LD_LIBRARY_PATH='' ldd libVisusKernel.so # informative 

RUN WHEEL=$(find ./dist -iname "*.whl") \
	&& WHEEL=${WHEEL/linux_x86_64/manylinux1_x86_64} \
	&& mv dist/*.whl ${WHEEL} \
	&& LD_LIBRARY_PATH="./:./Frameworks:/usr/local/lib" auditwheel show ${WHEEL} 
	
RUN  echo "Created $(find /home/visus/build/install/dist/*.whl)"
  










  
