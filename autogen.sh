#!/bin/sh
#gettextize --no-changelog \
true \
  && aclocal -I m4 \
  && libtoolize \
  && autoheader \
  && automake -a \
  && autoconf

if test x$1 != x--no-conf; then
  if test -e conf; then
    sh conf;
  fi
fi
