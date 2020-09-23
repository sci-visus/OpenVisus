#!/bin/bash
set -e

# Apache gets grumpy about PID files pre-existing
rm -f /var/run/httpd.pid

/usr/sbin/httpd -DFOREGROUND
