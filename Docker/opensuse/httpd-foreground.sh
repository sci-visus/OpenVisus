#!/bin/bash
set -e 
rm -f /run/httpd.pid
mkdir -p /var/log/apache2
rm -f /var/log/apache2/error_log 
exec /usr/sbin/httpd -DFOREGROUND
