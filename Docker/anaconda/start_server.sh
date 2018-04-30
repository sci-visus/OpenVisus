#!/bin/bash

set -e 

# Use /tmp for working directory (important for read-only Singularity instances)
ln -s /visus         /tmp/visus
ln -s /visus/apache2 /tmp/apache2

# Remove any cruft from previous runs
rm -f /tmp/apache2/run/apache2/apache2.pid
rm -f /tmp/apache2/lock/apache2/*

source /etc/apache2/envvars
mkdir -p $APACHE_RUN_DIR $APACHE_LOCK_DIR $APACHE_LOG_DIR
exec /usr/sbin/apache2 -DFOREGROUND
