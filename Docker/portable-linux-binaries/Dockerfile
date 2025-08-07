
# ////////////////////////////////////////////////////////
FROM quay.io/pypa/manylinux2014_x86_64 as qt515

RUN yum -y install \
  xz \
	libX11-devel libXrender-devel libXau-devel libXext-devel \
	mesa-libGL-devel mesa-libGLU mesa-libGLU-devel \
	fontconfig fontconfig-devel freetype freetype-devel

RUN \
  curl -L --insecure --retry 3 "http://download.qt.io/official_releases/qt/5.15/5.15.2/single/qt-everywhere-src-5.15.2.tar.xz" -O && \
  tar xf qt-everywhere-src-5.15.2.tar.xz && \
  pushd qt-everywhere-src-5.15.2 && \
  ./configure \
    -prefix /opt/qt515 \
    -opensource \
    -nomake examples \
    -nomake tests \
    -opengl desktop \
    -confirm-license \
    -skip activeqt \
    -skip androidextras \
    -skip connectivity \
    -skip location \
    -skip macextras \
    -skip multimedia \
    -skip sensors \
    -skip serialport \
    -skip wayland \
    -skip webchannel \
    -skip websockets \
    -skip winextras \
    -skip x11extras \
    -skip qtgamepad && \
  make && \
  make install && \
  popd && \
  rm -Rf qt-everywhere-src*

# ////////////////////////////////////////////////////////
FROM quay.io/pypa/manylinux2014_x86_64 as openssl11
WORKDIR /tmp
ADD https://www.openssl.org/source/openssl-1.1.1q.tar.gz /tmp
RUN tar xzf openssl-1.1.1q.tar.gz
WORKDIR /tmp/openssl-1.1.1q
RUN ./config -fpic shared --prefix=/opt/openssl11 --openssldir=/opt/openssl11
RUN make 
RUN make install

# ////////////////////////////////////////////////////////
FROM quay.io/pypa/manylinux2014_x86_64 as python39
RUN yum -y install xz zlib zlib-devel libffi libffi-devel 
COPY --from=openssl11 /opt/openssl11 /opt/openssl11 
WORKDIR /tmp
ADD https://www.python.org/ftp/python/3.9.20/Python-3.9.20.tgz /tmp/
RUN tar xzf Python-*.tgz
WORKDIR /tmp/Python-3.9.20
ENV CFLAGS="-I/opt/openssl11/include/" 
ENV LDFLAGS="-Wl,-rpath=/opt/openssl11/lib -Wl,--rpath=/opt/python39/lib"
RUN ./configure --enable-shared --prefix=/opt/python39 --with-openssl=/opt/openssl11
RUN make 
RUN make altinstall
RUN /opt/python39/bin/python3.9 -m pip install -q --upgrade pip
RUN /opt/python39/bin/python3.9 -m pip install -q numpy twine setuptools wheel urllib3 
RUN /opt/python39/bin/python3.9 -c "import pip,numpy,twine,wheel,urllib3;print(f'pip=={pip.__version__} numpy=={numpy.__version__} twine=={twine.__version__} wheel=={wheel.__version__} urllib3=={urllib3.__version__}')"

# ////////////////////////////////////////////////////////
FROM quay.io/pypa/manylinux2014_x86_64 as python310
RUN yum -y install xz zlib zlib-devel libffi libffi-devel 
COPY --from=openssl11 /opt/openssl11 /opt/openssl11 
WORKDIR /tmp
ADD https://www.python.org/ftp/python/3.10.15/Python-3.10.15.tgz /tmp/
RUN tar xzf Python-3.10.15.tgz
WORKDIR /tmp/Python-3.10.15
ENV CFLAGS="-I/opt/openssl11/include/" 
ENV LDFLAGS="-Wl,-rpath=/opt/openssl11/lib -Wl,--rpath=/opt/python310/lib"
RUN ./configure --enable-shared --prefix=/opt/python310 --with-openssl=/opt/openssl11
RUN make altinstall
RUN /opt/python310/bin/python3.10 -m pip install -q --upgrade pip
RUN /opt/python310/bin/python3.10 -m pip install -q numpy twine setuptools wheel urllib3 
RUN /opt/python310/bin/python3.10 -c "import pip,numpy,twine,wheel,urllib3;print(f'pip=={pip.__version__} numpy=={numpy.__version__} twine=={twine.__version__} wheel=={wheel.__version__} urllib3=={urllib3.__version__}')"

# ////////////////////////////////////////////////////////
FROM quay.io/pypa/manylinux2014_x86_64 as python311
RUN yum -y install xz zlib zlib-devel libffi libffi-devel 
COPY --from=openssl11 /opt/openssl11 /opt/openssl11 
WORKDIR /tmp
ADD https://www.python.org/ftp/python/3.11.10/Python-3.11.10.tgz /tmp/
RUN tar xzf Python-3.11.10.tgz
WORKDIR /tmp/Python-3.11.10
ENV CFLAGS="-I/opt/openssl11/include/" 
ENV LDFLAGS="-Wl,-rpath=/opt/openssl11/lib -Wl,--rpath=/opt/python311/lib"
RUN ./configure --enable-shared --prefix=/opt/python311 --with-openssl=/opt/openssl11
RUN make 
RUN make altinstall
RUN /opt/python311/bin/python3.11 -m pip install -q --upgrade pip
RUN /opt/python311/bin/python3.11 -m pip install -q numpy twine setuptools wheel urllib3 
RUN /opt/python311/bin/python3.11 -c "import pip,numpy,twine,wheel,urllib3;print(f'pip=={pip.__version__} numpy=={numpy.__version__} twine=={twine.__version__} wheel=={wheel.__version__} urllib3=={urllib3.__version__}')"

# ////////////////////////////////////////////////////////
FROM quay.io/pypa/manylinux2014_x86_64 as python312
RUN yum -y install xz zlib zlib-devel libffi libffi-devel 
COPY --from=openssl11 /opt/openssl11 /opt/openssl11 
WORKDIR /tmp
ADD https://www.python.org/ftp/python/3.12.7/Python-3.12.7.tgz /tmp/
RUN tar xzf Python-3.12.7.tgz
WORKDIR /tmp/Python-3.12.7
ENV CFLAGS="-I/opt/openssl11/include/" 
ENV LDFLAGS="-Wl,-rpath=/opt/openssl11/lib -Wl,--rpath=/opt/python312/lib"
RUN ./configure --enable-shared --prefix=/opt/python312 --with-openssl=/opt/openssl11
RUN make 
RUN make altinstall
RUN /opt/python312/bin/python3.12 -m pip install -q --upgrade pip
RUN /opt/python312/bin/python3.12 -m pip install -q numpy twine setuptools wheel urllib3 
RUN /opt/python312/bin/python3.12 -c "import pip,numpy,twine,wheel,urllib3;print(f'pip=={pip.__version__} numpy=={numpy.__version__} twine=={twine.__version__} wheel=={wheel.__version__} urllib3=={urllib3.__version__}')"


# ////////////////////////////////////////////////////////
FROM quay.io/pypa/manylinux2014_x86_64 as python313
RUN yum -y install xz zlib zlib-devel libffi libffi-devel 
COPY --from=openssl11 /opt/openssl11 /opt/openssl11 
WORKDIR /tmp
ADD https://www.python.org/ftp/python/3.13.6/Python-3.13.6.tgz /tmp/
RUN tar xzf Python-3.13.6.tgz
WORKDIR /tmp/Python-3.13.6
ENV CFLAGS="-I/opt/openssl11/include/" 
ENV LDFLAGS="-Wl,-rpath=/opt/openssl11/lib -Wl,--rpath=/opt/python313/lib"
RUN ./configure --enable-shared --prefix=/opt/python313 --with-openssl=/opt/openssl11
RUN make 
RUN make altinstall
RUN /opt/python313/bin/python3.13 -m pip install -q --upgrade pip
RUN /opt/python313/bin/python3.13 -m pip install -q numpy twine setuptools wheel urllib3 
RUN /opt/python313/bin/python3.13 -c "import pip,numpy,twine,wheel,urllib3;print(f'pip=={pip.__version__} numpy=={numpy.__version__} twine=={twine.__version__} wheel=={wheel.__version__} urllib3=={urllib3.__version__}')"


# ////////////////////////////////////////////////////////
FROM quay.io/pypa/manylinux2014_x86_64 as build_openvisus

ENV LC_ALL en_US.UTF-8
ENV LANG en_US.UTF-8
ENV LANGUAGE en_US.UTF-8

RUN yum -y install xz patchelf zlib zlib-devel libffi libffi-devel

COPY --from=openssl11 /opt/openssl11 /opt/openssl11
COPY --from=python39  /opt/python39  /opt/python39
COPY --from=python310 /opt/python310 /opt/python310
COPY --from=python311 /opt/python311 /opt/python311
COPY --from=python312 /opt/python312 /opt/python312
COPY --from=python313 /opt/python313 /opt/python313

# other openvisus dependencies
WORKDIR /tmp

# pcre (needed by httpd and swig)
RUN \
  curl -L --insecure "http://ftp.cs.stanford.edu/pub/exim/pcre/pcre-8.42.tar.gz" -O  && \
	tar xzf pcre*.tar.gz  && pushd pcre*  && ./configure  && make && make install  && popd && rm -Rf pcre*

# cmake
RUN \
  curl -L --insecure "https://github.com/Kitware/CMake/releases/download/v3.17.1/cmake-3.17.1-Linux-x86_64.tar.gz" -O && \
  tar xf cmake*.tar.gz --strip-components=1 -C /usr && rm -R cmake*.tar.gz 

# swig
RUN \
  curl -L --insecure "https://ftp.osuosl.org/pub/blfs/conglomeration/swig/swig-3.0.12.tar.gz" -O  && \
  tar xzf swig-*.tar.gz  && pushd swig-* && ./configure --prefix=/usr && make && make install && popd && rm -Rf swig-* 

# expat (needed by hbttpd)
RUN \
  curl -L --insecure "https://toolchains.bootlin.com/downloads/releases/sources/expat-2.2.6/expat-2.2.6.tar.bz2" -O	 && \
  tar xvjf expat*.bz2  && pushd expat*  && ./configure && make && make install && popd && rm -Rf expat*

# httpd
RUN \
  curl -L --insecure "https://archive.apache.org/dist/httpd/httpd-2.4.38.tar.gz"  -O	 && \
	tar xzf httpd*.tar.gz  && \
	pushd httpd*  && \
    curl -L --insecure "https://archive.apache.org/dist/apr/apr-1.6.5.tar.gz"      -O && tar xvzf apr-1.6.5.tar.gz       && mv ./apr-1.6.5      ./srclib/apr       && \
	  curl -L --insecure "https://archive.apache.org/dist/apr/apr-util-1.6.1.tar.gz" -O	&& tar xvzf apr-util-1.6.1.tar.gz  && mv ./apr-util-1.6.1 ./srclib/apr-util  && \
	  ./configure --with-included-apr && make && make install  && \
	popd  && \
	rm -Rf httpd* 

# gui stuff
RUN yum -y install \
  xz \
	libX11-devel libXrender-devel libXau-devel libXext-devel \
	mesa-libGL-devel mesa-libGLU mesa-libGLU-devel \
	fontconfig fontconfig-devel freetype freetype-devel

COPY --from=qt515 /opt/qt515 /opt/qt515
