#!/bin/bash

if [ "$LIBTOOLIZE" = "" ] && [ "`uname`" = "Darwin" ]; then
  LIBTOOLIZE=glibtoolize
fi

ACLOCAL=${ACLOCAL:-aclocal}
AUTOCONF=${AUTOCONF:-autoconf}
AUTOHEADER=${AUTOHEADER:-autoheader}
AUTOMAKE=${AUTOMAKE:-automake}
LIBTOOLIZE=${LIBTOOLIZE:-libtoolize}

set -ex
"$ACLOCAL"
"$LIBTOOLIZE"
"$AUTOHEADER"
"$AUTOCONF"
"$AUTOMAKE" --add-missing --copy

#aclocal && libtoolize && autoheader && automake --add-missing && autoconf
