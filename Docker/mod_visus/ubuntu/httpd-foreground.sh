#!/bin/sh
set -e

# Apache gets grumpy about PID files pre-existing
rm -f /var/run/httpd.pid

exec httpd -DFOREGROUND "$@"


