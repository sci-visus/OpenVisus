#!/bin/bash
set -e 
mkdir /run/apache2 
rm -f /run/apache2/httpd.pid
mkdir -p /var/log/apache2
ln -sf /proc/self/fd/1 /var/log/apache2/access.log 
ln -sf /proc/self/fd/1 /var/log/apache2/error.log
rm -f /var/log/apache2/error.log 
rm -f /var/log/apache2/access.log
exec httpd -D FOREGROUND
