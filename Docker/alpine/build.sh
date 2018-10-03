#!/bin/sh

# to try it step by step
# docker run -it --entrypoint=/bin/sh alpine:3.7

set -e 

if [ "$GIT_BRANCH" = "" ]; then
	GIT_BRANCH=master
fi

if [ "$BUILD_DIR" = "" ]; then
	BUILD_DIR=/tmp/build_visus
fi

if [ "$VISUS_HOME" = "" ]; then
	VISUS_HOME=/home/visus
fi

# ////////////////////////////////////////////////
# dependencies
# ////////////////////////////////////////////////

apk add --no-cache python3 apache2 curl libstdc++
apk add --no-cache --virtual OpenVisusBuildDeps alpine-sdk swig apache2-dev curl-dev python3-dev git cmake
pip3 install --upgrade pip 
pip3 install numpy 

# ////////////////////////////////////////////////
# build OpenVisus
# ////////////////////////////////////////////////

git clone -b$GIT_BRANCH https://github.com/sci-visus/OpenVisus.git $BUILD_DIR 

cd $BUILD_DIR && mkdir -p ./build && cd ./build 

cmake ../ \
	-DVISUS_INTERNAL_ZLIB=1 \
	-DVISUS_INTERNAL_LZ4=1 \
	-DVISUS_INTERNAL_TINYXML=1 \
	-DVISUS_INTERNAL_FREEIMAGE=1 \
	-DVISUS_INTERNAL_OPENSSL=0 \
	-DVISUS_INTERNAL_CURL=0 \
	-DDISABLE_OPENMP=1 \
	-DVISUS_GUI=0 \
	-DVISUS_HOME=$VISUS_HOME \
	-DVISUS_PYTHON_SYS_PATH=$VISUS_HOME

cmake --build . --target all -- -j 4 
cmake --build . --target install  
cmake --build . --target deploy 

mv ./install $VISUS_HOME

chown -R apache:apache $VISUS_HOME 
chmod -R a+rX $VISUS_HOME 

# ////////////////////////////////////////////////
# install mod_visus
# ////////////////////////////////////////////////

mkdir -p /etc/apache2/vhosts.d 

echo "LoadModule visus_module $VISUS_HOME/bin/libmod_visus.so" >> /etc/apache2/httpd.conf 

echo "Include /etc/apache2/vhosts.d/*.conf" >> /etc/apache2/httpd.conf 

cat <<EOF > /etc/apache2/vhosts.d/000-default.conf
<VirtualHost *:80>
  DocumentRoot /var/www
  <Directory /var/www>
    Options Indexes FollowSymLinks MultiViews
    AllowOverride none
    Require all granted
  </Directory> 
  <Location /mod_visus>
    SetHandler visus
    DirectorySlash Off
    Header set Access-Control-Allow-Origin "*"
    AllowOverride none
    Require all granted
  </Location>
</VirtualHost>
EOF

# ////////////////////////////////////////////////
# httpd-foreground.sh
# ////////////////////////////////////////////////

cat <<EOF > /usr/local/bin/httpd-foreground.sh
#!/bin/sh
set -e 
mkdir -p /run/apache2 
mkdir -p /var/log/apache2
rm -f /run/apache2/httpd.pid
ln -sf /proc/self/fd/1 /var/log/apache2/access.log 
ln -sf /proc/self/fd/1 /var/log/apache2/error.log
rm -f /var/log/apache2/error.log 
rm -f /var/log/apache2/access.log
export LD_LIBRARY_PATH=$VISUS_HOME/bin
echo "<include url='\$VISUS_DATASETS/visus.config' />" > $VISUS_HOME/visus.config 
chmod a+r $VISUS_HOME/visus.config 
exec httpd -D FOREGROUND
EOF

chmod a+x /usr/local/bin/httpd-foreground.sh 

# ////////////////////////////////////////////////
# clean up
# ////////////////////////////////////////////////

rm -Rf $BUILD_DIR 
	
apk del OpenVisusBuildDeps

# /usr/local/bin/httpd-foreground.sh 
