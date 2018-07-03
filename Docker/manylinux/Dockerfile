FROM quay.io/pypa/manylinux1_x86_64

ENV VISUS_HOME  /home/visus
ENV DEPS_DIR /home/visus_deps
ENV GIT_BRANCH pip
ENV GITHUB_RAW_URL https://raw.githubusercontent.com/sci-visus/OpenVisus/$GIT_BRANCH
ENV PATH             "$DEPS_DIR/bin:${PATH}"
ENV LD_LIBRARY_PATH  "$DEPS_DIR/lib:${LD_LIBRARY_PATH}"

RUN mkdir -p /tmp 

# /////////////////////////////////////////////
# install OpenVisus dependencies

RUN yum install -y curl  

ADD cmake.sh /tmp/
RUN chmod a+rx /tmp/cmake.sh && /tmp/cmake.sh $DEPS_DIR && rm /tmp/cmake.sh

ADD zlib.sh /tmp/
RUN chmod a+rx /tmp/zlib.sh && /tmp/zlib.sh $DEPS_DIR  && rm /tmp/zlib.sh

ADD lz4.sh /tmp/
RUN chmod a+rx /tmp/lz4.sh && /tmp/lz4.sh $DEPS_DIR && rm /tmp/lz4.sh

ADD tinyxml.sh /tmp/
RUN chmod a+rx /tmp/tinyxml.sh && /tmp/tinyxml.sh $DEPS_DIR && rm /tmp/tinyxml.sh

ADD freeimage.sh /tmp/freeimage.sh
RUN chmod a+rx /tmp/freeimage.sh && /tmp/freeimage.sh $DEPS_DIR && rm /tmp/freeimage.sh

ADD swig.sh /tmp/
RUN chmod a+rx /tmp/swig.sh && /tmp/swig.sh $DEPS_DIR && rm /tmp/swig.sh

ADD openssl.sh /tmp/
RUN chmod a+rx /tmp/openssl.sh && /tmp/openssl.sh $DEPS_DIR && rm /tmp/openssl.sh

ADD curl.sh /tmp/
RUN chmod a+rx /tmp/curl.sh && /tmp/curl.sh $DEPS_DIR && rm /tmp/curl.sh

ADD python36.sh /tmp/
RUN chmod a+rx /tmp/python36.sh && /tmp/python36.sh $DEPS_DIR && rm /tmp/python36.sh

# /////////////////////////////////////////////
# OpenVisus compilation step (forcing clone)

ADD https://api.github.com/repos/sci-visus/OpenVisus/git/refs/heads/$GIT_BRANCH version.json
RUN git clone -b$GIT_BRANCH https://github.com/sci-visus/OpenVisus.git $VISUS_HOME
RUN mkdir -p $VISUS_HOME/build
WORKDIR $VISUS_HOME/build 

RUN ls -lF $DEPS_DIR/bin/

RUN cmake \
  -DVISUS_GUI=0 \
  -DDISABLE_OPENMP=1 \
  -DZLIB_INCLUDE_DIR=$DEPS_DIR/include \
  -DZLIB_LIBRARY=$DEPS_DIR/lib/libz.so \
  -DLZ4_INCLUDE_DIR=$DEPS_DIR/include \
  -DLZ4_LIBRARY=$DEPS_DIR/lib/liblz4.so \
  -DTinyXML_INCLUDE_DIR=$DEPS_DIR/include \
  -DTinyXML_LIBRARY=$DEPS_DIR/lib/libtinyxml.so \
  -DFREEIMAGE_INCLUDE_DIRS=$DEPS_DIR/include \
  -DFREEIMAGE_LIBRARIES=$DEPS_DIR/lib/libfreeimage-3.16.0.so \
  -DOPENSSL_INCLUDE_DIR=$DEPS_DIR/include \
  -DOPENSSL_SSL_LIBRARY=$DEPS_DIR/lib/libssl.so \
  -DOPENSSL_CRYPTO_LIBRARY=$DEPS_DIR/lib/libcrypto.so \
  -DCURL_INCLUDE_DIR=$DEPS_DIR/include \
  -DCURL_LIBRARY=$DEPS_DIR/lib/libcurl.so \
  -DPYTHON_INCLUDE_DIR=$DEPS_DIR/include/python3.6m \
  -DPYTHON_LIBRARY=$DEPS_DIR/lib/libpython3.6m.so.1.0 \
  -DPYTHON_EXECUTABLE=$DEPS_DIR/bin/python3 \
  -DSWIG_EXECUTABLE=$DEPS_DIR/bin/swig \
  ../

RUN cmake --build . --target all -- -j 4


# /////////////////////////////////////////////
# installation step

RUN cmake --build . --target install

WORKDIR $VISUS_HOME/build/install

RUN ldd ./bin/libVisusKernel.so 
RUN cp $DEPS_DIR/lib/*.so* ./bin
RUN ln ./bin/libfreeimage-3.16.0.so ./bin/libfreeimage.so.3

RUN for it in $(find bin/*); do patchelf --set-rpath '$ORIGIN' $it; patchelf --shrink-rpath $it; done
RUN ldd ./bin/libVisusKernel.so 

RUN python3 setup.py bdist_wheel --python-tag=cp36 --plat-name=linux_x86_64
	
RUN WHEEL=$(find ./dist -iname "*.whl") && \
	WHEEL=${WHEEL/linux_x86_64/manylinux1_x86_64} && \
	mv dist/*.whl $WHEEL && \
	auditwheel show $WHEEL
	
#RUN twine upload --repository-url https://upload.pypi.org/legacy/ $WHEEL











  
