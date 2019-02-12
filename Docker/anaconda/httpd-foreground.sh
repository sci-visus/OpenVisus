#!/bin/bash
set -e 
source /etc/apache2/envvars
rm -f $APACHE_PID_FILE
mkdir -p $APACHE_RUN_DIR $APACHE_LOCK_DIR $APACHE_LOG_DIR
rm -f $APACHE_LOG_DIR/error.log 
exec /usr/sbin/apache2 -DFOREGROUND
