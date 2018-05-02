
# ZLIB_INCLUDE_DIRS=ExternalLibs/zlib-1.2.11 
# ZLIB_LIBRARIES=ExternalLibs/zlib-1.2.11      
wget https://zlib.net/zlib-1.2.11.tar.gz
tar xvzf zlib-1.2.11.tar.gz 
cd zlib-1.2.11
cmake .
make
cd .. && rm zlib-1.2.11.tar.gz

# LZ4_INCLUDE_DIR=ExternalLibs/lz4-1.8.1.2/lib
# LZ4_LIBRARY=ExternalLibs/lz4-1.8.1.2/lib/libl.so
wget https://github.com/lz4/lz4/archive/v1.8.1.2.tar.gz
tar xvzf v1.8.1.2.tar.gz  
cd lz4-1.8.1.2
make
cd .. && rm v1.8.1.2.tar.gz  

# TinyXML_INCLUDE_DIRS=ExternalLibs/tinyxml_2_6_2
# TinyXML_LIBRARIES=ExternalLibs/tinyxml_2_6_2/libtinyxml.a
wget https://downloads.sourceforge.net/project/tinyxml/tinyxml/2.6.2/tinyxml_2_6_2.zip
unzip tinyxml_2_6_2.zip && mv tinyxml tinyxml_2_6_2
cd tinyxml_2_6_2
g++ -c -Wall -Wno-unknown-pragmas -Wno-format -O3 -fPIC   tinyxml.cpp -o tinyxml.o
g++ -c -Wall -Wno-unknown-pragmas -Wno-format -O3 -fPIC   tinyxmlparser.cpp -o tinyxmlparser.o
g++ -c -Wall -Wno-unknown-pragmas -Wno-format -O3 -fPIC   xmltest.cpp -o xmltest.o
g++ -c -Wall -Wno-unknown-pragmas -Wno-format -O3 -fPIC   tinyxmlerror.cpp -o tinyxmlerror.o
g++ -c -Wall -Wno-unknown-pragmas -Wno-format -O3 -fPIC   tinystr.cpp -o tinystr.o
ar rcs libtinyxml.a tinyxml.o tinyxmlparser.o xmltest.o tinyxmlerror.o tinystr.o
cd .. && rm tinyxml_2_6_2.zip


# FREEIMAGE_INCLUDE_DIRS=ExternalLibs/FreeImage3160/Dist
# FREEIMAGE_LIBRARIES=ExternalLibs/FreeImage3160/Dist/libfreeimage.a
wget https://sourceforge.net/projects/freeimage/files/Source%20Distribution/3.16.0/FreeImage3160.zip # 3.17 has problems with gcc 5
unzip FreeImage3160.zip && mv FreeImage FreeImage3160
cd FreeImage3160
make
cd .. && rm FreeImage3160.zip

# OPENSSL_INCLUDE_DIR=ExternalLibs/openssl-1.0.2d/include
# OPENSSL_LIBRARIES=ExternalLibs
wget http://sourceforge.net/projects/openssl/files/openssl-1.0.2d-fips-2.0.10/openssl-1.0.2d-src.tar.gz
tar xvzf openssl-1.0.2d-src.tar.gz 
cd openssl-1.0.2d
./config
make
cd .. && rm openssl-1.0.2d-src.tar.gz 

# CURL_INCLUDE_DIRS=ExternalLibs/curl-7.59.0/lib
# CURL_LIBRARIES=ExternalLibs/curl-7.59.0/lib/.libs
wget https://curl.haxx.se/download/curl-7.59.0.tar.gz
tar xvzf curl-7.59.0.tar.gz 
cd curl-7.59.0
./configure
make
cd .. && rm curl-7.59.0.tar.gz 

# SWIG_EXECUTABLE=ExternalLibs/swig-3.0.12/install/share/swig/3.0.12
# SWIG_DIR=ExternalLibs/swig-3.0.12/install/bin/swig
wget https://downloads.sourceforge.net/project/swig/swig/swig-3.0.12/swig-3.0.12.tar.gz
tar xvzf swig-3.0.12.tar.gz
cd swig-3.0.12
./configure --prefix=$(pwd)/install
make 
make install
cd .. && rm swig-3.0.12.tar.gz





