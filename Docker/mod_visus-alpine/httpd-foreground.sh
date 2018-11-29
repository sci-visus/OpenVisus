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
