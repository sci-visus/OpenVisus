#!/bin/bash
set -e 

# OpenSuSE apache startup scripts.
# Manually set envvars found in /etc/apache2/envvars on most other OSes.
# These are the values of the default apache installation. 
export APACHE_RUN_DIR=/var/run
export APACHE_PID_FILE=${APACHE_RUN_DIR}/httpd.pid
export APACHE_LOCK_DIR=/var/lock/apache2
export APACHE_LOG_DIR=/var/log/apache2


# after this point the script is identical to resources/shared/httpd-foreground.sh (and should remain so)
rm -f $APACHE_PID_FILE
mkdir -p $APACHE_RUN_DIR $APACHE_LOCK_DIR $APACHE_LOG_DIR
rm -f $APACHE_LOG_DIR/error.log 
exec /usr/sbin/httpd -DFOREGROUND
