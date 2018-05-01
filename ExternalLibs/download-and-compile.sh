
# ZLIB_INCLUDE_DIRS=ExternalLibs/zlib-1.2.11 
# ZLIB_LIBRARIES=ExternalLibs/zlib-1.2.11      
wget https://zlib.net/zlib-1.2.11.tar.gz
tar xvzf zlib-1.2.11.tar.gz 
cd zlib-1.2.11
cmake .
make
cd .. && rm zlib-1.2.11.tar.gz

# LZ4_INCLUDE_DIR=ExternalLibs/lz4-1.8.1.2/lib
# LZ4_LIBRARY=ExternalLibs/lz4-1.8.1.2/lib
wget https://github.com/lz4/lz4/archive/v1.8.1.2.tar.gz
tar xvzf v1.8.1.2.tar.gz  
cd lz4-1.8.1.2
make
cd .. && rm v1.8.1.2.tar.gz  

# TinyXML_INCLUDE_DIRS=ExternalLibs/tinyxml_2_6_2
# TinyXML_LIBRARIES=ExternalLibs/tinyxml_2_6_2
wget https://downloads.sourceforge.net/project/tinyxml/tinyxml/2.6.2/tinyxml_2_6_2.zip
unzip tinyxml_2_6_2.zip && mv tinyxml tinyxml_2_6_2
cd tinyxml_2_6_2
make
ar rcs libtinyxml.a tinyxml.o tinyxmlparser.o xmltest.o tinyxmlerror.o tinystr.o
cd .. && rm tinyxml_2_6_2.zip


# FREEIMAGE_INCLUDE_DIRS=ExternalLibs/FreeImage3160/Dist
# FREEIMAGE_LIBRARIES=ExternalLibs/FreeImage3160/Dist
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



