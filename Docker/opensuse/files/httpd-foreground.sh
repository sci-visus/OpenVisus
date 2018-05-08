#!/bin/bash
set -e 
rm -f /run/httpd.pid
mkdir -p /var/log/apache2
rm -f /var/log/apache2/error.log /home/visus/visus.log
exec /usr/sbin/httpd -DFOREGROUND
