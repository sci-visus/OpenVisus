#!/bin/bash

set -e 

rm -f /usr/local/apache2/logs/httpd.pid

source /etc/apache2/envvars

mkdir -p $APACHE_RUN_DIR $APACHE_LOCK_DIR $APACHE_LOG_DIR

exec /usr/sbin/apache2 -DFOREGROUND
