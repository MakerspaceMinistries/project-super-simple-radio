#!/bin/sh

if [ -z "$GUNICORN_WORKERS" ]; then
  GUNICORN_WORKERS=$(( 2 * `cat /proc/cpuinfo | grep 'core id' | wc -l` + 1 ))
fi

# https://docs.gunicorn.org/en/latest/configure.html
gunicorn --chdir /app main:app -w $GUNICORN_WORKERS -b 0.0.0.0:80 --timeout 3600
