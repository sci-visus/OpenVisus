FROM quay.io/pypa/manylinux1_x86_64

yum update

# i need curl to download files
RUN yum install -y curl 
RUN yum install -y mesa-libGL mesa-libGLU mesa-libGL-devel  mesa-libGLU-devel

# recent version of cmake
RUN  set -x \
	&& mkdir -p /tmp && cd /tmp \
	&& curl https://cmake.org/files/v3.12/cmake-3.12.0-rc1.tar.gz -O \
	&& tar xvzf cmake-3.12.0-rc1.tar.gz && cd cmum install mesa-libGLUake-3.12.0-rc1 \
	&& ./bootstrap \
	&& make -j 8  \yum install mesa-libGL mesa-libGLU mesa-libGL-devel  mesa-libGLU-devel
	&& make install  \
	&& hash -r  \
	&& cd .. && rm -Rf cmake-3.12.0-rc1
	
# swig
RUN set -x \
	&& mkdir -p /tmp && cd /tmp \
	&& curl "https://ftp.osuosl.org/pub/blfs/conglomeration/swig/swig-3.0.12.tar.gz" -O \
	&& tar xvzf swig-3.0.12.tar.gz && cd swig-3.0.12 \
	&& curl "https://ftp.pcre.org/pub/pcre/pcre-8.42.tar.gz" -O \
	&& ./Tools/pcre-build.sh \
	&& ./configure  \
	&& make -j 4 \
	&& make install \
	&& cd ../ && rm -Rf swig-3.0.12

# openssl 
RUN set -x \
	&& mkdir -p /tmp && cd /tmp \
	&& curl https://www.openssl.org/source/openssl-1.0.2a.tar.gz -O \
	&& tar xvzf openssl-1.0.2a.tar.gz && cd openssl-1.0.2a \
	&& ./config -fpic shared --prefix=/usr/local \
	&& make \um install mesa-libGLU
	&& make install \
	&& cd ../ && rm -Rf openssl-1.0.2a 

# python3
RUN set -x \
	&& mkdir -p /tmp && cd /tmp \
	&& yum install -y zlib-devel \
	&& curl "https://www.python.org/ftp/python/3.6.5/Python-3.6.5.tgz" -O \
	&& tar xvzf Python-3.6.5.tgz \
	&& cd Python-3.6.5  \
	&& echo "SSL=/usr/local/ssl" >> Modules/Setup.dist \
	&& echo "_ssl _ssl.c -DUSE_SSL -I/usr/local/include -I/usr/local/include/openssl -L/usr/local/lib -lssl -lcrypto" >> Modules/Setup.dist \
	&& ./configure -enable-shared \
	&& make -j 4 \
	&& make install \
	&& cd ../ \RUN yum install -y mesa-libGL mesa-libGLU mesa-libGL-devel  mesa-libGLU-devel
	&& rm -Rf Python-3.6.5
	
# install pip3 packages
RUN set -x \
	&& pip3 install --upgrade pip \
	&& pip3 install numpy PyQt5==5.9 setuptools wheel twine auditwheel 

# xcb proto (for qt)
RUN set -x \
	&& mkdir -p /tmp && cd /tmp \
	&& wget --no-check-certificate http://xcb.freedesktop.org/dist/xcb-proto-1.11.tar.gz \
	&& tar -xzf xcb-proto-1.11.tar.gz \
	&& cd xcb-proto-1.11 \
	&& ./configure\RUN yum install -y mesa-libGL mesa-libGLU mesa-libGL-devel  mesa-libGLU-devel
	&& make -j8 \
	&& make install \
	&& cd .. && rm -Rf xcb-proto-1.11

# pthread stubs (for qt)
RUN set -x \
	&& mkdir -p /tmp && cd /tmp \
	&& wget --no-check-certificate http://xcb.freedesktop.org/dist/libpthread-stubs-0.3.tar.gz \
	&& tar -xzf libpthread-stubs-0.3.tar.gz \
	&& cd libpthread-stubs-0.3 \
	&& ./configure \
	&& make -j8 \
	&& make install \
	&& cd .. && rm -Rf libpthread-stubs-0.3

# xcb (for qt)
RUN set -x \
	&& mkdir -p /tmp && cd /tmp \
	&& wget --no-check-certificate http://xcb.freedesktop.org/dist/libxcb-1.11.tar.gz \
	&& tar -xzf libxcb-1.11.tar.gz \
	&& cd libxcb-1.11 \
	&& ./configure \
	&& make -j8 \
	&& make install \
	&& cd ..  && rm -Rf libxcb-1.11

# qt 
RUN set -x \
  && mkdir -p /tmp && cd /tmp \
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
  && cd ..  && rm -Rf qt-everywhere-opensource-src-5.4.0 


# /////////////////////////////////////////////
# OpenVisus 

ENV GIT_BRANCH scrgiorgio

# force cloning
ADD https://api.github.com/repos/sci-visus/OpenVisus/git/refs/heads/$GIT_BRANCH version.json

RUN set -x \
	&& git clone -b$GIT_BRANCH https://github.com/sci-visus/OpenVisus.git /home/visus \
	&& cd /home/visus  && mkdir -p build && cd build \
	&& cmake  -DCMAKE_BUILD_TYPE=RelWithDebugInfo -DDISABLE_OPENMP=1  -DQt5_DIR=/usr/local/Qt-5.4.0/lib/cmake/Qt5/ ../ \
	&& cmake --build . --target all -- -j 4 \
	&& cmake --build . --target install \
	&& cmake --build . --target dist 
	

WORKDIR /home/visus/build/install

RUN set -x \  
	&& ldd libVisusKernel.so # informative
  && WHEEL=$(find ./dist -iname "*.whl") \
  && WHEEL=${WHEEL/linux_x86_64/manylinux1_x86_64} \
  && mv dist/*.whl $WHEEL \
  && auditwheel show $WHEEL
  









  
