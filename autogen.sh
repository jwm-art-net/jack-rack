#!/bin/sh
aclocal-1.6 -I m4 \
  && autoheader \
  && automake-1.6 -a \
  && autoconf

if test x$1 != x--no-conf; then
  if test -e conf; then
    sh conf;
  fi
fi
